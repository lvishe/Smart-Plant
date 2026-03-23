#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <FS.h> // Often needed for serving files like HTML/JS

// Network credentials
const char* ssid = "Maalot-student";
const char* password = "";

// Create an AsyncWebServer object on port 80
AsyncWebServer server(80);

void setup(){
  Serial.begin(115200);
  WiFi.begin(ssid);

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected to WiFi. IP address: ");
  Serial.println(WiFi.localIP());

  // Define an asynchronous handler for the root path ("/")
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/plain", "Hello from ESP32 Async Web Server!");
  });

  // Example of serving a static HTML page (requires File System like SPIFFS)
  // server.on("/site", HTTP_GET, [](AsyncWebServerRequest *request){
  //   request->send(SPIFFS, "/index.html", "text/html");
  // });

  // Start the server
  server.begin();
  Serial.println("Async HTTP server started");
}

void loop() {
  // The asynchronous server runs in its own task, so the main loop remains free
  // for other tasks. Do not use 'delay()' or 'yield()' in server callbacks.
}
