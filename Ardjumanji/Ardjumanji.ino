//Jumanji BAR v0.00

#include <Wire.h>
#include <VL53L0X.h>
#include "SoftwareSerial.h"
#include "DFRobotDFPlayerMini.h"

#define DEBUG
#define PIN_MP3_RX2       9
#define PIN_MP3_TX2       8
#define PIN_PIR_1         11
#define PIN_PIR_2         10
#define PIN_KEY1          2
#define PIN_DOOR          3

int playerState = 0;  //no sound
int playerVolume = 0; //no volume

#define LEVEL3_DISTANCE   700 //actual distance 100mm less
#define LEVEL1_DELAY      8000
#define LEVEL2_DELAY      4000
#define LEVEL3_DELAY      2000
#define TOUCH_DELAY       3000
#define VOLUME_OUT_DELAY  1000
#define VOLUME_IN_DELAY   500
#define VOLUME_MAX        30
#define VOLUME_MIN        16

SoftwareSerial mySoftwareSerial2(PIN_MP3_RX2, PIN_MP3_TX2); // RX, TX
DFRobotDFPlayerMini myDFPlayer2;

VL53L0X sensor;

void setup()
{
#ifdef DEBUG
  Serial.begin(9600);
#endif
  
  Wire.begin();
  mySoftwareSerial2.begin(9600);
  myDFPlayer2.begin(mySoftwareSerial2);

  sensor.init();
  sensor.setTimeout(500);
  // lower the return signal rate limit (default is 0.25 MCPS)
  sensor.setSignalRateLimit(0.1);
  // increase laser pulse periods (defaults are 14 and 10 PCLKs)
  sensor.setVcselPulsePeriod(VL53L0X::VcselPeriodPreRange, 18);
  sensor.setVcselPulsePeriod(VL53L0X::VcselPeriodFinalRange, 14);

#ifdef DEBUG
  if (!myDFPlayer2.begin(mySoftwareSerial2)) {  //Use softwareSerial to communicate with mp3.
    Serial.println(F("Unable to begin: 2"));
    Serial.println(F("1.Please recheck the connection!"));
    Serial.println(F("2.Please insert the SD card!"));
    //while(true);
  }
#endif
  pinMode(PIN_PIR_1, INPUT); 
  pinMode(PIN_PIR_2, INPUT); 
  pinMode(PIN_KEY1, INPUT_PULLUP); 
  pinMode(PIN_DOOR, INPUT_PULLUP); 

  myDFPlayer2.volume(0);
  
#ifdef DEBUG
  int num = myDFPlayer2.readFileCountsInFolder(1);
  Serial.print("files found: ");
  Serial.println(num);
  Serial.println("Setup has finished");
#endif
}

bool isBodyPresent(){
  static uint8_t oldVal=0;
  static unsigned long moment = 0;
  uint8_t val = digitalRead(PIN_PIR_1);
  if(oldVal != val){
#ifdef DEBUG
    Serial.print(F("PIR 1 val "));
    Serial.println(val);
#endif
    oldVal = val;
    if(val == LOW)moment = millis() + LEVEL1_DELAY;
  }
  return val == HIGH || (val == LOW && moment > millis());
}
bool isBodyNear(){
  static uint8_t oldVal=0;
  static unsigned long moment = 0;
  uint8_t val = digitalRead(PIN_PIR_2);
  if(oldVal != val){
#ifdef DEBUG
    Serial.print(F("PIR 2 val "));
    Serial.println(val);
#endif
    oldVal = val;
    if(val == LOW)moment = millis() + LEVEL2_DELAY;
  }
  return val == HIGH || (val == LOW && moment > millis());
}

bool isBodyCloser(int distance){
  static bool oldVal = false;
  static unsigned long moment = 0;
  bool val = false;
  int d = sensor.readRangeSingleMillimeters();
  if (sensor.timeoutOccurred() ) {
    val = oldVal;
  }
  else{
    val = (d - distance) < -100;
  }
  if(oldVal != val){
#ifdef DEBUG
    Serial.print(F("level 3 val "));
    Serial.println(val);
#endif
    oldVal = val;
    if(val == false)moment = millis() + LEVEL3_DELAY;
  }
  return val == true || (val == false && moment > millis());
}

bool isTouched(){
  static uint8_t oldVal=0;
  static unsigned long moment = 0;
  uint8_t val = digitalRead(PIN_KEY1);
  if(oldVal != val){
    oldVal = val;
    if(val == LOW)moment = millis() + TOUCH_DELAY;
#ifdef DEBUG
    Serial.print(F("touch val "));
    Serial.println(val);
#endif
  }
  return val == HIGH || (val == LOW && moment > millis());;
}

bool isDoorClosed(){
  static uint8_t oldVal=0;
  uint8_t val = digitalRead(PIN_DOOR);
  if(oldVal != val){
#ifdef DEBUG
    Serial.print(F("DOOR CLOSED "));
    Serial.println(val == HIGH);
#endif
    oldVal = val;
  }
  return val == HIGH;
}
void changeState(int newState){
  if(newState == playerState)return;
#ifdef DEBUG
  Serial.print(F("changeState "));
  Serial.print(playerState);
  Serial.print(F(" to "));
  Serial.println(newState);
#endif
  if(newState > playerState){
    playerState = newState;
    if(playerState > 1 || playerVolume == VOLUME_MIN){
      startPlaying(playerState);
    }
  }
  if(newState < playerState){
    playerState = newState;
    startPlaying(playerState);
  }
}
void startPlaying(int num){
  static unsigned long moment = 0;
  if(num > 0){
    if(playerState > 0){
      //need to wait for apropriate moment before change the track
      while((millis()-moment)%720 < 10);
    }
    myDFPlayer2.playFolder(1,num);
    moment = millis();
#ifdef DEBUG
    Serial.print(F("Start playing "));
    Serial.println(num);
#endif
  }
}
void volumeLoop(){
  static unsigned long moment = 0;
  if(playerState == 0 && playerVolume > VOLUME_MIN && moment < millis()){
    playerVolume-=2;
    if(playerVolume <= VOLUME_MIN){
      playerVolume = VOLUME_MIN;
      myDFPlayer2.stop();
#ifdef DEBUG
      Serial.println(F("STOP playing"));
#endif
    }
    else{
      myDFPlayer2.volume(playerVolume);
      moment = millis() + VOLUME_OUT_DELAY;
      #ifdef DEBUG
      Serial.print(F("Volume fade out "));
      Serial.println(playerVolume);
      #endif
    }
  }
  if(playerState == 1 && playerVolume < VOLUME_MAX && moment < millis()){
    playerVolume+=2;
    if(playerVolume > VOLUME_MAX)playerVolume=VOLUME_MAX;
    #ifdef DEBUG
    Serial.print(F("Volume fade in "));
    Serial.println(playerVolume);
    #endif
    myDFPlayer2.volume(playerVolume);
    moment = millis() + VOLUME_IN_DELAY;
  }
  if(playerState > 1 && playerVolume < VOLUME_MAX){
    playerVolume = VOLUME_MAX;
    #ifdef DEBUG
    Serial.print(F("Volume max "));
    Serial.println(playerVolume);
    #endif
    myDFPlayer2.volume(playerVolume);
  }
}
void loop(){
  volumeLoop();
  if(isDoorClosed())
  {
    if(isTouched()){
      changeState(4);
    }
    else{
      if(isBodyCloser(LEVEL3_DISTANCE)){
        changeState(3);
      }
      else{
        if(isBodyNear()){
          changeState(2);
        }
        else{
          if(isBodyPresent()){
            changeState(1);
          }
          else{
            changeState(0);
          }
        }
      }
    }
  }

  if (myDFPlayer2.available()) {
    printDetail(myDFPlayer2.readType(), myDFPlayer2.read()); //Print the detail message from DFPlayer to handle different errors and states.
  }
  delay(100);
}

void printDetail(uint8_t type, int value){
  switch (type) {
    case TimeOut:
      Serial.println(F("Time Out!"));
      break;
    case WrongStack:
      Serial.println(F("Stack Wrong!"));
      break;
    case DFPlayerCardInserted:
      Serial.println(F("Card Inserted!"));
      break;
    case DFPlayerCardRemoved:
      Serial.println(F("Card Removed!"));
      break;
    case DFPlayerCardOnline:
      Serial.println(F("Card Online!"));
      break;
    case DFPlayerUSBInserted:
      //Serial.println("USB Inserted!");
      break;
    case DFPlayerUSBRemoved:
      //Serial.println("USB Removed!");
      break;
    case DFPlayerPlayFinished:
      //Serial.print(F("Number:"));
      //Serial.print(value);
      //Serial.println(F(" Play Finished!"));

      break;
    case DFPlayerError:
      Serial.print(F("DFPlayerError:"));
      switch (value) {
        case Busy:
          Serial.println(F("Card not found"));
          break;
        case Sleeping:
          //Serial.println(F("Sleeping"));
          break;
        case SerialWrongStack:
          Serial.println(F("Get Wrong Stack"));
          break;
        case CheckSumNotMatch:
          Serial.println(F("Check Sum Not Match"));
          break;
        case FileIndexOut:
          //Serial.println(F("File Index Out of Bound"));
          break;
        case FileMismatch:
          Serial.println(F("Cannot Find File"));
          //errNoFile = 1;
          break;
        case Advertise:
          //Serial.println(F("In Advertise"));
          break;
        default:
          break;
      }
      break;
    default:
      break;
  }
}
