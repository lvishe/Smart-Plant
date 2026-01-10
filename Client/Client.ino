
#include "Communicator.h"



 void setup()
{
  Lclient comm;
  Serial.begin(115200);
  comm.startSTA();
 
  // Example static sensor data for 5 sensors
  float sensorData[5] = {23.5, 50.2, 101.3, 12.7, 75.0};

  // Build comma-separated string
  String message = "";
  for (int i = 0; i < 5; i++)
  {
      message += String(sensorData[i], 2); // 2 decimals
      if (i < 4) message += ",";           // comma separator
  }

  for(;;)
  {
    for (int i = 0; i < 10; i++)
    {
      comm.send(message);
      Serial.print("Sent: ");
      Serial.println(message);
      delay(500); 
    }

    delay(2000);
  }

}



void loop() {}