#include "LClient.h"

void Lclient::startSTA() {
    WiFi.begin("ESP32_AP", "12345678");
    while (WiFi.status() != WL_CONNECTED) delay(500);
    _client.connect("192.168.4.1", 8080);
}

bool Lclient::sendPacket(String& data, Opcode code)
{
    if (!_client.connected()) 
    {
        return false;
    }
    
    // 1. Prepare Header
    Message header;
    header.code = code;
    header.data_lenght = data.length();

    // 2. Create a single buffer for Header + Payload
    int totalSize = sizeof(Message) + header.data_lenght;
    uint8_t* fullPacket = new uint8_t[totalSize];

    // 3. Copy Header into the start of the buffer
    memcpy(fullPacket, &header, sizeof(Message));

    // 4. Copy String data into the buffer right after the header
    memcpy(fullPacket + sizeof(Message), data.c_str(), header.data_lenght);

    // 5. SEND 
    size_t sent = _client.write(fullPacket, totalSize);

    delete[] fullPacket; // Free memory
    
    return sent == totalSize;
}

bool Lclient::receivePacket(Opcode &codeOut, String &strOut, int &intOut)
{
    if (!_client.connected() || _client.available() < sizeof(Message)) 
    {
        return false;
    }

    // Read Header
    Message header;
    _client.read((uint8_t*)&header, sizeof(Message));
    codeOut = header.code;

    // Read Payload
    if (header.code == Opcode::UPDATE && header.data_lenght == sizeof(int))
    {
        _client.read((uint8_t*)&intOut, sizeof(int));
    }
    else 
    {
        char* buf = new char[header.data_lenght + 1];
        _client.read((uint8_t*)buf, header.data_lenght);
        buf[header.data_lenght] = '\0';
        strOut = String(buf);
        delete[] buf;
    }
    return true;
}