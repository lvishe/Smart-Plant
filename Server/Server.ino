#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <DNSServer.h>
#include <ESPmDNS.h>
#include <Preferences.h>
<<<<<<< HEAD
#include <vector>
#include "LServer.h"
#include "html_pages.h"
=======
#include "LServer.h"
>>>>>>> 6aadd463b6d1fff5c24b0226862f0d128d1ccd35

Lserver server;
AsyncWebServer webServer(80);
DNSServer dnsServer;
Preferences preferences;

#define RESET_PREFERENCES_ON_BOOT false
<<<<<<< HEAD
#define MAX_HISTORY 20

// Cooldown timer to prevent serial spam
unsigned long lastAlarmTime = 0;
const unsigned long ALARM_COOLDOWN = 15000; 

struct SensorStream {
    String name;
    float values[MAX_HISTORY];
    bool hasMax = false;
    float maxThreshold = 0.0f;
    
    SensorStream(String n) {
        name = n;
        for (int i = 0; i < MAX_HISTORY; i++) values[i] = -999.0f;
    }
};

struct DeviceStream {
    String id;
    String ip;
    int updateDelay = 5000; // Track delay per client
    std::vector<SensorStream> sensors;
    String timeHistory[MAX_HISTORY];
    int historyIndex = 0;
    int historyCount = 0;
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
String makePrefKey(const String& devId, const String& sensorName) {
    String key = "m_" + devId + "_" + sensorName;
    if (key.length() > 15) key = key.substring(0, 15);
    return key;
}

bool addSensorDataToDevice(DeviceStream& dev, const String& name, float value) {
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

    if (dev.sensors[foundIdx].hasMax && value > dev.sensors[foundIdx].maxThreshold) {
        return true; // Threshold breached
    }
    return false;
=======

// Global Variables
int nextDelay = 5000;
String lastReceivedData = "Waiting...";
String lastClientIP = "None";
String ssid = "";
String pass = "";

const char setup_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>Wi-Fi Configuration</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        * { box-sizing: border-box; }

        body {
            margin: 0;
            font-family: 'Segoe UI', sans-serif;
            background: linear-gradient(135deg, #1f4037, #99f2c8);
            display: flex;
            justify-content: center;
            align-items: center;
            min-height: 100vh;
        }

        .card {
            background: white;
            padding: 30px;
            width: 90%;
            max-width: 380px;
            border-radius: 20px;
            box-shadow: 0 10px 40px rgba(0,0,0,0.3);
            text-align: center;
        }

        h2 {
            margin-top: 0;
            color: #1f4037;
        }

        input {
            width: 100%;
            padding: 12px;
            margin: 10px 0;
            border-radius: 10px;
            border: 1px solid #ccc;
            font-size: 1em;
        }

        button {
            width: 100%;
            padding: 12px;
            border-radius: 10px;
            border: none;
            background: linear-gradient(90deg, #00c6ff, #0072ff);
            color: white;
            font-weight: bold;
            font-size: 1em;
            cursor: pointer;
            transition: 0.3s;
        }

        button:hover {
            opacity: 0.85;
        }

        .back {
            display: block;
            margin-top: 15px;
            font-size: 0.85em;
            color: #555;
            text-decoration: none;
        }

    </style>
</head>
<body>

<div class="card">
    <h2>Change Wi-Fi</h2>

    <form action="/save" method="get">
        <input type="text" name="ssid" placeholder="Wi-Fi Name (SSID)" required>
        <input type="password" name="pass" placeholder="Password">
        <button type="submit">Save & Reconnect</button>
    </form>

    <a class="back" href="/">Back to Dashboard</a>
</div>

</body>
</html>
)rawliteral";

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>Smart Plant Dashboard</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        * { box-sizing: border-box; }

        body {
            margin: 0;
            font-family: Arial, sans-serif;
            background: linear-gradient(135deg, #0f2027, #2c5364);
            color: white;
            text-align: center;
        }

        .container {
            width: 95%;
            max-width: 480px;
            margin: auto;
            padding-top: 30px;
        }

        .card {
            background: rgba(255,255,255,0.1);
            padding: 20px;
            border-radius: 15px;
            margin-bottom: 20px;
        }

        .status {
            font-weight: bold;
            font-size: 18px;
            margin-top: 5px;
        }

        .data-value {
            font-size: 22px;
            font-weight: bold;
            color: #00ffcc;
        }

        input {
            width: 100%;
            padding: 10px;
            border-radius: 8px;
            border: none;
            margin-bottom: 10px;
        }

        button {
            width: 100%;
            padding: 10px;
            border-radius: 8px;
            border: none;
            background: #00c6ff;
            color: white;
            font-weight: bold;
        }

        a {
            color: #00ffcc;
            text-decoration: none;
            font-size: 14px;
        }

    </style>
</head>
<body>

<div class="container">

    <div class="card">
        <h2>System Status</h2>
        <div class="status">%WIFI_STATUS%</div>
        <div>IP: %STA_IP%</div>
    </div>

    <div class="card">
        <h3>Live Monitor</h3>
        <div class="data-value" id="data-val">%LAST_DATA%</div>
        <div>From: <span id="ip-val">%LAST_IP%</span></div>
    </div>

    <div class="card">
        <h3>Update Interval (ms)</h3>
        <form action="/update">
            <input type="number" name="delay" value="%DELAY_VAL%">
            <button type="submit">Update Delay</button>
        </form>
    </div>

    <a href="/setup">Configure Wi-Fi</a>

</div>

<script>
setInterval(function()
{
    fetch('/status')
    .then(r => r.json())
    .then(data =>
    {
        document.getElementById('data-val').innerText = data.lastData;
        document.getElementById('ip-val').innerText = data.lastIP;
    });
}, 2000);
</script>

</body>
</html>
)rawliteral";
// ==========================
// SETUP
// ==========================
void setup()
{
    Serial.begin(115200);
    delay(1000);

    preferences.begin("wifi-gate", false);

    if (RESET_PREFERENCES_ON_BOOT)
    {
        Serial.println("DEBUG: Clearing all preferences from flash...");
        preferences.clear();
    }

    ssid = preferences.getString("ssid", "");
    pass = preferences.getString("pass", "");
    nextDelay = preferences.getInt("delay", 5000);

    WiFi.mode(WIFI_AP_STA);
    server.startAP();

    dnsServer.start(53, "*", WiFi.softAPIP());

    if (ssid != "")
    {
        Serial.print("Connecting to: ");
        Serial.println(ssid);

        WiFi.begin(ssid.c_str(), pass.c_str());

        unsigned long startAttempt = millis();

        while (WiFi.status() != WL_CONNECTED &&
               millis() - startAttempt < 12000)
        {
            delay(500);
            Serial.print(".");
        }

        if (WiFi.status() == WL_CONNECTED)
        {
            Serial.println("\n--- WIFI CONNECTED ---");
            Serial.print("DASHBOARD IP: ");
            Serial.println(WiFi.localIP());

            if (MDNS.begin("plant"))
            {
                Serial.println("mDNS started: http://plant.local");
            }
        }
        else
        {
            Serial.println("\n--- WIFI FAILED (AP mode only) ---");
        }
    }

    // ==========================
    // ROUTES
    // ==========================
   webServer.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
{
    String page = index_html;

    page.replace("%WIFI_STATUS%", 
        (WiFi.status() == WL_CONNECTED) ? "CONNECTED" : "DISCONNECTED");

    page.replace("%STA_IP%", 
        (WiFi.status() == WL_CONNECTED) 
        ? WiFi.localIP().toString() 
        : "192.168.4.1");

    page.replace("%DELAY_VAL%", String(nextDelay));
    page.replace("%LAST_IP%", lastClientIP);
    page.replace("%LAST_DATA%", lastReceivedData);

    request->send(200, "text/html", page);
});

    webServer.on("/setup", HTTP_GET,
    [](AsyncWebServerRequest *request)
    {
        request->send_P(200, "text/html", setup_html);
    });

    webServer.on("/status", HTTP_GET,
    [](AsyncWebServerRequest *request)
    {
        String json =
            "{\"lastData\":\"" + lastReceivedData +
            "\", \"lastIP\":\"" + lastClientIP + "\"}";

        request->send(200, "application/json", json);
    });

    webServer.on("/update", HTTP_GET,
    [](AsyncWebServerRequest *request)
    {
        if (request->hasParam("delay"))
        {
            nextDelay = request->getParam("delay")->value().toInt();
            preferences.putInt("delay", nextDelay);
        }

        request->redirect("/");
    });

    webServer.on("/save", HTTP_GET,
    [](AsyncWebServerRequest *request)
    {
        if (request->hasParam("ssid"))
        {
            preferences.putString("ssid",
                request->getParam("ssid")->value());

            preferences.putString("pass",
                request->hasParam("pass")
                ? request->getParam("pass")->value()
                : "");

            request->send(200, "text/plain",
                "Settings saved. Rebooting...");

            delay(2000);
            ESP.restart();
        }
    });

    webServer.on("/generate_204", HTTP_GET,
    [](AsyncWebServerRequest *request)
    {
        request->redirect("/setup");
    });

    webServer.on("/fwlink", HTTP_GET,
    [](AsyncWebServerRequest *request)
    {
        request->redirect("/setup");
    });

    webServer.onNotFound(
    [](AsyncWebServerRequest *request)
    {
        if (request->client()->localIP() == WiFi.softAPIP())
        {
            request->redirect("/setup");
        }
        else
        {
            request->send(404, "text/plain", "Not Found");
        }
    });

    webServer.begin();
>>>>>>> 6aadd463b6d1fff5c24b0226862f0d128d1ccd35
}

void processIncomingMessage(const ClientMessage& msg, const String& ip) {
    String devId = String(msg.device_id);
    
    int devIdx = -1;
    for (int i = 0; i < activeDevices.size(); i++) {
        if (activeDevices[i].id == devId) {
            devIdx = i; break;
        }
    }
    if (devIdx == -1) {
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
    
    for (auto& s : dev.sensors) {
        s.values[dev.historyIndex] = -999.0f;
    }

<<<<<<< HEAD
    bool triggerAlarm = false;
    triggerAlarm |= addSensorDataToDevice(dev, "Temp", msg.temp);
    triggerAlarm |= addSensorDataToDevice(dev, "Humidity", msg.humidity);
    triggerAlarm |= addSensorDataToDevice(dev, "Moisture", (float)msg.moisture);
    triggerAlarm |= addSensorDataToDevice(dev, "pH", msg.ph_level);
    triggerAlarm |= addSensorDataToDevice(dev, "TDS", (float)msg.tds_value);

    // --- REPLACED AUDIO WITH SERIAL PRINT ---
    if (triggerAlarm) {
        if (millis() - lastAlarmTime > ALARM_COOLDOWN) {
            Serial.println("**************************************************");
            Serial.printf("ALARM! Device %s exceeded a sensor threshold!\n", dev.id.c_str());
            Serial.println("**************************************************");
            lastAlarmTime = millis();
        }
    }
    // ----------------------------------------

    dev.timeHistory[dev.historyIndex] = String(dev.historyCount + 1);
    dev.historyIndex = (dev.historyIndex + 1) % MAX_HISTORY;
    if (dev.historyCount < MAX_HISTORY) dev.historyCount++;

    lastReceivedData = "ID:" + devId + " T:" + String(msg.temp, 1) + " pH:" + String(msg.ph_level, 1);
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
    server.startAP();
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

void loop() {
    dnsServer.processNextRequest();
    
    if (ssid != "" && WiFi.status() != WL_CONNECTED && millis() - lastWifiCheck > 10000) {
        Serial.println("Connection lost. Reconnecting to WiFi...");
        WiFi.disconnect();
        WiFi.begin(ssid.c_str(), pass.c_str());
        lastWifiCheck = millis();
    }

    if (pendingSave) {
        pendingSave = false;
        preferences.putString("ssid", pendingSsid);
        preferences.putString("pass", pendingPass);
        delay(2000);
        ESP.restart();
    }

    if (millis() - lastIndexRebuild > 10000) {
        rebuildIndex();
        lastIndexRebuild = millis();
    }

    if (server.acceptClient()) Serial.println("LServer: Client connected");

    ClientMessage incomingMsg;
    String remoteIP;
    
    if (server.receivePacket(incomingMsg, remoteIP)) {
        lastClientIP = remoteIP;
        processIncomingMessage(incomingMsg, remoteIP); 
        rebuildCaches();
        
        // Find specific delay for the device that just sent data
        int currentDevDelay = 5000;
        for (auto& dev : activeDevices) {
            if (dev.id == String(incomingMsg.device_id)) {
                currentDevDelay = dev.updateDelay;
                break;
            }
        }
        
        server.sendUpdate(currentDevDelay); 
        Serial.printf("Received from %s: ID=%u, Temp=%.1f\n", remoteIP.c_str(), incomingMsg.device_id, incomingMsg.temp);
=======
// ==========================
// LOOP
// ==========================
void loop()
{
    dnsServer.processNextRequest();

    if (server.acceptClient())
    {
        Serial.println("LServer Connected");
    }

    Opcode code;
    String data;
    String remoteIP;

    if (server.receivePacket(code, data, remoteIP))
    {
        lastReceivedData = data;
        lastClientIP = remoteIP;
        server.sendUpdate(nextDelay);
>>>>>>> 6aadd463b6d1fff5c24b0226862f0d128d1ccd35
    }
}