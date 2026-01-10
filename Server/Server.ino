
#include "Communicator.h"


void setup()
{
  Lserver comm;

  Serial.begin(115200);
  comm.startAP();
  
  String msg;
  for(;;)
  {
    if(comm.acceptClient())
    {
      for(;;)
      {
          
          if (comm.receive(msg))
          {
              Serial.println("Received: " + msg);
          }
          delay(1); // allow WiFi stack to run
      }
    }
    
    delay(1); // allow WiFi stack to run
  }

}


void loop()
{

}
