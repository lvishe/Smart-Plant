#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <DNSServer.h>
#include <ESPmDNS.h>
#include <Preferences.h>
#include <vector>
#include "LServer.h"
#include "html_pages.h"

Lserver server;
AsyncWebServer webServer(80);
DNSServer dnsServer;
Preferences preferences;

#define RESET_PREFERENCES_ON_BOOT false
#define MAX_HISTORY 20

// Cooldown timer to prevent serial spam
unsigned long lastAlarmTime = 0;
const unsigned long ALARM_COOLDOWN = 15000; 

struct SensorStream 
{
    String name;
    float values[MAX_HISTORY];
    bool hasMax = false;
    float maxThreshold = 0.0f;
    
    SensorStream(String n) 
    {
        name = n;
        for (int i = 0; i < MAX_HISTORY; i++) values[i] = -999.0f;
    }
};

struct DeviceStream 
{
    String id;
    String ip;
    int updateDelay = 5000; // Track delay per client
    std::vector<SensorStream> sensors;
    String timeHistory[MAX_HISTORY];
    int historyIndex = 0;
    int historyCount = 0;
    unsigned long lastSeen = 0;
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

// Ensure Preference keys don't exceed length limits
String makePrefKey(const String& devId, const String& sensorName) 
{
    String key = "m_" + devId + "_" + sensorName;
    if (key.length() > 15) key = key.substring(0, 15);
    return key;
}

bool addSensorDataToDevice(DeviceStream& dev, const String& name, float value) 
{
    int foundIdx = -1;
    for (int i = 0; i < dev.sensors.size(); i++) {
        if (dev.sensors[i].name == name) {
            foundIdx = i;
            break;
        }
    }
    
    if (foundIdx == -1) {
        SensorStream newSensor(name);
        String prefKey = makePrefKey(dev.id, name);
        float savedMax = preferences.getFloat(prefKey.c_str(), -9999.0f);
        if (savedMax != -9999.0f) {
            newSensor.hasMax = true;
            newSensor.maxThreshold = savedMax;
        }
        dev.sensors.push_back(newSensor);
        foundIdx = dev.sensors.size() - 1;
    }
    
    dev.sensors[foundIdx].values[dev.historyIndex] = value;

    if (dev.sensors[foundIdx].hasMax && value > dev.sensors[foundIdx].maxThreshold) 
    {
        return true; // Threshold breached
    }

    return false;
}

void processIncomingMessage(const ClientMessage& msg, const String& ip) 
{
    String devId = String(msg.device_id);
    
    int devIdx = -1;
    for (int i = 0; i < activeDevices.size(); i++) 
    {
        if (activeDevices[i].id == devId)
        {
            devIdx = i; break;
        }
    }

    if (devIdx == -1) 
    {
        DeviceStream newDev;
        newDev.id = devId;
        newDev.ip = ip;
        
        // Load saved delay for this device from preferences
        String delayKey = "d_" + devId;
        newDev.updateDelay = preferences.getInt(delayKey.c_str(), 5000);
        
        activeDevices.push_back(newDev);
        devIdx = activeDevices.size() - 1;
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

    
    if (triggerAlarm) 
    {
        if (millis() - lastAlarmTime > ALARM_COOLDOWN) 
        {
            Serial.println("**************************************************");
            Serial.printf("ALARM! Device %s exceeded a sensor threshold!\n", dev.id.c_str());
            Serial.println("**************************************************");
            lastAlarmTime = millis();
        }
    }
    
    dev.timeHistory[dev.historyIndex] = String(dev.historyCount + 1);
    dev.historyIndex = (dev.historyIndex + 1) % MAX_HISTORY;
    if (dev.historyCount < MAX_HISTORY) dev.historyCount++;

    lastReceivedData = "ID:" + devId + " T:" + String(msg.temp, 1) + " pH:" + String(msg.ph_level, 1);
}


void cleanupInactiveDevices() {
    unsigned long now = millis();
    // Timeout threshold (e.g., 30 seconds). 
    // You can also make this dynamic: activeDevices[i].updateDelay * 3
    const unsigned long TIMEOUT = 30000; 
    bool changed = false;

    for (int i = 0; i < activeDevices.size(); ) {
        if (now - activeDevices[i].lastSeen > TIMEOUT) {
            Serial.printf("Device %s timed out. Removing from dashboard.\n", activeDevices[i].id.c_str());
            activeDevices.erase(activeDevices.begin() + i);
            changed = true;
            // Do not increment 'i' here because erase shifts the vector
        } else {
            i++;
        }
    }

    if (changed) {
        rebuildCaches(); // Refresh the web dashboard data immediately
    }
}



void rebuildIndex() {
    String page = index_html;
    page.replace("%WIFI_STATUS%", (WiFi.status() == WL_CONNECTED) ? "CONNECTED" : "DISCONNECTED");
    page.replace("%STA_IP%", (WiFi.status() == WL_CONNECTED) ? WiFi.localIP().toString() : "192.168.4.1");
    cachedIndexHTML = page;
}

void rebuildCaches() {
    String json;
    json.reserve(2048); 
    
    json = "{\"devices\":[";
    for (int d = 0; d < activeDevices.size(); d++) {
        if (d > 0) json += ",";
        DeviceStream& dev = activeDevices[d];
        
        json += "{\"id\":\"" + dev.id + "\",\"ip\":\"" + dev.ip + "\",\"delay\":" + String(dev.updateDelay) + ",\"labels\":[";
        
        int start = (dev.historyCount < MAX_HISTORY) ? 0 : dev.historyIndex;
        for (int i = 0; i < dev.historyCount; i++) {
            int ri = (start + i) % MAX_HISTORY;
            if (i > 0) json += ",";
            json += dev.timeHistory[ri];
        }
        
        json += "],\"sensors\":[";
        for (int s = 0; s < dev.sensors.size(); s++) {
            if (s > 0) json += ",";
            SensorStream& sen = dev.sensors[s];
            json += "{\"name\":\"" + sen.name + "\"";
            
            if (sen.hasMax) {
                json += ",\"max\":" + String(sen.maxThreshold);
            }
            
            json += ",\"data\":[";
            for (int i = 0; i < dev.historyCount; i++) {
                int ri = (start + i) % MAX_HISTORY;
                float v = sen.values[ri];
                if (i > 0) json += ",";
                if (v <= -999.0f) json += "null";
                else { 
                    char buf[12];
                    dtostrf(v, 1, 1, buf); 
                    json += buf; 
                }
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

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    preferences.begin("wifi-gate", false);
    if (RESET_PREFERENCES_ON_BOOT) { preferences.clear(); }

    ssid = preferences.getString("ssid", "");
    pass = preferences.getString("pass", "");

    WiFi.mode(WIFI_AP_STA);
    server.begin();
    dnsServer.start(53, "*", WiFi.softAPIP());

    if (ssid != "") {
        WiFi.begin(ssid.c_str(), pass.c_str());
        unsigned long t = millis();
        while (WiFi.status() != WL_CONNECTED && millis() - t < 12000) { 
            delay(500); 
            Serial.print(".");
        }
        if (WiFi.status() == WL_CONNECTED) {
            if (MDNS.begin("plant")) Serial.println("mDNS: http://plant.local");
        }
    }

    rebuildIndex();

    webServer.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(200, "text/html", cachedIndexHTML);
    });
    
    webServer.on("/setup", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send_P(200, "text/html", setup_html);
    });

    webServer.on("/history", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(200, "application/json", cachedHistoryJSON);
    });

    webServer.on("/status", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(200, "application/json", cachedStatusJSON);
    });

    webServer.on("/set_max", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (request->hasParam("dev") && request->hasParam("sensor") && request->hasParam("val")) {
            String devId = request->getParam("dev")->value();
            String sensorName = request->getParam("sensor")->value();
            String valStr = request->getParam("val")->value();
            
            for (auto& dev : activeDevices) {
                if (dev.id == devId) {
                    for (auto& sen : dev.sensors) {
                        if (sen.name == sensorName) {
                            if (valStr.length() == 0) { sen.hasMax = false; } 
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
            
            rebuildCaches(); 
            request->send(200, "text/plain", "Threshold Saved");
        } else {
            request->send(400, "text/plain", "Missing Parameters");
        }
    });

    webServer.on("/set_delay", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (request->hasParam("dev") && request->hasParam("delay")) {
            String devId = request->getParam("dev")->value();
            int newDelay = request->getParam("delay")->value().toInt();
            
            for (auto& dev : activeDevices) {
                if (dev.id == devId) {
                    dev.updateDelay = newDelay;
                    break;
                }
            }
            
            String prefKey = "d_" + devId; 
            preferences.putInt(prefKey.c_str(), newDelay);
            
            rebuildCaches();
            request->send(200, "text/plain", "Delay Saved");
        } else {
            request->send(400, "text/plain", "Missing Parameters");
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
}

unsigned long lastIndexRebuild = 0;
unsigned long lastWifiCheck = 0;
unsigned long lastCleanup = 0;

void loop() 
{
    dnsServer.processNextRequest();
    
    if (ssid != "" && WiFi.status() != WL_CONNECTED && millis() - lastWifiCheck > 10000) 
    {
        Serial.println("Connection lost. Reconnecting to WiFi...");
        WiFi.disconnect();
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
        // BUG FIX: Update the global IP for the web interface
        lastClientIP = remoteIP; 
        
        Serial.printf("Received data from ID: %X at IP: %s\n", incomingMsg.device_id, remoteIP.c_str());
        
        processIncomingMessage(incomingMsg, remoteIP);
        
        // Find the specific delay for this device
        uint32_t targetDelay = 5000;
        for (auto& dev : activeDevices) 
        {
            if (dev.id == String(incomingMsg.device_id)) {
                targetDelay = dev.updateDelay;
                break;
            }
        }

        server.sendUpdate(targetDelay);
        
        // BUG FIX: Rebuild JSON caches so the web dashboard shows the update immediately
        rebuildCaches(); 
    }

}
