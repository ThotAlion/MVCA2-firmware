#include <Wire.h>
#include <VL53L1X.h>

// encoder
#define CHA1 2
#define CHB1 4
#define CHA2 3
#define CHB2 5
#define CHA3 18
#define CHB3 22
#define CHA4 19
#define CHB4 24
// I²C on 20 and 21

int pinPWM[] =    {10, 11, 12, 44};
int pinDir[] =    {30, 31, 32, 33};
int SHUTLASER[] = {6 , 7, 8, 9};

int realPos[] =     {0, 0, 0, 0};
int change = 0;
int verb = 1;
int desiredPos[] =  {0, 0, 0, 0};
int desiredVit[] =  {0, 0, 0, 0};
int rangemm[] =     {0, 0, 0, 0};
int state[] =       {0, 0, 0, 0};

// serial decoder
int serialState = 0;
String inputString = "";
int id;
int mode;
int param;
int temp;
int error;
int errorInt;

VL53L1X lasers[4];

// sampling rate (ms)
int dtMs = 10;
int Vmax = 30;
float Kp = 2.0;
float KKp[] = {1.0, 1.0, 1.0, 1.0};
float Ki = 0.0;
int freq;

void setup() {
  // encoder pins in input:
  pinMode(CHA1, INPUT_PULLUP);
  pinMode(CHB1, INPUT_PULLUP);
  pinMode(CHA2, INPUT_PULLUP);
  pinMode(CHB2, INPUT_PULLUP);
  pinMode(CHA3, INPUT_PULLUP);
  pinMode(CHB3, INPUT_PULLUP);
  pinMode(CHA4, INPUT_PULLUP);
  pinMode(CHB4, INPUT_PULLUP);

  // motors switched off
  for (int imotor = 0; imotor <= 3; imotor++) {
    pinMode(pinPWM[imotor], OUTPUT);
    analogWrite(pinPWM[imotor], 0);
    pinMode(pinDir[imotor], OUTPUT);
    digitalWrite(pinDir[imotor], LOW);
  }

  //trigger interrupt on channel A
  attachInterrupt(digitalPinToInterrupt(CHA1), incDec1, CHANGE );
  attachInterrupt(digitalPinToInterrupt(CHA2), incDec2, CHANGE );
  attachInterrupt(digitalPinToInterrupt(CHA3), incDec3, CHANGE );
  attachInterrupt(digitalPinToInterrupt(CHA4), incDec4, CHANGE );
  Serial.begin(115200);
  Wire.begin();
  Wire.setClock(400000);
  // switch off all lasers
  for (int ilaser = 0; ilaser <= 3; ilaser++) {
    pinMode(SHUTLASER[ilaser], OUTPUT);
    digitalWrite(SHUTLASER[ilaser], LOW);
  }
  // set address of lasers
  for (int ilaser = 0; ilaser <= 3; ilaser++) {
    digitalWrite(SHUTLASER[ilaser], HIGH);
    lasers[ilaser].init();
    lasers[ilaser].setAddress(ilaser + 1);
    lasers[ilaser].setDistanceMode(VL53L1X::Short);
    lasers[ilaser].setMeasurementTimingBudget(20000);
    lasers[ilaser].startContinuous(20);
    delay(100);
  }
}

void loop() {
  while (Serial.available()) {
    // get the new byte:
    char inChar = (char)Serial.read();
    // add it to the inputString:
    
    // if the incoming character is a newline, set a flag so the main loop can
    // do something about it:
    if (inChar == '\n') {
      /*Serial.print("etat:");
      Serial.println(serialState);
      Serial.print("entree:");
      Serial.println(inputString);*/
      switch (serialState) {
        case 0:
          if (inputString == "A") {
            serialState = 1;
          }
          break;
        case 1:
          temp = inputString.toInt();
          if (temp == 0) {
            serialState = 0;
          } else {
            id = temp;
            //Serial.print("id:");
            //Serial.println(id);
            serialState = 2;
          }
          break;
        case 2:
          temp = inputString.toInt();
          if (temp == 0) {
            serialState = 0;
          } else {
            mode = temp;
            //Serial.print("mode:");
            //Serial.println(mode);
            serialState = 3;
          }
          break;
        case 3:
          param = inputString.toInt();
          if (id <= 4) {
            state[id - 1] = mode;
            if (mode == 1) {
              realPos[id - 1] = param;
            } else if (mode == 2) {
              desiredVit[id - 1] = param;
            } else if (mode == 3) {
              errorInt = 0;
              desiredPos[id - 1] = param;
            }
          } else if (id == 5) {
            Vmax = param;
          } else if (id == 6) {
            Kp = param/10.0;
          } else if (id == 7) {
            Ki = param/100.0;
          }
          serialState = 0;
          break;
      }
      inputString = "";
    }else{
      inputString += inChar;
    }
  }
  for (int imotor = 0; imotor <= 3; imotor++) {
    // read lasers
    lasers[imotor].read(false);
    if (lasers[imotor].ranging_data.range_status == 0) {
      rangemm[imotor] = lasers[imotor].ranging_data.range_mm;
    } else {
      rangemm[imotor] = 1000;
    }
    //print
    if(change==1 && verb == 1){
      Serial.print(imotor);
      Serial.print(":");
      Serial.println(realPos[imotor]);
    }
    // choose speed
    int v = 0;
    switch (state[imotor]) {
      case 2:
        v = desiredVit[imotor];
        break;
      case 3:
        error = realPos[imotor]-desiredPos[imotor];
        errorInt+=error;
        v = (int)(KKp[imotor]*Kp*error+Ki*errorInt);
        break;  
      default:
        v = 0;
    }
    // anti node protection
    if(rangemm[imotor]<50 and v>0){
      v = 0;
    }
    // limit speed
    if (v > 0) {
      if (v > Vmax) {
        v = Vmax;
      }
      analogWrite(pinPWM[imotor], v);
      digitalWrite(pinDir[imotor], LOW);
    } else if (v < 0) {
      if (v < -Vmax) {
        v = -Vmax;
      }
      analogWrite(pinPWM[imotor], -v);
      digitalWrite(pinDir[imotor], HIGH);
    } else {
      analogWrite(pinPWM[imotor], 0);
      digitalWrite(pinDir[imotor], LOW);
    }
  }
  if(change == 1){change=0;}
  delay(dtMs);
}



// INTERRUPTS
void incDec1() {
  if (digitalRead(CHA1) && !digitalRead(CHB1)) {
    realPos[0]++;change = 1;
  }
  if (digitalRead(CHA1) && digitalRead(CHB1)) {
    realPos[0]--;change = 1;
  }
  if (!digitalRead(CHA1) && digitalRead(CHB1)) {
    realPos[0]++;change = 1;
  }
  if (!digitalRead(CHA1) && !digitalRead(CHB1)) {
    realPos[0]--;change = 1;
  }
}
void incDec2() {
  if (digitalRead(CHA2) && !digitalRead(CHB2)) {
    realPos[1]--;change = 1;
  }
  if (digitalRead(CHA2) && digitalRead(CHB2)) {
    realPos[1]++;change = 1;
  }
  if (!digitalRead(CHA2) && digitalRead(CHB2)) {
    realPos[1]--;change = 1;
  }
  if (!digitalRead(CHA2) && !digitalRead(CHB2)) {
    realPos[1]++;change = 1;
  }
}
void incDec3() {
  if (digitalRead(CHA3) && !digitalRead(CHB3)) {
    realPos[2]--;change = 1;
  }
  if (digitalRead(CHA3) && digitalRead(CHB3)) {
    realPos[2]++;change = 1;
  }
  if (!digitalRead(CHA3) && digitalRead(CHB3)) {
    realPos[2]--;change = 1;
  }
  if (!digitalRead(CHA3) && !digitalRead(CHB3)) {
    realPos[2]++;change = 1;
  }
}
void incDec4() {
  if (digitalRead(CHA4) && !digitalRead(CHB4)) {
    realPos[3]--;change = 1;
  }
  if (digitalRead(CHA4) && digitalRead(CHB4)) {
    realPos[3]++;change = 1;
  }
  if (!digitalRead(CHA4) && digitalRead(CHB4)) {
    realPos[3]--;change = 1;
  }
  if (!digitalRead(CHA4) && !digitalRead(CHB4)) {
    realPos[3]++;change = 1;
  }
}
