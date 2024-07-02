#include <Arduino.h>
#include "L298N.h"

/* Function declarations */ 
void extend(int millisecs);
void retract(int millisecs);
void set_speed(int speed);
void move_to(int pos);
void go_forward();
void go_reverse();
void recvWithStartEndMarkers();
void parseData();

/* Variable declarations */ 
int potval;
const byte numChars = 32;
char receivedChars[numChars];
char textfromPC[(int)numChars/2];
char tempChars[numChars];        // temporary array for use by strtok() function
boolean newData = false;
int integerFromPC = 0;

// Pin vals for L298N to Arduino
int motor1pin1 = 2;
int motor1pin2 = 3;

int motor2pin1 = 4;
int motor2pin2 = 5;

int ENAPIN = 9;
int ENBPIN = 10;

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

  //Controlling speed (0  = off and 255 = max speed):     
  pinMode(ENAPIN,  OUTPUT); // ENA Pin
  pinMode(ENAPIN, OUTPUT); // ENB Pin

  // 630 is full extend, 0 is full retract
  potval = analogRead(A0);
  move_to(630);
  // analogWrite(9, 0);
}

/* Loop Code */
void loop() { 
  potval = analogRead(A0);
  // Serial.println(String(potval));
  strcpy(tempChars, receivedChars);
  recvWithStartEndMarkers();
  parseData();
}

/* Function Definitions */
// Serial string receive function
void recvWithStartEndMarkers() {
    static boolean recvInProgress = false;
    static byte ndx = 0;
    char startMarker = '<';
    char endMarker = '>';
    char rc;

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
  char * strtokIndx;
  strtokIndx = strtok(tempChars," ");
  strcpy(textfromPC, strtokIndx);
  strtokIndx = strtok(NULL, ";");   // this continues where the previous call left off
  integerFromPC = atoi(strtokIndx); // convert this part to an integer

  // Parse parts to interpret commands
  if (newData == true) {
    switch(textfromPC[0]) {
      case 'A':
        Serial.println(receivedChars);
        delay(1000);
        break;
      case 'E':
        extend(integerFromPC);
        break;
      case 'R':
        retract(integerFromPC);
        break;
      case 'P':
        move_to(integerFromPC);
        break;
      default:
        break;
    }
    newData = false;
  }
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
void set_speed(int speed) {
  analogWrite(ENAPIN, speed); 
}
// Timed movements
void extend(int millisecs) {
  if (potval < 630) {
    set_speed(255);
    go_forward();
    delay(millisecs);
  }
  set_speed(0);
}
void retract(int millisecs) {
  if (potval > 10) {
    set_speed(255);
    go_reverse();
    delay(millisecs);
  }
  set_speed(0);
}
// Position (rudimentary bang-bang control)
void move_to(int pos) {
  while (potval < pos) {
    set_speed(255);
    go_forward();
    potval = analogRead(A0);
  }
  delay(500);
  while (potval > pos) {
    set_speed(255);
    go_reverse();
    potval = analogRead(A0);
  }
  set_speed(0);
}

// void retract_to(int pos) {
//   while (potval > pos) {
//     set_speed(255);
//     go_reverse();
//     potval = analogRead(A0);
//   }
//   set_speed(0);
// }
