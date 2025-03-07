/*
    MODBUS RTU - AS A SLAVE DEVICE
*/

#define MODBUS_SLAVE_ID 0x01
#define MODBUS_FUNCTION_READ_HOLDING_REGISTERS 0x03
#define MODBUS_FUNCTION_WRITE_HOLDING_REGISTERS 0x10
#define MODBUS_REGISTER_COUNT 4  // Registers to be written by Master for FC-16
#define MODBUS_BAUD_RATE 9600

#include <RF24.h>
#include <RF24Network.h>
#include <SPI.h>
#include <WiFi.h>
#include <Ethernet.h>

struct payload_t {  // Structure of our payload
  uint16_t LoomID = 0;
  uint16_t runtime_hrs = 0;
  uint16_t runtime_mins = 0;
  uint16_t stoppagetime_hrs = 0;
  uint16_t stoppagetime_mins = 0;
  uint16_t Weftstoppagetime_hrs = 0;
  uint16_t Weftstoppagetime_mins = 0;
  uint16_t Othersstoppagetime_hrs = 0;
  uint16_t Othersstoppagetime_mins = 0;

  uint16_t remainingBeam = 0;
  uint16_t Shiftproduction = 0;
  // int Totalproduction = 0; // on desktop application
  uint16_t LoomStatus = 0;
  uint16_t LoomRpm = 0;
};

struct userData_t {  // Structure of our userData
  uint16_t beamlength = 0;
  uint16_t pick = 0;
  uint16_t resetData = 0;
  // int reed = 0;
  // int beamNo = 0;
  // int ArticleNo = 0;
  uint16_t shrinkage = 0;
};

payload_t payload;

RF24 radio(4, 5);               // nRF24L01 (CE,CSN)
RF24Network network(radio);     // Include the radio in the network
const uint16_t this_node = 00;  // Address of our node in Octal format
const uint16_t Loom1 = 01;      // Address of this node in Octal format
HardwareSerial RS485Serial(1);

uint16_t HoldingRegisters[MODBUS_REGISTER_COUNT] = { 0 };  // Array for holding registers - From Master
uint16_t buffer[sizeof(payload) / sizeof(uint16_t)] = { 0 };

// // Insert your network credentials
// const char *ssid = "ECC";
// const char *password = "123456789";

void setup() {
  pinMode(2, OUTPUT);
  digitalWrite(2, HIGH);
  Serial.begin(115200);
  RS485Serial.begin(MODBUS_BAUD_RATE, SERIAL_8N1, 16, 17);
  Serial.println("ESP32 Master Device !");

  SPI.begin();
  radio.begin();
  network.begin(90, this_node);  //(channel, node address)
  radio.setDataRate(RF24_2MBPS);

  // WiFi.mode(WIFI_STA);
  // WiFi.begin(ssid, password);
  // Serial.print("Connecting to Wi-Fi");

  // while (WiFi.status() != WL_CONNECTED) {
  //   Serial.print(".");
  //   delay(500);
  // }

  // Serial.print("Connected with IP: ");
  // Serial.println(WiFi.localIP());
  // Serial.println("Wifi Connected !!");
  // Serial.println(WiFi.macAddress());

  // Serial.println(sizeof(HoldingRegisters));
  // Serial.println(sizeof(buffer));
  // Serial.println(sizeof(uint16_t)); // 2
  // Serial.println(sizeof(payload) / sizeof(uint16_t));

  digitalWrite(2, LOW);
}

// uint8_t requestLength = 8;  // Minimum Modbus RTU frame size

void loop() {
  network.update();

  while (network.available()) {  // Is there any incoming data?
    RF24NetworkHeader header;
    network.read(header, &payload, sizeof(payload));  // Read the incoming data
    Serial.print("From Loom 1 :  ");
    printData();

    // Copy struct to uint16 array
    memcpy(buffer, &payload, sizeof(payload));
  }


  // Step 1: Read the request frame
  if (RS485Serial.available()) {
    // incomingData[len++] = RS485Serial.read();
    uint8_t incomingData[20], len = RS485Serial.readBytes(incomingData, 20);

    if (len > 0) {
      // Validate the slave ID
      if (incomingData[0] == MODBUS_SLAVE_ID) {
        // Verify CRC
        uint16_t receivedCRC = (incomingData[len - 1] << 8) | incomingData[len - 2];
        uint16_t calculatedCRC = calculateCRC(incomingData, len - 2);

        if (receivedCRC == calculatedCRC) {
          // Step 4: Handle Function Codes
          switch (incomingData[1]) {
            case 0x03:  // Read Holding Registers
              Serial.println("Read Holding Registers Request");
              processRequest03(incomingData, len);
              break;
            case 0x06:  // Write Single Register
              HoldingRegisters[MODBUS_REGISTER_COUNT] = { 0 };
              Serial.println("Write Single Holding Register");
              processRequest06(incomingData);
              sentValue();
              break;
            case 0x10:  // Write Multiple Registers
              HoldingRegisters[MODBUS_REGISTER_COUNT] = { 0 };
              Serial.println("Write Multiple Holding Registers");
              processRequest16(incomingData,len);
              sentToLoom_16();
              break;
            default:
              Serial.println("Unsupported Function Code");
              return;
          }
        } else {
          Serial.println("Invalid CRC");  // CRC mismatch
        }
      } else {
        Serial.println("Invalid Slave ID");  // Ignore request not meant for this slave
      }
    }
  }
}

void sentToLoom_16() {
  // Print the array
  for (int i = 0; i < sizeof(HoldingRegisters) / sizeof(uint16_t); i++) {
    Serial.print(HoldingRegisters[i]);
    Serial.print(" ");
  }
  Serial.println();

  userData_t userData;
  // Copy the array into the struct
  memcpy(&userData, HoldingRegisters, sizeof(userData));

  network.update();
  RF24NetworkHeader header1(Loom1);
  bool ok = network.write(header1, &userData, sizeof(userData));  // Send the data
}

void sentValue() {
  userData_t userData;

  // Copy the array into the struct
  memcpy(&userData, HoldingRegisters, sizeof(userData));

  network.update();
  RF24NetworkHeader header1(Loom1);
  bool ok = network.write(header1, &userData, sizeof(userData));  // Send the data
}

void printData() {
  Serial.print(payload.LoomID);
  Serial.print("\t");
  Serial.print(payload.LoomStatus);
  Serial.print("\t");
  Serial.print(payload.stoppagetime_mins);
  Serial.print("\t");
  Serial.print(payload.Weftstoppagetime_mins);
  Serial.print("\t");
  Serial.print(payload.Othersstoppagetime_mins);
  Serial.print("\t");
  Serial.print(payload.runtime_mins);
  Serial.print("\t");
  Serial.print(payload.LoomRpm);
  Serial.print("\t");
  Serial.print(payload.Shiftproduction);
  Serial.print("\t");
  Serial.print(payload.remainingBeam);
  Serial.println("   ");
}


uint16_t calculateCRC(uint8_t* data, uint8_t length) {
  uint16_t crc = 0xFFFF;
  for (uint8_t i = 0; i < length; i++) {
    crc ^= data[i];
    for (uint8_t j = 0; j < 8; j++) {
      if (crc & 0x0001) {
        crc >>= 1;
        crc ^= 0xA001;
      } else {
        crc >>= 1;
      }
    }
  }
  return crc;
}

void sendResponse(uint8_t* response, uint8_t length) {
  RS485Serial.write(response, length);
  RS485Serial.flush();
}

// Function Code ; 0x03 as a Slave device
void processRequest03(uint8_t* request, uint8_t length) {
  // Parse the starting address and quantity of registers
  // uint16_t startAddress = (request[2] << 8) | request[3];
  // uint16_t quantity = (request[4] << 8) | request[5];
  uint16_t startAddress = word(request[2], request[3]);
  uint16_t quantity = word(request[4], request[5]);

  // Validate the request
  if ((startAddress + quantity) - 1 > (sizeof(buffer) / sizeof(uint16_t))) {
    Serial.println("Invalid Address Range Request by Master");  //
  }

  // Build the response
  uint8_t response[5 + 2 * quantity];
  response[0] = MODBUS_SLAVE_ID;  // Slave ID
  response[1] = 0x03;             // Function Code
  response[2] = quantity * 2;     // Byte Count
  for (uint16_t i = startAddress; i < (startAddress + quantity); i++) {
    // response[3 + 2 * i] = HoldingRegisters[startAddress + i] >> 8;    // High byte
    // response[4 + 2 * i] = HoldingRegisters[startAddress + i] & 0xFF;  // Low byte
    response[3 + (i - startAddress) * 2] = highByte(buffer[i]);
    response[4 + ((i - startAddress) * 2)] = lowByte(buffer[i]);
  }

  // Append CRC
  uint16_t crc = calculateCRC(response, 3 + 2 * quantity);
  response[3 + 2 * quantity] = crc & 0xFF;  // CRC low byte
  response[4 + 2 * quantity] = crc >> 8;    // CRC high byte

  // Send the response
  sendResponse(response, 5 + 2 * quantity);
}

// Function Code ; 0x10 as a Slave device
void processRequest16(uint8_t* request, uint8_t length) {
  uint16_t startAddress = (request[2] << 8) | request[3];
  uint16_t quantity = (request[4] << 8) | request[5];
  uint8_t byteCount = request[6];

  // Validate the request
  if (startAddress + quantity > 10 || byteCount != quantity * 2) {
    Serial.println("Invalid Data length");  // Invalid address or data length
  }

  // Write the data to the holding registers
  for (uint16_t i = 0; i < quantity; i++) {
    HoldingRegisters[startAddress + i] = (request[7 + 2 * i] << 8) | request[8 + 2 * i];
  }

  // Build the response
  uint8_t response[8];
  response[0] = MODBUS_SLAVE_ID;  // Slave ID
  response[1] = 0x10;             // Function code
  response[2] = request[2];       // Starting address high byte
  response[3] = request[3];       // Starting address low byte
  response[4] = request[4];       // Quantity high byte
  response[5] = request[5];       // Quantity low byte
  uint16_t crc = calculateCRC(response, 6);
  response[6] = crc & 0xFF;  // CRC low byte
  response[7] = crc >> 8;    // CRC high byte

  // Send the response
  sendResponse(response, 8);
}

// Function Code ; 0x06 as a Slave device
void processRequest06(uint8_t* request) {  // Write a Single Holding Register
  uint8_t response[8];
  uint16_t registerAddress = (request[2] << 8) | request[3];
  uint16_t registerValue = (request[4] << 8) | request[5];

  if (registerAddress >= (sizeof(HoldingRegisters) / sizeof(uint16_t))) {
    Serial.println("Invalid Address Length");
    return;
  }

  // Step 6: Update the register
  HoldingRegisters[registerAddress] = registerValue;

  // Step 7: Prepare the response (echo the request)
  for (int i = 0; i < 6; i++) {
    response[i] = request[i];
  }

  // Step 8: Calculate CRC for the response
  uint16_t responseCRC = calculateCRC(response, 6);
  response[6] = responseCRC & 0xFF;  // CRC Low byte
  response[7] = responseCRC >> 8;    // CRC High byte

  // Step 9: Send the response
  sendResponse(response, 8);
}


/* LED Test
  // digitalWrite(2, HIGH);
  // delay(1000);
  // digitalWrite(2, LOW);
  // delay(1000);
*/
