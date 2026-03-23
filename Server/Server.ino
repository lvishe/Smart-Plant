#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <DNSServer.h>
#include <ESPmDNS.h>
#include <Preferences.h>
#include "LServer.h"

Lserver server;
AsyncWebServer webServer(80);
DNSServer dnsServer;
Preferences preferences;

#define RESET_PREFERENCES_ON_BOOT false

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
}


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
    }
}