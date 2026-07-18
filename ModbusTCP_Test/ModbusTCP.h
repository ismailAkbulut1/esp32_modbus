#ifndef MODBUSTCP_H
#define MODBUSTCP_H

#include <Arduino.h>
#include <EthernetENC.h>

class ModbusTCP
{
private:

    EthernetClient client;

    uint16_t transactionID;

    uint8_t unitID;

    uint16_t timeout;

    uint8_t txBuffer[260];
    uint8_t rxBuffer[260];

public:

    ModbusTCP();

    bool connect(IPAddress ip,uint16_t port=502);

    void disconnect();

    uint16_t readWord(uint16_t address);

    bool writeWord(uint16_t address,uint16_t value);

    bool writeRegisters(uint16_t address, uint16_t *values, uint16_t count);

    float readFloat(uint16_t address);

    bool writeFloat(uint16_t address,float value);

    bool readRegisters(uint16_t address, uint16_t *values, uint16_t count);

    bool readCoil(uint16_t address);

    bool writeCoil(uint16_t address,bool state);
};

#endif