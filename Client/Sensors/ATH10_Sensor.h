#include <Wire.h>
#include "Sensor.h"

// --- AHT10 SENSOR (I2C - NO LIBRARY) ---
enum class AHTMode { TEMPERATURE, HUMIDITY };

class AHT10Sensor : public Sensor {
    private:
        static bool _initialized;
        const uint8_t _addr = 0x38;
        AHTMode _mode;

        bool _triggerAndRead(uint32_t &rawH, uint32_t &rawT) {
            Wire.beginTransmission(_addr);
            Wire.write(0xAC); Wire.write(0x33); Wire.write(0x00);
            if (Wire.endTransmission() != 0) return false;

            // Wait for completion (Status Bit 7) with 200ms timeout
            uint32_t start = millis();
            while (true) {
                if (millis() - start > 200) return false;
                Wire.requestFrom(_addr, (uint8_t)1);
                if (Wire.available() && !(Wire.read() & 0x80)) break;
                delay(10);
            }

            Wire.requestFrom(_addr, (uint8_t)6);
            if (Wire.available() == 6) {
                Wire.read(); // Status
                uint32_t h1 = Wire.read(), h2 = Wire.read(), h3 = Wire.read();
                uint32_t t1 = Wire.read(), t2 = Wire.read();
                rawH = (h1 << 12) | (h2 << 4) | (h3 >> 4);
                rawT = ((h3 & 0x0F) << 16) | (t1 << 8) | t2;
                return true;
            }
            return false;
        }

    public:
        AHT10Sensor(AHTMode mode) : 
            Sensor(mode == AHTMode::TEMPERATURE ? "AirTemp" : "AirHum", -1), 
            _mode(mode) {
            if (!_initialized) {
                Wire.begin();
                Wire.beginTransmission(_addr);
                Wire.write(0xE1); Wire.write(0x08); Wire.write(0x00);
                if (Wire.endTransmission() == 0) _initialized = true;
            }
        }

        float read_value() override {
            if (!_initialized) return -999.0;
            uint32_t rH, rT;
            if (!_triggerAndRead(rH, rT)) return -999.0;
            
            return (_mode == AHTMode::TEMPERATURE) 
                ? ((float)rT / 1048576.0) * 200.0 - 50.0 
                : ((float)rH / 1048576.0) * 100.0;
        }
};
bool AHT10Sensor::_initialized = false;