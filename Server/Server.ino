#include "Server.h"

Lserver comm;

void setup()
{
  Serial.begin(115200);
  comm.startAP();
}

void loop()
{
  // Check for new connections
  comm.acceptClient();

  Opcode op;
  String msg;

  // Process incoming messages
  if (comm.receive(op, msg))
  {
      if (op == Opcode::DATA)
      {
          Serial.print("Received DATA: ");
          Serial.println(msg);

          if (msg.indexOf("101.3") >= 0) 
          {
             Serial.println("High value detected, sending UPDATE to slow down client...");
             comm.sendUpdate(2000); // Set delay to 2000ms
          }
      }
  }
}