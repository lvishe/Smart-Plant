#include "LClient.h"

void Lclient::startSTA() {
<<<<<<< HEAD
    WiFi.begin("ESP32_AP");
=======
    WiFi.begin("ESP32_AP", "12345678");
>>>>>>> 6aadd463b6d1fff5c24b0226862f0d128d1ccd35
    while (WiFi.status() != WL_CONNECTED) delay(500);
    _client.connect("192.168.4.1", 8080);
}

<<<<<<< HEAD
bool Lclient::sendPacket(ClientMessage &msg)
=======
bool Lclient::sendPacket(String& data, Opcode code)
>>>>>>> 6aadd463b6d1fff5c24b0226862f0d128d1ccd35
{
    if (!_client.connected()) 
    {
        return false;
    }
    
<<<<<<< HEAD
    // No dynamic allocation (new/delete) required anymore!
    // Send the memory block containing the struct directly.
    size_t sent = _client.write((uint8_t*)&msg, sizeof(ClientMessage));
    
    return sent == sizeof(ClientMessage);
}

bool Lclient::receivePacket(ServerMessage &msgOut)
{
    // Wait until a full ServerMessage has arrived
    if (!_client.connected() || _client.available() < sizeof(ServerMessage)) 
=======
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
>>>>>>> 6aadd463b6d1fff5c24b0226862f0d128d1ccd35
    {
        return false;
    }

<<<<<<< HEAD
    // Read the payload directly into the struct
    size_t bytesRead = _client.readBytes((char*)&msgOut, sizeof(ServerMessage));
    
    return bytesRead == sizeof(ServerMessage);
=======
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
>>>>>>> 6aadd463b6d1fff5c24b0226862f0d128d1ccd35
}