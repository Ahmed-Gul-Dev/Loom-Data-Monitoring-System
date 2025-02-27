#define ID 1

#include <RF24.h>
#include <RF24Network.h>
#include <SPI.h>
#include <EEPROM.h>

RF24 radio(9, 8);            // nRF24L01 (CE,CSN)
RF24Network network(radio);  // Include the radio in the network

const uint16_t this_device = 01;  // Address of this node in Octal format
const uint16_t Master = 00;       // Master Node
unsigned long last_sent = 0;      // Wireless data transfer previous millis

struct payload_t {  // Structure of our payload
  uint16_t LoomID = ID;
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
userData_t userData;

bool send_data = false;
unsigned long RunMillis = 0, Stoppagemillis = 0;                                //32-bit
int runCount = 0, stoppageCount = 0, weftFaultCount = 0, othersFaultcount = 0;  // 1min Counter
int BeamLength = 0, BeamLengthShrink = 0, Pick = 0, ShrinkFactor = 0;
float LoomProduction = 0, productionFactor = 0;  // pick/inch * 39.4

