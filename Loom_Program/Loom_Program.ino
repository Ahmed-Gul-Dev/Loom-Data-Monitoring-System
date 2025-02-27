/**************************************************
Run time
Eff%
Stoppage time
Weft Stoppage time
Others Stoppage time
Loom Rpm
Loom Status
Loom Production(m)
Remaining Beam Length
****************************************************
Pick/inch (density)
Beam Length
Reed
Beam No.
Sizing Lot
Shrinkage(%)
*****************************************************/

#define VoltagePin 5
#define RedLED 6     // Run/Stop Signal
#define YellowLED 4  // Leno/Other Fault Signal
#define BlueLED 3    // Weft Signal
#define interval 5

#include "Config.h"

// daily mins = 60 * 24 = 1440 mins
// weekly mins = 60 * 24 * 7 = 10080 mins
// monthly mins = 60 * 24 * 30 = 43200 mins

int pulses = 0;
void count() {
  if (digitalRead(RedLED) == 1) {
    pulses++;
  }
}

void setup() {
  Serial.begin(9600);
  initPins();
  attachInterrupt(digitalPinToInterrupt(BlueLED), count, FALLING);

  SPI.begin();
  radio.begin();
  network.begin(90, this_device);  //(channel, node address)
  radio.setDataRate(RF24_2MBPS);
  Serial.println("Success !");
  delay(1000);
}

uint32_t pmillis = 0;
void loop() {
  network.update();
  // == == = Receiving == == =
  while (network.available()) {  // Is there any incoming data ?
    RF24NetworkHeader header;
    network.read(header, &userData, sizeof(userData));  // Read the incoming data

    if (userData.beamlength != BeamLength && userData.beamlength >= 0)
      writeIntIntoEEPROM(8, userData.beamlength);
    if (userData.pick != Pick && userData.pick >= 0)
      writeIntIntoEEPROM(10, userData.pick);
    if (userData.shrinkage != ShrinkFactor && userData.shrinkage >= 0)
      writeIntIntoEEPROM(12, userData.shrinkage);
    productionFactor = (float)(userData.pick) * 39.4;
    BeamLengthShrink = userData.beamlength - ((userData.beamlength / 100) * userData.shrinkage);
    // ShiftChange();
    Serial.println("Data Received from Master !!");
    Serial.print(userData.beamlength);
    Serial.print("\t");
    Serial.print(userData.pick);
    Serial.print("\t");
    Serial.println(userData.shrinkage);
  }

  if (digitalRead(RedLED) == 0) {  // Stop Signal
    payload.LoomStatus = 0;
    payload.LoomRpm = 0;
    if (millis() - Stoppagemillis >= 1000) {  // 1 sec Counter
      if (digitalRead(YellowLED) == 0) {
        othersFaultcount++;
      } else if (digitalRead(BlueLED) == 0) {
        weftFaultCount++;
      } else {
        stoppageCount++;
      }
      Stoppagemillis = millis();
    }
    MinsFunc();
  } else if (digitalRead(RedLED) == 1) {  // Running Signal
    payload.LoomStatus = 1;
    if (millis() - RunMillis >= 990) {  // 1 sec Counter
      runCount++;
      RunMillis = millis();
    }
    if (runCount >= interval) {
      payload.LoomRpm = pulses;
      pulses = 0;
      runCount = 0;
      ProductionFunc();
    }
  }
  if (millis() - pmillis >= 10000) {
    network.update();
    LoomProduction += (float)payload.LoomRpm / productionFactor;
    payload.Shiftproduction = (uint16_t)LoomProduction;
    payload.remainingBeam = BeamLengthShrink - payload.Shiftproduction;
    printData();
    RF24NetworkHeader headerM(Master);
    bool ok = network.write(headerM, &payload, sizeof(payload));  // Send the data
    pmillis = millis();
  }


  // if (digitalRead(VoltagePin) == 1) {
  //   saveEEPROM();
  //   delay(3000);
  // }
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


void writeIntIntoEEPROM(int address, int number)  // for only 2 bytes - int
{
  byte byte1 = number >> 8;
  byte byte2 = number & 0xFF;
  EEPROM.write(address, byte1);
  EEPROM.write(address + 1, byte2);
}

int readIntFromEEPROM(int address) {
  byte byte1 = EEPROM.read(address);
  byte byte2 = EEPROM.read(address + 1);
  return (byte1 << 8) + byte2;
}

void initPins() {
  Serial.println("Initializing...!");
  pinMode(VoltagePin, INPUT);
  pinMode(RedLED, INPUT);
  pinMode(YellowLED, INPUT);
  pinMode(BlueLED, INPUT);

  BeamLength = readIntFromEEPROM(8);
  Pick = readIntFromEEPROM(10);
  ShrinkFactor = readIntFromEEPROM(12);
  // EEPROM.get(0, LoomProduction);

  productionFactor = (float)(Pick)*39.4;
  BeamLengthShrink = BeamLength - ((BeamLength / 100) * ShrinkFactor);

  Serial.print(BeamLength);
  Serial.print("\t");
  Serial.print(Pick);
  Serial.print("\t");
  Serial.print(ShrinkFactor);
  Serial.print("\t");
  Serial.print(productionFactor);
  Serial.print("\t");
  Serial.println(BeamLengthShrink);
}

void saveEEPROM() {
  writeIntIntoEEPROM(14, payload.remainingBeam);
  writeIntIntoEEPROM(16, payload.Shiftproduction);
  // EEPROM.put(0, LoomProduction);
  EEPROM.write(18, payload.Othersstoppagetime_mins);
  EEPROM.write(19, payload.stoppagetime_mins);
  EEPROM.write(20, payload.Weftstoppagetime_mins);
  EEPROM.write(21, payload.runtime_mins);
  EEPROM.write(22, payload.stoppagetime_hrs);
  EEPROM.write(23, payload.Weftstoppagetime_hrs);
  EEPROM.write(24, payload.Othersstoppagetime_hrs);
  EEPROM.write(25, payload.runtime_hrs);
}

void ProductionFunc() {
  payload.runtime_mins++;
  if (payload.runtime_mins >= 60) {
    payload.runtime_hrs++;
    payload.runtime_mins = 0;
  }
}

void MinsFunc() {
  if (stoppageCount >= interval) {
    payload.stoppagetime_mins++;
    stoppageCount = 0;
  } else if (weftFaultCount >= interval) {
    payload.Weftstoppagetime_mins++;
    weftFaultCount = 0;
  } else if (othersFaultcount >= interval) {
    payload.Othersstoppagetime_mins++;
    othersFaultcount = 0;
  }

  if (payload.stoppagetime_mins >= 60) {
    payload.stoppagetime_hrs++;
    payload.stoppagetime_mins = 0;
  } else if (payload.Weftstoppagetime_mins >= 60) {
    payload.Weftstoppagetime_hrs++;
    payload.Weftstoppagetime_mins = 0;
  } else if (payload.Othersstoppagetime_mins >= 60) {
    payload.Othersstoppagetime_hrs++;
    payload.Othersstoppagetime_mins = 0;
  }
}