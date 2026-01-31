#include <Wire.h>
#include <Adafruit_AHTX0.h>

Adafruit_AHTX0 b;
Adafruit_AHTX0 a;
void setup() 
{
  Serial.begin(115200);
  wire.begin(21, 22);
  if (b.begin() && a.begin() 
  {
    Serial.println("Failed to find AHT10 / AHT20 chip");
    for (;;) 
    {
      delay(10);
    }

  }
  Serial.println("AHT10 / AHT20 Found!");
}
void loop() {
  sensors_event_t humidity, temp;
  aht.getEvent(&humidity, &temp);
  Serial.print("Temperature:");
  Serial.println( );
  Serial.println("degrees C");
  Serial.print("Humidity:");
  Serial.print(humidity.relative_humidity);
  Serial.println("% rH");
  delay(5000);
}