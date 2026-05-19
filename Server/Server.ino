#include <WiFi.h>
#include <ESPmDNS.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <DNSServer.h>
#include <Preferences.h>
#include <vector>

#include "LServer.h"
#include "html_pages.h"

#include "DFRobotDFPlayerMini.h"

#define PIN_MP3_TX 5
#define PIN_MP3_RX 4

#define RESET_PREFERENCES_ON_BOOT false
#define MAX_HISTORY 20

/** mDNS: http://plant.local/ on home LAN when STA is connected. */
#define MDNS_HOSTNAME "plant"

#define AP_SSID "SmartPlant"
#define AP_PASSWORD "12345678"

#define PREFS_NAMESPACE "wifi-gate"

static const unsigned long kAlarmCooldownMs = 15000;
static const unsigned long kHistoryJsonRebuildMinMs = 100;
static const unsigned long kMdnsRetryMs = 30000;
static const unsigned long kDeviceIdleTimeoutMs = 30000;
static const unsigned long kAlarmMuteAfterClearMs = 60000;
static const unsigned long kWifiReconnectIntervalMs = 10000;
static const unsigned long kIndexRebuildIntervalMs = 10000;
static const unsigned long kCleanupIntervalMs = 5000;

static const float kNoReading = -999.0f;
static const float kPrefsNoMax = -9999.0f;

// -----------------------------------------------------------------------------

DFRobotDFPlayerMini myDFPlayer;
Lserver server;
AsyncWebServer webServer(80);
DNSServer dnsServer;
Preferences preferences;

struct SensorStream
{
    String name;
    float values[MAX_HISTORY];
    bool hasMax = false;
    float maxThreshold = 0.0f;
    bool alarmLatched = false;
    bool resetPending = true;

    explicit SensorStream(String n)
        : name(n)
    {
        for (int i = 0; i < MAX_HISTORY; i++)
        {
            values[i] = kNoReading;
        }
    }
};

struct DeviceStream
{
    String id;
    String ip;
    int updateDelay = 5000;
    std::vector<SensorStream> sensors;
    String timeHistory[MAX_HISTORY];
    int historyIndex = 0;
    int historyCount = 0;
    unsigned long lastSeen = 0;
    bool alarmActive = false;
    unsigned long alarmMutedUntil = 0;
};

std::vector<DeviceStream> activeDevices;

String lastReceivedData = "Waiting...";
String lastClientIP = "None";
String ssid = "";
String pass = "";

String cachedIndexHTML = "";
String cachedHistoryJSON = "{\"devices\":[]}";

bool pendingSave = false;
String pendingSsid = "";
String pendingPass = "";
bool needsRebuild = false;

unsigned long lastAlarmTime = 0;
unsigned long lastIndexRebuild = 0;
unsigned long lastWifiCheck = 0;
unsigned long lastCleanup = 0;

static bool s_historyJsonDirty = false;
static unsigned long s_lastHistoryJsonRebuildMs = 0;

static bool s_mdnsActive = false;
static unsigned long s_mdnsLastBeginAttemptMs = 0;

// -----------------------------------------------------------------------------
// Forward declarations

void rebuildCaches();
void rebuildIndex();

// -----------------------------------------------------------------------------

static void updateMdnsForSta()
{
    if (WiFi.status() == WL_CONNECTED)
    {
        if (s_mdnsActive)
        {
            return;
        }

        unsigned long now = millis();
        if (s_mdnsLastBeginAttemptMs != 0 && (now - s_mdnsLastBeginAttemptMs) < kMdnsRetryMs)
        {
            return;
        }
        s_mdnsLastBeginAttemptMs = now;

        if (MDNS.begin(MDNS_HOSTNAME))
        {
            MDNS.addService("http", "tcp", 80);
            s_mdnsActive = true;
            s_mdnsLastBeginAttemptMs = 0;
            Serial.printf("mDNS started: http://%s.local/ (STA %s)\n", MDNS_HOSTNAME,
                          WiFi.localIP().toString().c_str());
        }
        else
        {
            Serial.println("mDNS: MDNS.begin failed, will retry");
        }
    }
    else
    {
        s_mdnsLastBeginAttemptMs = 0;
        if (s_mdnsActive)
        {
            MDNS.end();
            s_mdnsActive = false;
            Serial.println("mDNS stopped (STA not connected)");
        }
    }
}

static int findDeviceIndexById(const String& devId)
{
    for (size_t i = 0; i < activeDevices.size(); i++)
    {
        if (activeDevices[i].id == devId)
        {
            return (int)i;
        }
    }
    return -1;
}

String makePrefKey(const String& devId, const String& sensorName)
{
    String key = "m_" + devId + "_" + sensorName;
    if (key.length() > 15)
    {
        key = key.substring(0, 15);
    }
    return key;
}

bool addSensorDataToDevice(DeviceStream& dev, const String& name, float value)
{
    int foundIdx = -1;
    for (int i = 0; i < (int)dev.sensors.size(); i++)
    {
        if (dev.sensors[i].name == name)
        {
            foundIdx = i;
            break;
        }
    }

    if (foundIdx == -1)
    {
        SensorStream newSensor(name);
        float savedMax = preferences.getFloat(makePrefKey(dev.id, name).c_str(), kPrefsNoMax);
        if (savedMax != kPrefsNoMax)
        {
            newSensor.hasMax = true;
            newSensor.maxThreshold = savedMax;
        }
        dev.sensors.push_back(newSensor);
        foundIdx = (int)dev.sensors.size() - 1;
        Serial.printf("[NEW SENSOR] %s for device %s\n", name.c_str(), dev.id.c_str());
    }

    SensorStream& sen = dev.sensors[foundIdx];
    sen.values[dev.historyIndex] = value;

    if (!sen.hasMax)
    {
        return false;
    }

    const bool breached = (value > sen.maxThreshold);
    if (breached)
    {
        if (!sen.alarmLatched && sen.resetPending)
        {
            Serial.printf("[ALARM TRIGGER] %s on %s | Value: %.2f > Max: %.2f\n", name.c_str(), dev.id.c_str(), value,
                          sen.maxThreshold);
            sen.alarmLatched = true;
            sen.resetPending = false;
            return true;
        }
    }
    else
    {
        if (sen.alarmLatched)
        {
            Serial.printf("[SAFE] %s on %s returned below threshold (%.2f)\n", name.c_str(), dev.id.c_str(), value);
            sen.resetPending = true;
        }
        sen.alarmLatched = false;
    }
    return false;
}

void updateDeviceAlarmState(DeviceStream& dev)
{
    if (millis() < dev.alarmMutedUntil)
    {
        dev.alarmActive = false;
        return;
    }

    const bool wasActive = dev.alarmActive;
    dev.alarmActive = false;
    for (auto& s : dev.sensors)
    {
        if (s.alarmLatched)
        {
            dev.alarmActive = true;
            break;
        }
    }
    if (wasActive && !dev.alarmActive)
    {
        Serial.printf("[ALARM AUTO CLEAR] Device %s is now safe\n", dev.id.c_str());
    }
}

void processIncomingMessage(const ClientMessage& msg, const String& ip)
{
    Serial.printf("[DATA] Received from device %s | IP: %s\n", String(msg.device_id).c_str(), ip.c_str());
    const String devId = String(msg.device_id);
    int devIdx = findDeviceIndexById(devId);

    if (devIdx == -1)
    {
        DeviceStream newDev;
        newDev.id = devId;
        newDev.ip = ip;
        newDev.updateDelay = preferences.getInt(("d_" + devId).c_str(), 5000);
        activeDevices.push_back(newDev);
        devIdx = (int)activeDevices.size() - 1;
        Serial.printf("[NEW DEVICE] Registered: %s\n", devId.c_str());
    }

    DeviceStream& dev = activeDevices[(size_t)devIdx];
    dev.ip = ip;
    dev.lastSeen = millis();

    for (auto& s : dev.sensors)
    {
        s.values[dev.historyIndex] = kNoReading;
    }

    bool triggerAlarm = false;
    triggerAlarm |= addSensorDataToDevice(dev, "Temp", msg.temp);
    triggerAlarm |= addSensorDataToDevice(dev, "Humidity", msg.humidity);
    triggerAlarm |= addSensorDataToDevice(dev, "Moisture", (float)msg.moisture);
    triggerAlarm |= addSensorDataToDevice(dev, "pH", msg.ph_level);
    triggerAlarm |= addSensorDataToDevice(dev, "TDS", (float)msg.tds_value);

    if (triggerAlarm && millis() > dev.alarmMutedUntil)
    {
        if (millis() - lastAlarmTime > kAlarmCooldownMs)
        {
            Serial.println("**************************************************");
            Serial.printf("*** ALARM SOUNDING - Device %s ***\n", dev.id.c_str());
            Serial.println("**************************************************");
            myDFPlayer.play(1);
            lastAlarmTime = millis();
        }
    }

    updateDeviceAlarmState(dev);

    dev.timeHistory[dev.historyIndex] = String(dev.historyCount + 1);
    dev.historyIndex = (dev.historyIndex + 1) % MAX_HISTORY;
    if (dev.historyCount < MAX_HISTORY)
    {
        dev.historyCount++;
    }
    lastReceivedData = "ID:" + devId + " T:" + String(msg.temp, 1) + " pH:" + String(msg.ph_level, 1);

    s_historyJsonDirty = true;
}

void rebuildCaches()
{
    String json;
    json.reserve(8192);

    json = "{\"devices\":[";
    for (size_t d = 0; d < activeDevices.size(); d++)
    {
        if (d > 0)
        {
            json += ",";
        }
        DeviceStream& dev = activeDevices[d];

        json += "{\"id\":\"" + dev.id + "\",\"ip\":\"" + dev.ip + "\",\"delay\":" + String(dev.updateDelay) +
                ",\"alarm\":" + (dev.alarmActive ? "true" : "false") + ",\"labels\":[";
        const int start = (dev.historyCount < MAX_HISTORY) ? 0 : dev.historyIndex;
        for (int i = 0; i < dev.historyCount; i++)
        {
            if (i > 0)
            {
                json += ",";
            }
            json += dev.timeHistory[(start + i) % MAX_HISTORY];
        }
        json += "],\"sensors\":[";
        for (size_t s = 0; s < dev.sensors.size(); s++)
        {
            if (s > 0)
            {
                json += ",";
            }
            SensorStream& sen = dev.sensors[s];
            json += "{\"name\":\"" + sen.name + "\"";
            if (sen.hasMax)
            {
                json += ",\"max\":" + String(sen.maxThreshold, 1);
            }
            json += ",\"data\":[";

            const int startIdx = (dev.historyCount < MAX_HISTORY) ? 0 : dev.historyIndex;
            for (int i = 0; i < dev.historyCount; i++)
            {
                if (i > 0)
                {
                    json += ",";
                }
                const float v = sen.values[(startIdx + i) % MAX_HISTORY];
                json += (v <= kNoReading ? "null" : String(v, 1));
            }
            json += "]}";
        }
        json += "]}";
    }
    json += "]}";
    cachedHistoryJSON = json;
    rebuildIndex();
}

void cleanupInactiveDevices()
{
    const unsigned long now = millis();
    bool changed = false;
    for (int i = 0; i < (int)activeDevices.size();)
    {
        if ((unsigned long)(now - activeDevices[(size_t)i].lastSeen) > kDeviceIdleTimeoutMs)
        {
            Serial.printf("[TIMEOUT] Device %s removed\n", activeDevices[(size_t)i].id.c_str());
            activeDevices.erase(activeDevices.begin() + i);
            changed = true;
        }
        else
        {
            i++;
        }
    }
    if (changed)
    {
        rebuildCaches();
    }
}

void rebuildIndex()
{
    String page = index_html;
    page.replace("%WIFI_STATUS%", (WiFi.status() == WL_CONNECTED) ? "CONNECTED" : "DISCONNECTED");
    page.replace("%STA_IP%", (WiFi.status() == WL_CONNECTED) ? WiFi.localIP().toString() : WiFi.softAPIP().toString());
    cachedIndexHTML = page;
}

// -----------------------------------------------------------------------------
// HTTP handlers (Allman)

static void onHttpRoot(AsyncWebServerRequest* request)
{
    request->send(200, "text/html", cachedIndexHTML);
}

static void onHttpSetup(AsyncWebServerRequest* request)
{
    request->send_P(200, "text/html", setup_html);
}

static void onHttpHistory(AsyncWebServerRequest* request)
{
    request->send(200, "application/json", cachedHistoryJSON);
}

static void onHttpSetMax(AsyncWebServerRequest* request)
{
    if (!request->hasParam("dev") || !request->hasParam("sensor") || !request->hasParam("val"))
    {
        request->send(400, "text/plain", "Missing dev, sensor, or val");
        return;
    }

    const String devId = request->getParam("dev")->value();
    const String sensorName = request->getParam("sensor")->value();
    const String valStr = request->getParam("val")->value();

    for (auto& dev : activeDevices)
    {
        if (dev.id != devId)
        {
            continue;
        }
        dev.alarmMutedUntil = 0;
        for (auto& sen : dev.sensors)
        {
            if (sen.name != sensorName)
            {
                continue;
            }
            if (valStr.length() == 0)
            {
                sen.hasMax = false;
            }
            else
            {
                sen.hasMax = true;
                sen.maxThreshold = valStr.toFloat();
            }
            break;
        }
        break;
    }

    const String prefKey = makePrefKey(devId, sensorName);
    if (valStr.length() == 0)
    {
        preferences.remove(prefKey.c_str());
    }
    else
    {
        preferences.putFloat(prefKey.c_str(), valStr.toFloat());
    }
    needsRebuild = true;
    request->send(200, "text/plain", "Threshold Saved");
}

static void onHttpSetDelay(AsyncWebServerRequest* request)
{
    if (!request->hasParam("dev") || !request->hasParam("delay"))
    {
        request->send(400, "text/plain", "Missing dev or delay");
        return;
    }

    const String devId = request->getParam("dev")->value();
    const int newDelay = request->getParam("delay")->value().toInt();
    for (auto& dev : activeDevices)
    {
        if (dev.id == devId)
        {
            dev.updateDelay = newDelay;
        }
    }
    preferences.putInt(("d_" + devId).c_str(), newDelay);
    needsRebuild = true;
    request->send(200, "text/plain", "Delay Saved");
}

static void onHttpClearAlarm(AsyncWebServerRequest* request)
{
    if (!request->hasParam("dev"))
    {
        request->send(400, "text/plain", "Missing dev");
        return;
    }

    const String devId = request->getParam("dev")->value();
    Serial.printf("[UI] Clear alarm requested for %s\n", devId.c_str());
    for (auto& dev : activeDevices)
    {
        if (dev.id != devId)
        {
            continue;
        }
        dev.alarmActive = false;
        dev.alarmMutedUntil = millis() + kAlarmMuteAfterClearMs;
        for (auto& sen : dev.sensors)
        {
            sen.alarmLatched = false;
            sen.resetPending = true;
        }
        break;
    }
    needsRebuild = true;
    request->send(200, "text/plain", "Alarm Cleared");
}

static void onHttpSave(AsyncWebServerRequest* request)
{
    if (!request->hasParam("ssid"))
    {
        request->send(400, "text/plain", "Missing ssid");
        return;
    }
    pendingSsid = request->getParam("ssid")->value();
    pendingPass = request->hasParam("pass") ? request->getParam("pass")->value() : "";
    pendingSave = true;
    request->send(200, "text/plain", "Saved. Rebooting...");
}

static void onHttpNotFound(AsyncWebServerRequest* request)
{
    if (request->client()->localIP() == WiFi.softAPIP())
    {
        request->redirect("/setup");
    }
    else
    {
        request->send(404, "text/plain", "Not Found");
    }
}

// -----------------------------------------------------------------------------

void setup()
{
    Serial.begin(115200);
    delay(1000);
    Serial.println("\n=== Smart Plant Gateway Starting ===");

    Serial1.begin(9600, SERIAL_8N1, PIN_MP3_RX, PIN_MP3_TX);
    if (myDFPlayer.begin(Serial1))
    {
        myDFPlayer.volume(25);
        Serial.println("DFPlayer Ready");
    }
    else
    {
        Serial.println("DFPlayer failed to initialize");
    }

    preferences.begin(PREFS_NAMESPACE, false);
    if (RESET_PREFERENCES_ON_BOOT)
    {
        preferences.clear();
    }

    ssid = preferences.getString("ssid", "");
    pass = preferences.getString("pass", "");

    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(AP_SSID);
    Serial.printf("AP started — SSID: %s | IP: %s\n", AP_SSID, WiFi.softAPIP().toString().c_str());

    dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
    dnsServer.setTTL(300);
    dnsServer.start(53, "*", WiFi.softAPIP());
    Serial.println("DNS server started (captive portal)");

    server.begin();

    if (ssid != "")
    {
        Serial.printf("Connecting to saved WiFi: %s\n", ssid.c_str());
        WiFi.begin(ssid.c_str(), pass.c_str());
    }

    rebuildIndex();

    webServer.on("/", HTTP_GET, onHttpRoot);
    webServer.on("/setup", HTTP_GET, onHttpSetup);
    webServer.on("/history", HTTP_GET, onHttpHistory);
    webServer.on("/set_max", HTTP_GET, onHttpSetMax);
    webServer.on("/set_delay", HTTP_GET, onHttpSetDelay);
    webServer.on("/clear_alarm", HTTP_GET, onHttpClearAlarm);
    webServer.on("/save", HTTP_GET, onHttpSave);
    webServer.onNotFound(onHttpNotFound);
    webServer.begin();

    Serial.println("=== Web server started — gateway ready ===");
}

void loop()
{
    updateMdnsForSta();
    dnsServer.processNextRequest();

    if (needsRebuild)
    {
        rebuildCaches();
        needsRebuild = false;
        s_historyJsonDirty = false;
        s_lastHistoryJsonRebuildMs = millis();
    }
    else if (s_historyJsonDirty && (millis() - s_lastHistoryJsonRebuildMs >= kHistoryJsonRebuildMinMs))
    {
        rebuildCaches();
        s_historyJsonDirty = false;
        s_lastHistoryJsonRebuildMs = millis();
    }

    if (ssid != "" && WiFi.status() != WL_CONNECTED && millis() - lastWifiCheck > kWifiReconnectIntervalMs)
    {
        WiFi.begin(ssid.c_str(), pass.c_str());
        lastWifiCheck = millis();
    }

    if (pendingSave)
    {
        pendingSave = false;
        preferences.putString("ssid", pendingSsid);
        preferences.putString("pass", pendingPass);
        delay(2000);
        ESP.restart();
    }

    if (millis() - lastIndexRebuild > kIndexRebuildIntervalMs)
    {
        rebuildIndex();
        lastIndexRebuild = millis();
    }

    if (millis() - lastCleanup > kCleanupIntervalMs)
    {
        cleanupInactiveDevices();
        lastCleanup = millis();
    }

    ClientMessage incomingMsg;
    String remoteIP;
    if (server.receivePacket(incomingMsg, remoteIP))
    {
        lastClientIP = remoteIP;
        processIncomingMessage(incomingMsg, remoteIP);

        uint32_t targetDelay = 5000;
        const String id = String(incomingMsg.device_id);
        for (auto& dev : activeDevices)
        {
            if (dev.id == id)
            {
                targetDelay = (uint32_t)dev.updateDelay;
                break;
            }
        }
        server.sendUpdate(targetDelay);
    }
}
