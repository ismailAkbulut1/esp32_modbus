#include <SPI.h>
#include <EthernetENC.h>

#define CS_PIN 5

//======================================================
// Ethernet Ayarları
//======================================================

byte mac[] = {0xDE,0xAD,0xBE,0xEF,0xFE,0x01};

IPAddress espIP(192,168,0,20);
IPAddress subnet(255,255,255,0);
IPAddress dnsIP(0,0,0,0);
IPAddress gatewayIP(0,0,0,0);

IPAddress plcIP(192,168,0,3);

EthernetClient client;

//======================================================
// PLC Bağlantısı
//======================================================

bool connectPLC()
{
    Serial.println("--------------------------------");
    Serial.println("PLC'ye baglaniliyor...");

    if(client.connect(plcIP,502))
    {
        Serial.println("Baglanti Basarili");
        return true;
    }

    Serial.println("Baglanti Hatasi");
    return false;
}

//======================================================
// PLC Bağlantısını Kapat
//======================================================

void disconnectPLC()
{
    client.stop();
    Serial.println("Baglanti Kapatildi");
}

//======================================================
// Holding Register Oku (Function 03)
//======================================================

void readHoldingRegisters(uint16_t startAddress,uint16_t quantity)
{

    uint8_t request[] =
    {
        0x00,0x01,                     // Transaction ID
        0x00,0x00,                     // Protocol ID
        0x00,0x06,                     // Length

        0xFF,                          // Unit ID

        0x03,                          // Function Code

        highByte(startAddress),
        lowByte(startAddress),

        highByte(quantity),
        lowByte(quantity)
    };

    client.write(request,sizeof(request));

    Serial.println();
    Serial.println("Holding Register Okuma Istegi Gonderildi");

    Serial.println("PLC cevabi bekleniyor...");

    unsigned long t0 = millis();

    while(!client.available())
    {
        if(millis() - t0 > 3000)
        {
            Serial.println("Timeout! PLC cevap vermedi.");
            return;
        }
    }

    Serial.println("PLC cevap verdi.");

    if(client.available())
    {
        uint8_t response[64];

        int len=client.read(response,sizeof(response));

        Serial.println();
        Serial.print("Gelen Byte : ");
        Serial.println(len);

        Serial.println("Ham Veri");

        for(int i=0;i<len;i++)
        {
            if(response[i]<16) Serial.print("0");

            Serial.print(response[i],HEX);
            Serial.print(" ");
        }

        Serial.println();
        Serial.println();

        if(response[7]==0x83)
        {
            Serial.print("Modbus Hata Kodu : ");
            Serial.println(response[8],HEX);
            return;
        }

        for(int i=0;i<quantity;i++)
        {
            uint16_t value=(response[9+(i*2)]<<8)|response[10+(i*2)];

            Serial.print("HoldingReg[");
            Serial.print(i);
            Serial.print("] = ");
            Serial.println(value);
        }

        if(quantity >= 9)
        {
            // REAL veriler
            uint16_t reg0 = (response[9]  << 8) | response[10];
            uint16_t reg1 = (response[11] << 8) | response[12];

            uint16_t reg2 = (response[13] << 8) | response[14];
            uint16_t reg3 = (response[15] << 8) | response[16];

            uint16_t reg4 = (response[17] << 8) | response[18];
            uint16_t reg5 = (response[19] << 8) | response[20];

            float temp = registersToFloat(reg0, reg1);
            float pressure = registersToFloat(reg2, reg3);
            float flow = registersToFloat(reg4, reg5);

            // WORD veriler
            uint16_t test0 = (response[21] << 8) | response[22];
            uint16_t test1 = (response[23] << 8) | response[24];
            uint16_t test2 = (response[25] << 8) | response[26];

            Serial.println();
            Serial.println("===== PROCESS DATA =====");

            Serial.print("Temp     : ");
            Serial.println(temp);

            Serial.print("Pressure : ");
            Serial.println(pressure);

            Serial.print("Flow     : ");
            Serial.println(flow);

            Serial.print("Test0    : ");
            Serial.println(test0);

            Serial.print("Test1    : ");
            Serial.println(test1);

            Serial.print("Test2    : ");
            Serial.println(test2);

            Serial.println("========================");
        }

    }
}


//=====================================================
// Function 04
// Read Input Registers
//=====================================================

void readInputRegisters(uint16_t startAddress, uint16_t quantity)
{
    uint8_t request[12];

    request[0]=0x00;
    request[1]=0x02;      // Transaction ID

    request[2]=0x00;
    request[3]=0x00;      // Protocol

    request[4]=0x00;
    request[5]=0x06;      // Length

    request[6]=0xFF;      // Unit ID

    request[7]=0x04;      // Function Code

    request[8]=highByte(startAddress);
    request[9]=lowByte(startAddress);

    request[10]=highByte(quantity);
    request[11]=lowByte(quantity);

    client.write(request,12);

    Serial.println();
    Serial.println("Function 04 Gonderildi");

    delay(100);

    if(client.available())
    {
        uint8_t response[64];

        int len=client.read(response,sizeof(response));

        Serial.print("Gelen Byte Sayisi : ");
        Serial.println(len);

        Serial.println("Input Registerlar");

        for(int i=0;i<quantity;i++)
        {
            uint16_t value=(response[9+i*2]<<8)|response[10+i*2];

            Serial.print("IR");
            Serial.print(startAddress+i);
            Serial.print(" = ");

            Serial.println(value);
        }
    }
}




//======================================================
// Holding Register Yaz (Function 06)
//======================================================

void writeHoldingRegister(uint16_t regAddress,uint16_t value)
{

    uint8_t request[] =
    {
        0x00,0x02,
        0x00,0x00,
        0x00,0x06,

        0xFF,

        0x06,

        highByte(regAddress),
        lowByte(regAddress),

        highByte(value),
        lowByte(value)
    };

    client.write(request,sizeof(request));

    Serial.println();
    Serial.println("Holding Register Yazma Istegi");

    delay(100);

    if(client.available())
    {
        uint8_t response[32];

        int len=client.read(response,sizeof(response));

        Serial.print("Yanit : ");

        for(int i=0;i<len;i++)
        {
            if(response[i]<16) Serial.print("0");

            Serial.print(response[i],HEX);
            Serial.print(" ");
        }

        Serial.println();

        if(response[7]==0x86)
        {
            Serial.print("Yazma Hatasi : ");
            Serial.println(response[8],HEX);
            return;
        }

        Serial.println("Yazma Basarili");
    }

}

//======================================================
// Function 16
// Birden Fazla Holding Register Yaz
//======================================================

void writeMultipleRegisters(uint16_t startAddress,
                            uint16_t *values,
                            uint16_t quantity)
{

    uint8_t request[256];

    //---------------------------------
    // Transaction ID
    //---------------------------------

    request[0]=0x00;
    request[1]=0x03;

    //---------------------------------
    // Protocol
    //---------------------------------

    request[2]=0x00;
    request[3]=0x00;

    //---------------------------------
    // Length
    //---------------------------------

    uint16_t length=7+(quantity*2);

    request[4]=highByte(length);
    request[5]=lowByte(length);

    //---------------------------------

    request[6]=0xFF;

    request[7]=0x10;

    //---------------------------------

    request[8]=highByte(startAddress);
    request[9]=lowByte(startAddress);

    //---------------------------------

    request[10]=highByte(quantity);
    request[11]=lowByte(quantity);

    //---------------------------------

    request[12]=quantity*2;

    //---------------------------------

    int index=13;

    for(int i=0;i<quantity;i++)
    {
        request[index++]=highByte(values[i]);
        request[index++]=lowByte(values[i]);
    }

    client.write(request,index);

    Serial.println();
    Serial.println("Function 16 Gonderildi");

    delay(100);

    if(client.available())
    {
        uint8_t response[32];

        int len=client.read(response,sizeof(response));

        Serial.print("PLC Cevabi : ");

        for(int i=0;i<len;i++)
        {
            if(response[i]<16) Serial.print("0");

            Serial.print(response[i],HEX);
            Serial.print(" ");
        }

        Serial.println();
        Serial.println("Yazma Tamamlandi");
    }

}


float registersToFloat(uint16_t reg1,uint16_t reg2)
{
    union
    {
        float f;
        uint8_t b[4];
    } data;

    data.b[3]=highByte(reg1);
    data.b[2]=lowByte(reg1);

    data.b[1]=highByte(reg2);
    data.b[0]=lowByte(reg2);

    return data.f;
}


//======================================================
// SETUP
//======================================================

void setup()
{

    Serial.begin(115200);

    SPI.begin(18,19,23,CS_PIN);

    Ethernet.init(CS_PIN);

    Ethernet.begin(mac,espIP,dnsIP,gatewayIP,subnet);

    delay(1000);

    Serial.print("ESP IP : ");
    Serial.println(Ethernet.localIP());

}

//======================================================
// LOOP
//======================================================

void loop()
{

    if(connectPLC())
    {

        //-------------------------------
        // Holding Register Oku
        //-------------------------------

        readHoldingRegisters(0,9);

        delay(1000);

        //-------------------------------
        // Holding Register Yaz
        //-------------------------------

        /*uint16_t values[3]={111,222,333};

        writeMultipleRegisters(0,values,3);

        disconnectPLC();
        */

    }

    while(1);

}
