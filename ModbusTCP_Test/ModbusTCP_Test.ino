#include <SPI.h>
#include <EthernetENC.h>
#include "ModbusTCP.h"

#define CS_PIN 5

byte mac[] =
{
  0xDE,0xAD,0xBE,0xEF,0xFE,0x01
};

IPAddress espIP(192,168,0,20);
IPAddress subnet(255,255,255,0);

IPAddress dnsIP(0,0,0,0);
IPAddress gatewayIP(0,0,0,0);

IPAddress plcIP(192,168,0,3);

ModbusTCP plc;

void setup()
{
    Serial.begin(115200);

    SPI.begin(18,19,23,CS_PIN);

    Ethernet.init(CS_PIN);

    Ethernet.begin(mac,espIP,dnsIP,gatewayIP,subnet);

    Serial.print("ESP IP : ");
    Serial.println(Ethernet.localIP());

    if(plc.connect(plcIP))
    {
        Serial.println("PLC Baglandi");
    }
    else
    {
        Serial.println("Baglanti Hatasi");
    }
}

void loop()
{
    uint16_t regs[6];

    if(plc.readRegisters(0, regs, 6))
    {
        for(int i=0;i<6;i++)
        {
            Serial.print("REG");
            Serial.print(i);
            Serial.print(" = ");

            Serial.println(regs[i]);
        }
    }

    delay(2000);
}