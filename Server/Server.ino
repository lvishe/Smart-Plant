#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <DNSServer.h>
#include <ESPmDNS.h>
#include <Preferences.h>
#include <vector>
#include "LServer.h"
#include "html_pages.h"

// --- MP3 Player Support ---
#include "DFRobotDFPlayerMini.h"
#define PIN_MP3_TX 5
#define PIN_MP3_RX 4

DFRobotDFPlayerMini myDFPlayer;

Lserver server;
AsyncWebServer webServer(80);
DNSServer dnsServer;
Preferences preferences;

#define RESET_PREFERENCES_ON_BOOT false
#define MAX_HISTORY 20

unsigned long lastAlarmTime = 0;
const unsigned long ALARM_COOLDOWN = 15000;

struct SensorStream
{
    String name;
    float values[MAX_HISTORY];
    bool hasMax = false;
    float maxThreshold = 0.0f;
    
    bool alarmLatched = false;
    bool resetPending = true;

    SensorStream(String n) : name(n)
    {
        for (int i = 0; i < MAX_HISTORY; i++)
        {
            values[i] = -999.0f;
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
String cachedStatusJSON = "{\"lastData\":\"Waiting...\",\"lastIP\":\"None\"}";

bool pendingSave = false;
String pendingSsid = "";
String pendingPass = "";
bool needsRebuild = false;

String makePrefKey(const String& devId, const String& sensorName)
{
    String key = "m_" + devId + "_" + sensorName;
    if (key.length() > 15) key = key.substring(0, 15);
    return key;
}

bool addSensorDataToDevice(DeviceStream& dev, const String& name, float value)
{
    int foundIdx = -1;
    for (int i = 0; i < dev.sensors.size(); i++)
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
        float savedMax = preferences.getFloat(makePrefKey(dev.id, name).c_str(), -9999.0f);
        if (savedMax != -9999.0f)
        {
            newSensor.hasMax = true;
            newSensor.maxThreshold = savedMax;
        }
        dev.sensors.push_back(newSensor);
        foundIdx = dev.sensors.size() - 1;
        Serial.printf("[NEW SENSOR] %s for device %s\n", name.c_str(), dev.id.c_str());
    }
    
    SensorStream& sen = dev.sensors[foundIdx];
    sen.values[dev.historyIndex] = value;
    
    if (!sen.hasMax) return false;

    bool breached = (value > sen.maxThreshold);
    if (breached)
    {
        if (!sen.alarmLatched && sen.resetPending)
        {
            Serial.printf("[ALARM TRIGGER] %s on %s | Value: %.2f > Max: %.2f\n", 
                         name.c_str(), dev.id.c_str(), value, sen.maxThreshold);
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
    // FIX: Respect the mute timer. If the device is muted, force the UI alarm to stay OFF!
    if (millis() < dev.alarmMutedUntil)
    {
        dev.alarmActive = false;
        return;
    }

    bool wasActive = dev.alarmActive;
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

void rebuildCaches();

void processIncomingMessage(const ClientMessage& msg, const String& ip)
{
    Serial.printf("[DATA] Received from device %s | IP: %s\n", String(msg.device_id).c_str(), ip.c_str());
    String devId = String(msg.device_id);
    int devIdx = -1;
    for (int i = 0; i < activeDevices.size(); i++)
    {
        if (activeDevices[i].id == devId)
        {
            devIdx = i;
            break;
        }
    }

    if (devIdx == -1)
    {
        DeviceStream newDev;
        newDev.id = devId;
        newDev.ip = ip;
        newDev.updateDelay = preferences.getInt(("d_" + devId).c_str(), 5000);
        activeDevices.push_back(newDev);
        devIdx = activeDevices.size() - 1;
        Serial.printf("[NEW DEVICE] Registered: %s\n", devId.c_str());
    }
    
    DeviceStream& dev = activeDevices[devIdx];
    dev.ip = ip;
    dev.lastSeen = millis();

    for (auto& s : dev.sensors)
    {
        s.values[dev.historyIndex] = -999.0f;
    }

    bool triggerAlarm = false;
    triggerAlarm |= addSensorDataToDevice(dev, "Temp", msg.temp);
    triggerAlarm |= addSensorDataToDevice(dev, "Humidity", msg.humidity);
    triggerAlarm |= addSensorDataToDevice(dev, "Moisture", (float)msg.moisture);
    triggerAlarm |= addSensorDataToDevice(dev, "pH", msg.ph_level);
    triggerAlarm |= addSensorDataToDevice(dev, "TDS", (float)msg.tds_value);

    if (triggerAlarm && millis() > dev.alarmMutedUntil)
    {
        if (millis() - lastAlarmTime > ALARM_COOLDOWN)
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
    if (dev.historyCount < MAX_HISTORY) dev.historyCount++;
    lastReceivedData = "ID:" + devId + " T:" + String(msg.temp, 1) + " pH:" + String(msg.ph_level, 1);
    
    rebuildCaches();
}

void rebuildCaches()
{
    String json;
    json.reserve(8192);
    
    json = "{\"devices\":[";
    for (size_t d = 0; d < activeDevices.size(); d++)
    {
        if (d > 0) json += ",";
        DeviceStream& dev = activeDevices[d];
        
        json += "{\"id\":\"" + dev.id + "\",\"ip\":\"" + dev.ip + "\",\"delay\":" + String(dev.updateDelay) +
                ",\"alarm\":" + (dev.alarmActive ? "true" : "false") + ",\"labels\":[";
        int start = (dev.historyCount < MAX_HISTORY) ? 0 : dev.historyIndex;
        for (int i = 0; i < dev.historyCount; i++)
        {
            if (i > 0) json += ",";
            json += dev.timeHistory[(start + i) % MAX_HISTORY];
        }
        json += "],\"sensors\":[";
        for (size_t s = 0; s < dev.sensors.size(); s++)
        {
            if (s > 0) json += ",";
            SensorStream& sen = dev.sensors[s];
            json += "{\"name\":\"" + sen.name + "\"";
            if (sen.hasMax) json += ",\"max\":" + String(sen.maxThreshold, 1);
            json += ",\"data\":[";
            
            int startIdx = (dev.historyCount < MAX_HISTORY) ? 0 : dev.historyIndex;
            for (int i = 0; i < dev.historyCount; i++)
            {
                if (i > 0) json += ",";
                float v = sen.values[(startIdx + i) % MAX_HISTORY];
                json += (v <= -999.0f ? "null" : String(v, 1));
            }
            json += "]}";
        }
        json += "]}";
    }
    json += "]}";
    cachedHistoryJSON = json;
    cachedStatusJSON = "{\"lastData\":\"" + lastReceivedData + "\",\"lastIP\":\"" + lastClientIP + "\"}";
    rebuildIndex();
}

void cleanupInactiveDevices()
{
    unsigned long now = millis();
    bool changed = false;
    for (int i = 0; i < activeDevices.size(); )
    {
        if (now - activeDevices[i].lastSeen > 30000)
        {
            Serial.printf("[TIMEOUT] Device %s removed\n", activeDevices[i].id.c_str());
            activeDevices.erase(activeDevices.begin() + i);
            changed = true;
        }
        else i++;
    }
    if (changed) rebuildCaches();
}

void rebuildIndex()
{
    String page = index_html;
    page.replace("%WIFI_STATUS%", (WiFi.status() == WL_CONNECTED) ? "CONNECTED" : "DISCONNECTED");
    page.replace("%STA_IP%", (WiFi.status() == WL_CONNECTED) ? WiFi.localIP().toString() : WiFi.softAPIP().toString());
    cachedIndexHTML = page;
}

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

    preferences.begin("wifi-gate", false);
    if (RESET_PREFERENCES_ON_BOOT) preferences.clear();

    ssid = preferences.getString("ssid", "");
    pass = preferences.getString("pass", "");

    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP("SmartPlant-Gateway", "12345678");
    Serial.printf("AP Started → SSID: SmartPlant-Gateway | IP: %s\n", WiFi.softAPIP().toString().c_str());

    dnsServer.start(53, "*", WiFi.softAPIP());
    Serial.println("DNS Server started (captive portal)");

    server.begin();
    if (ssid != "")
    {
        Serial.printf("Connecting to saved WiFi: %s\n", ssid.c_str());
        WiFi.begin(ssid.c_str(), pass.c_str());
    }

    rebuildIndex();

    // Web Routes
    webServer.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(200, "text/html", cachedIndexHTML);
    });
    webServer.on("/setup", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send_P(200, "text/html", setup_html);
    });
    webServer.on("/history", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(200, "application/json", cachedHistoryJSON);
    });
    webServer.on("/set_max", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (request->hasParam("dev") && request->hasParam("sensor") && request->hasParam("val")) {
            String devId = request->getParam("dev")->value();
            String sensorName = request->getParam("sensor")->value();
            String valStr = request->getParam("val")->value();
            for (auto& dev : activeDevices) {
                if (dev.id == devId) 
                {
                    dev.alarmMutedUntil = 0;
                    for (auto& sen : dev.sensors) {
                        if (sen.name == sensorName) {
                            if (valStr.length() == 0) sen.hasMax = false;
                            else { sen.hasMax = true; sen.maxThreshold = valStr.toFloat(); }
                            break;
                        }
                    }
                    break;
                }
            }
            String prefKey = makePrefKey(devId, sensorName);
            if (valStr.length() == 0) preferences.remove(prefKey.c_str());
            else preferences.putFloat(prefKey.c_str(), valStr.toFloat());
            needsRebuild = true;
            request->send(200, "text/plain", "Threshold Saved");
        }
    });
    webServer.on("/set_delay", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (request->hasParam("dev") && request->hasParam("delay")) {
            String devId = request->getParam("dev")->value();
            int newDelay = request->getParam("delay")->value().toInt();
            for (auto& dev : activeDevices) {
                if (dev.id == devId) dev.updateDelay = newDelay;
            }
            preferences.putInt(("d_" + devId).c_str(), newDelay);
            needsRebuild = true;
            request->send(200, "text/plain", "Delay Saved");
        }
    });
    webServer.on("/clear_alarm", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (request->hasParam("dev")) {
            String devId = request->getParam("dev")->value();
            Serial.printf("[UI] Clear alarm requested for %s\n", devId.c_str());
            for (auto& dev : activeDevices) {
                if (dev.id == devId) {
                    dev.alarmActive = false;
                    dev.alarmMutedUntil = millis() + 60000;
                    for (auto& sen : dev.sensors) {
                        sen.alarmLatched = false;
                        sen.resetPending = true;
                    }
                    break;
                }
            }
            needsRebuild = true;
            request->send(200, "text/plain", "Alarm Cleared");
        }
    });
    webServer.on("/save", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (request->hasParam("ssid")) {
            pendingSsid = request->getParam("ssid")->value();
            pendingPass = request->hasParam("pass") ? request->getParam("pass")->value() : "";
            pendingSave = true;
            request->send(200, "text/plain", "Saved. Rebooting...");
        }
    });
    webServer.onNotFound([](AsyncWebServerRequest *request) {
        if (request->client()->localIP() == WiFi.softAPIP()) request->redirect("/setup");
        else request->send(404, "text/plain", "Not Found");
    });
    webServer.begin();
    Serial.println("=== Web Server Started - Gateway Ready ===");
}

unsigned long lastIndexRebuild = 0;
unsigned long lastWifiCheck = 0;
unsigned long lastCleanup = 0;

void loop()
{
    dnsServer.processNextRequest();
    if (needsRebuild)
    {
        rebuildCaches();
        needsRebuild = false;
    }
    
    if (ssid != "" && WiFi.status() != WL_CONNECTED && millis() - lastWifiCheck > 10000)
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

    if (millis() - lastIndexRebuild > 10000)
    {
        rebuildIndex();
        lastIndexRebuild = millis();
    }

    if (millis() - lastCleanup > 5000)
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
        for (auto& dev : activeDevices)
        {
            if (dev.id == String(incomingMsg.device_id))
            {
                targetDelay = dev.updateDelay;
                break;
            }
        }
        server.sendUpdate(targetDelay);
    }
}