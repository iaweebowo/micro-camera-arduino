#include <Arduino.h>
#include <Servo.h>

/* Function declarations */ 
void extend(int millisecs);
void retract(int millisecs);
void set_speed(int speed);
void move_to(int pos);
void go_forward();
void go_reverse();
void zoom(int zoom);
void recvWithStartEndMarkers();
void parseData();
void brush_vibrate(int millis);
void brush_off();

/* Variable declarations */ 
int potval;
const byte numChars = 32;
char receivedChars[numChars];
char textfromPC[(int)numChars/2];
char tempChars[numChars];        // temporary array for use by strtok() function
boolean newData = false;
float integerFromPC = 0;
boolean brushOn = false;
boolean brushOnStart = true;
unsigned long brushTimer = 0;
unsigned long brushTimerSaved = 0;
int brushVibLength = 0;

// Pin vals for L298N to Arduino
int motor1pin1 = 2;
int motor1pin2 = 3;

int motor2pin1 = 7;
int motor2pin2 = 8;

int ENAPIN = 5;
int ENBPIN = 6;

int BRSHPIN1 = 4;
int BRSHPIN2 = 12;

int SRVPIN = 11;

Servo zoomServo;

/* Single Run Code */
void setup() { 
  Serial.begin(9600);
  // Serial.flush();

  // Motor 1 Forward & Reverse
  pinMode(motor1pin1, OUTPUT);
  pinMode(motor1pin2, OUTPUT);

  // Motor 2 Forward & Reverse
  pinMode(motor2pin1,  OUTPUT);
  pinMode(motor2pin2, OUTPUT);

  // Controlling speed (0  = off and 255 = max speed):
  // Min speed 40!     
  pinMode(ENAPIN,  OUTPUT); // ENA Pin
  pinMode(ENAPIN, OUTPUT); // ENB Pin

  // Brush pins
  pinMode(BRSHPIN1, OUTPUT);
  pinMode(BRSHPIN2, OUTPUT);

  digitalWrite(BRSHPIN1, LOW);
  digitalWrite(BRSHPIN2, LOW);

  // pinMode(SRVPIN, OUTPUT);
  zoomServo.attach(SRVPIN); 

  // 630 is full extend, 0 is full retract
  move_to(635);

  zoomServo.write(0);

  Serial.println("SERIAL READY!");
}

/* Loop Code */
void loop() { 
  potval = analogRead(A0);
  if (brushOn) {
    brushTimer = millis();
    if (brushOnStart) {
      brushTimerSaved = brushTimer;
      brushOnStart = false;
    }
    brush_vibrate(brushVibLength);
  }
  // Serial.println(potval);
  strcpy(tempChars, receivedChars);
  recvWithStartEndMarkers();
  parseData();
}

/* Function Definitions */
// Serial string receive function
void recvWithStartEndMarkers() {
    static boolean recvInProgress = false;
    static byte ndx = 0;
    // Expected start and end characters
    char startMarker = '<';
    char endMarker = '>';
    char rc;

    // Wait for python input
    while (Serial.available() > 0 && newData == false) {
        rc = Serial.read();

        if (recvInProgress == true) {
            if (rc != endMarker) {
                receivedChars[ndx] = rc;
                ndx++;
                if (ndx >= numChars) {
                    ndx = numChars - 1;
                }
            }
            else {
                receivedChars[ndx] = '\0'; // terminate the string
                recvInProgress = false;
                ndx = 0;
                newData = true;
            }
        }

        else if (rc == startMarker) {
            recvInProgress = true;
        }
    }
}
// Received string details parsing
void parseData() {
  // Get parts of received string
  // Expected format: "<ABC 123;>"
  char * strtokIndx;
  strtokIndx = strtok(tempChars," ");
  strcpy(textfromPC, strtokIndx);
  strtokIndx = strtok(NULL, ";");   // this continues where the previous call left off
  integerFromPC = atof(strtokIndx); // convert this part to an integer

  // Parse parts to interpret commands
  if (newData == true) {
    switch(textfromPC[0]) {
      case 'A':
        Serial.println(receivedChars);
        break;
      case 'B':
        go_reverse();
        Serial.println("Done!");
        break;
      case 'E':
        extend(integerFromPC);
        Serial.println("Done!");
        break;
      case 'F':
        go_forward();
        Serial.println("Done!");
        break;
      case 'G':
        Serial.println(potval);
        break;
      case 'P':
        move_to(integerFromPC);
        Serial.println("Done!");
        break;
      case 'R':
        retract(integerFromPC);
        Serial.println("Done!");
        break;
      case 'S': 
        set_speed(integerFromPC);
        Serial.println(integerFromPC);
        break;
      case 'V':
        brushOn = true;
        brushOnStart = true;
        brushVibLength = integerFromPC;
        Serial.println("Done!");
        break;
      case 'Z':
        zoom(integerFromPC);
        Serial.println("Done!");
        break;
      default:
        Serial.println("Done!");
        break;
    }
    newData = false;
  }
}

void brush_vibrate(int millis) {
  if (int(brushTimer - brushTimerSaved) < millis) {
    digitalWrite(BRSHPIN1, HIGH);
    digitalWrite(BRSHPIN2, LOW);
    delayMicroseconds(1200);
    digitalWrite(BRSHPIN1, LOW);
    digitalWrite(BRSHPIN2, HIGH);
    delayMicroseconds(1200);
  }
  else {
    brush_off();
    brushOnStart = true;
    brushVibLength = 0;
  }
}

void brush_off() {
  brushOn = false;
  digitalWrite(BRSHPIN1, LOW);
  digitalWrite(BRSHPIN2, LOW);
}

// Directional functions
void go_forward() {
  digitalWrite(motor1pin1, HIGH);
  digitalWrite(motor1pin2, LOW);
}
void go_reverse() {
  digitalWrite(motor1pin1, LOW);
  digitalWrite(motor1pin2, HIGH);
}
// Speed setting
// Speed of 70 is minimum!
void set_speed(int speed) {
  analogWrite(ENAPIN, speed); 
}
// Timed movements
void extend(int millisecs) {
  if (potval < 630) {
    go_forward();
    set_speed(255);
    delay(millisecs);
  }
  set_speed(0);
}
void retract(int millisecs) {
  if (potval > 10) {
    go_reverse();
    set_speed(255);
    delay(millisecs);
  }
  set_speed(0);
}
// Position (rudimentary bang-bang control)
// Max pot val is usually a bit over 630
void move_to(int pos) {
  potval = analogRead(A0);
  if (pos > 640) pos = 640;
  else if (pos < 0) pos = 0;
  while (abs(pos - potval) > 1) {
    potval = analogRead(A0);
    // Serial.print(potval);
    // Serial.print("|");
    // Serial.println(pos);
    if (pos - potval > 1) {
      go_forward();
      set_speed(200);
    }
    if (potval - pos > 1) {
      go_reverse();
      set_speed(200);
    }
  }
  set_speed(0);
}

void zoom(int zoom) {
  zoomServo.write(zoom);
}

// void retract_to(int pos) {
//   while (potval > pos) {
//     set_speed(255);
//     go_reverse();
//     potval = analogRead(A0);
//   }
//   set_speed(0);
// }
