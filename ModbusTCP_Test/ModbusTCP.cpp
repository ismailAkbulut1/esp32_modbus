#include "ModbusTCP.h"

ModbusTCP::ModbusTCP()
{
    transactionID = 1;
    unitID = 0xFF;
    timeout = 1000;
}

bool ModbusTCP::connect(IPAddress ip,uint16_t port)
{
    return client.connect(ip,port);
}

void ModbusTCP::disconnect()
{
    client.stop();
}

bool ModbusTCP::writeRegisters(uint16_t address, uint16_t *values, uint16_t count)
{
    // Modbus'ta maksimum 123 register tek pakette yazılabilir.
    if(count == 0 || count > 123)
        return false;

    // MBAP Header
    txBuffer[0] = highByte(transactionID);
    txBuffer[1] = lowByte(transactionID);

    txBuffer[2] = 0x00;
    txBuffer[3] = 0x00;

    // Length = UnitID + Function + Address(2) + Count(2) + ByteCount(1) + Data
    uint16_t length = 7 + count * 2;

    txBuffer[4] = highByte(length);
    txBuffer[5] = lowByte(length);

    txBuffer[6] = unitID;
    txBuffer[7] = 0x10;      // FC16

    txBuffer[8] = highByte(address);
    txBuffer[9] = lowByte(address);

    txBuffer[10] = highByte(count);
    txBuffer[11] = lowByte(count);

    txBuffer[12] = count * 2;

    int index = 13;

    for(int i = 0; i < count; i++)
    {
        txBuffer[index++] = highByte(values[i]);
        txBuffer[index++] = lowByte(values[i]);
    }

    client.write(txBuffer, index);

    unsigned long startTime = millis();

    while(!client.available())
    {
        if(millis() - startTime > timeout)
        {
            Serial.println("Timeout");
            return false;
        }
    }

    int len = client.read(rxBuffer, sizeof(rxBuffer));

    if(len < 12)
        return false;

    if(rxBuffer[7] == 0x90)
    {
        Serial.print("Modbus Error : ");
        Serial.println(rxBuffer[8], HEX);
        return false;
    }

    transactionID++;

    return true;
}

uint16_t ModbusTCP::readWord(uint16_t address)
{
    // Modbus TCP Read Holding Register (FC03)

    uint8_t request[] =
    {
        highByte(transactionID),
        lowByte(transactionID),

        0x00,0x00,

        0x00,0x06,

        unitID,

        0x03,

        highByte(address),
        lowByte(address),

        0x00,0x01
    };

    client.write(request,sizeof(request));

    unsigned long startTime = millis();

    while(!client.available())
    {
        if(millis()-startTime>timeout)
        {
            Serial.println("Timeout");
            return 0;
        }
    }

    int len = client.read(rxBuffer,sizeof(rxBuffer));

    if(len<11)
    {
        Serial.println("Eksik Paket");
        return 0;
    }

    if(rxBuffer[7]==0x83)
    {
        Serial.print("Modbus Error : ");
        Serial.println(rxBuffer[8],HEX);
        return 0;
    }

    transactionID++;

    return (rxBuffer[9]<<8)|rxBuffer[10];
}

float ModbusTCP::readFloat(uint16_t address)
{
    uint8_t request[] =
    {
        highByte(transactionID),
        lowByte(transactionID),

        0x00,0x00,

        0x00,0x06,

        unitID,

        0x03,

        highByte(address),
        lowByte(address),

        0x00,0x02      // 2 Register = 1 Float
    };

    client.write(request,sizeof(request));

    unsigned long startTime = millis();

    while(!client.available())
    {
        if(millis()-startTime>timeout)
        {
            Serial.println("Timeout");
            return 0;
        }
    }

    int len = client.read(rxBuffer,sizeof(rxBuffer));

    if(len<13)
    {
        Serial.println("Eksik Paket");
        return 0;
    }

    if(rxBuffer[7]==0x83)
    {
        Serial.print("Modbus Error : ");
        Serial.println(rxBuffer[8],HEX);
        return 0;
    }

    union
    {
        float value;
        uint8_t bytes[4];
    } data;

    data.bytes[3]=rxBuffer[9];
    data.bytes[2]=rxBuffer[10];
    data.bytes[1]=rxBuffer[11];
    data.bytes[0]=rxBuffer[12];

    transactionID++;

    return data.value;
}

bool ModbusTCP::writeWord(uint16_t address,uint16_t value)
{
    uint8_t request[] =
    {
        highByte(transactionID),
        lowByte(transactionID),

        0x00,0x00,

        0x00,0x06,

        unitID,

        0x06,

        highByte(address),
        lowByte(address),

        highByte(value),
        lowByte(value)
    };

    client.write(request,sizeof(request));

    unsigned long startTime = millis();

    while(!client.available())
    {
        if(millis()-startTime>timeout)
        {
            Serial.println("Timeout");
            return false;
        }
    }

    int len = client.read(rxBuffer,sizeof(rxBuffer));

    if(len < 12)
    {
        return false;
    }

    if(rxBuffer[7]==0x86)
    {
        Serial.print("Modbus Error : ");
        Serial.println(rxBuffer[8],HEX);
        return false;
    }

    transactionID++;

    return true;
}

bool ModbusTCP::writeFloat(uint16_t address,float value)
{
    union
    {
        float f;
        uint16_t reg[2];
    } data;

    data.f = value;

    uint16_t values[2];

    values[0] = (data.reg[1]);
    values[1] = (data.reg[0]);

    return writeRegisters(address, values, 2);
}

bool ModbusTCP::readRegisters(uint16_t address, uint16_t *values, uint16_t count)
{
    if(count == 0 || count > 125)
        return false;

    txBuffer[0] = highByte(transactionID);
    txBuffer[1] = lowByte(transactionID);

    txBuffer[2] = 0x00;
    txBuffer[3] = 0x00;

    txBuffer[4] = 0x00;
    txBuffer[5] = 0x06;

    txBuffer[6] = unitID;

    txBuffer[7] = 0x03;        // FC03

    txBuffer[8] = highByte(address);
    txBuffer[9] = lowByte(address);

    txBuffer[10] = highByte(count);
    txBuffer[11] = lowByte(count);

    client.write(txBuffer,12);

    unsigned long startTime = millis();

    while(!client.available())
    {
        if(millis()-startTime>timeout)
        {
            Serial.println("Timeout");
            return false;
        }
    }

    int len = client.read(rxBuffer,sizeof(rxBuffer));

    if(len < (9 + count*2))
        return false;

    if(rxBuffer[7] == 0x83)
    {
        Serial.print("Modbus Error : ");
        Serial.println(rxBuffer[8],HEX);
        return false;
    }

    for(int i=0;i<count;i++)
    {
        values[i] =
        (
            (rxBuffer[9+i*2]<<8)
            |
            rxBuffer[10+i*2]
        );
    }

    transactionID++;

    return true;
}

bool ModbusTCP::readCoil(uint16_t address)
{
    txBuffer[0]=highByte(transactionID);
    txBuffer[1]=lowByte(transactionID);

    txBuffer[2]=0x00;
    txBuffer[3]=0x00;

    txBuffer[4]=0x00;
    txBuffer[5]=0x06;

    txBuffer[6]=unitID;
    txBuffer[7]=0x01;      // FC01

    txBuffer[8]=highByte(address);
    txBuffer[9]=lowByte(address);

    txBuffer[10]=0x00;
    txBuffer[11]=0x01;

    client.write(txBuffer,12);

    unsigned long start=millis();

    while(!client.available())
    {
        if(millis()-start>timeout)
            return false;
    }

    int len=client.read(rxBuffer,sizeof(rxBuffer));

    if(len<10)
        return false;

    if(rxBuffer[7]==0x81)
        return false;

    transactionID++;

    return (rxBuffer[9]&0x01);
}

bool ModbusTCP::writeCoil(uint16_t address,bool state)
{
    txBuffer[0]=highByte(transactionID);
    txBuffer[1]=lowByte(transactionID);

    txBuffer[2]=0x00;
    txBuffer[3]=0x00;

    txBuffer[4]=0x00;
    txBuffer[5]=0x06;

    txBuffer[6]=unitID;
    txBuffer[7]=0x05;      // FC05

    txBuffer[8]=highByte(address);
    txBuffer[9]=lowByte(address);

    if(state)
    {
        txBuffer[10]=0xFF;
        txBuffer[11]=0x00;
    }
    else
    {
        txBuffer[10]=0x00;
        txBuffer[11]=0x00;
    }

    client.write(txBuffer,12);

    unsigned long start=millis();

    while(!client.available())
    {
        if(millis()-start>timeout)
            return false;
    }

    int len=client.read(rxBuffer,sizeof(rxBuffer));

    if(len<12)
        return false;

    if(rxBuffer[7]==0x85)
        return false;

    transactionID++;

    return true;
}
