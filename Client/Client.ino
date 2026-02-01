#include "Client.h"

Lclient comm;
int loopDelay = 500; // Start with 500ms

void setup()
{
  Serial.begin(115200);
  comm.startSTA();
}

void loop()
{
  // 1. Prepare Data
  float sensorData[5] = {23.5, 50.2, 101.3, 12.7, 75.0};
  String message = "";
  for (int i = 0; i < 5; i++) {
      message += String(sensorData[i], 2);
      if (i < 4) message += ",";
  }

  Serial.print("Sending: ");
  Serial.println(message);

  // 2. Perform Transaction: Send DATA -> Wait -> Receive UPDATE
  // The function parses the response internally using switch case
  loopDelay = comm.performTransaction(message, loopDelay);

  Serial.print("Server requested delay: ");
  Serial.println(loopDelay);

  // 3. Wait the requested amount
  delay(loopDelay);
}