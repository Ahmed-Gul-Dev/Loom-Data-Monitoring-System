/*
      Loom Simulator
*/

#define StopPin 3
#define OtherPin 7
#define FTPin 9
#define OtherInput 6
#define FT_fault 5
#define StopSW 4
#define led 13

#include <Bounce2.h>
Bounce2::Button Stopbutton = Bounce2::Button();

bool stopStatus = true;
void setup() {
  Serial.begin(9600);
  Stopbutton.attach(StopSW, INPUT);  // USE EXTERNAL PULL-UP

  // DEBOUNCE INTERVAL IN MILLISECONDS
  Stopbutton.interval(10);
  Stopbutton.setPressedState(LOW);

  pinMode(OtherInput, INPUT);
  pinMode(FT_fault, INPUT);

  pinMode(led, OUTPUT);
  pinMode(FTPin, OUTPUT);
  pinMode(OtherPin, OUTPUT);
  pinMode(StopPin, OUTPUT);
  digitalWrite(StopPin, HIGH);
  digitalWrite(OtherPin, LOW);
  digitalWrite(FTPin, LOW);
  digitalWrite(led, LOW);
  delay(200);
}

void loop() {
  Stopbutton.update();
  if (Stopbutton.pressed()) {
    stopStatus = !stopStatus;
    Serial.println(stopStatus);
    if (stopStatus) {
      PORTD |= B00001000;   // Stop Pin HIGH
    } else {                // Machine Run State
      PORTD &= !B10000000;  // Other Pin LOW
      PORTB &= !B00000010;  // FT Pin LOW
      PORTD &= !B00001000;  // Stop Pin LOW
    }
  }
  else if (digitalRead(OtherInput) == 0 && stopStatus == 0) {
    stopStatus = true;
    PORTD |= B10001000;  // Stop Pin and Other Pin HIGH
    PORTB &= !B00000010;  // FT Pin LOW
  }
  else if (digitalRead(FT_fault) == 0 && stopStatus == 0) {
    stopStatus = true;   
    PORTD &= !B10000000;  // Other Pin LOW
     PORTD |= B00001000;   // Stop Pin HIGH
    PORTB |= B00000010;   // FT Pin HIGH
  }

  if (stopStatus == 0) {  // Machine is Running !
    PORTB |= B00000010;   // FT Pin HIGH
    delay(10);
    PORTB &= !B00000010;  // FT Pin LOW
    delay(90);
  }
}

/*
Stopbutton.update();
  if (Stopbutton.pressed()) {
    stopStatus = !stopStatus;
    // Serial.println(stopStatus);
    if (stopStatus) {
      digitalWrite(StopPin, HIGH);
    } else {  // Machine Run State
      digitalWrite(OtherPin, LOW);
      digitalWrite(FTPin, LOW);
      digitalWrite(StopPin, LOW);
    }
  } else if (digitalRead(OtherInput) == 0 && stopStatus == 0) {
    stopStatus = true;
    digitalWrite(StopPin, HIGH);
    digitalWrite(OtherPin, HIGH);
    digitalWrite(FTPin, LOW);
  } else if (digitalRead(FT_fault) == 0 && stopStatus == 0) {
    stopStatus = true;
    digitalWrite(StopPin, HIGH);
    digitalWrite(OtherPin, LOW);
    digitalWrite(FTPin, HIGH);
  }

  if (stopStatus == 0) {  // Machine is Running !
    digitalWrite(FTPin, HIGH);
    delay(10);
    digitalWrite(FTPin, LOW);
    delay(90);
  }

*/
