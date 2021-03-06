/*
  PTW ECU V2.0 IO-test

  This software is to test all of the IO-ports of the ECU with ATTINY84A. It is NOT compatible with the original Systema ECU!

  The circuit:
  - LED attached from pin 10
  - pushbuttons like trigger, magazin detect, light barrier or FA-input will light the LED and put a high on both mosfet outputs


  created 06. Nov.2019
  by Benjamin Burg <https://github.com/ChefBenni/PTW-Electronics>
  modified 11 May 2020
  by Benjamin Burg

  This example code is in the public domain.

  https://github.com/ChefBenni/PTW-Electronics
*/

// constants won't change. They're used here to set pin numbers:
const int Mag_Sig_Pin = 5;    // the number of the magazine detection switch pin  - Low active  - pull-up
const int Trig_Sig_Pin = 9;   // the number of the trigger buttin pin             - Low active  - pull-up
const int LedPin = 10;        // the number of the LED pin                        - High active
const int FA_Sig_Pin = 8;     // the number of the full auto signal pin           - Low active  - pull-up
const int LS_Input_Pin = 3;   // the number of the light barrier detector pin     - Low active
const int LS_Led_Pin = 2;    // the number of the light barrier LED pin          - Low active
const int FET_On_Pin = 6;    // the number of the motor turn MOSFET pin          - High active
const int FET_Brake_Pin = 4; // the number of the brake MOSFET pin               - High active
const int Jumper1_Pin = 0;    // the number of the jumper1 pin                    - Low active  - pull-up
const int Jumper2_Pin = 1;    // the number of the jumper2 pin                    - Low active  - pull-up


// variables will change:
int MagState = 0;             // variable for reading the magazine button status
int TriggerState = 0;         // variable for reading the trigger button status
int FaState = 0;              // variable for reading the full auto status
int LsState = 0;              // variable for reading the light barrier status


void setup() {
  // inputs
  pinMode(Mag_Sig_Pin, INPUT);    // initialize the pushbutton pin as an input
  pinMode(Trig_Sig_Pin, INPUT);   // initialize the pushbutton pin as an input
  pinMode(FA_Sig_Pin, INPUT);     // initialize the pushbutton pin as an input
  pinMode(LS_Input_Pin, INPUT);   // initialize the pushbutton pin as an input
  pinMode(Jumper1_Pin, INPUT);    // initialize the pushbutton pin as an input
  pinMode(Jumper2_Pin, INPUT);    // initialize the pushbutton pin as an input

  // pull resistors
  digitalWrite(Mag_Sig_Pin, HIGH);     // turn on pull-up resitor
  digitalWrite(Trig_Sig_Pin, HIGH);    // turn on pull-up resitor
  digitalWrite(FA_Sig_Pin, HIGH);      // turn on pull-up resitor
  digitalWrite(Jumper1_Pin, HIGH);     // turn on pull-up resitor
  digitalWrite(Jumper2_Pin, HIGH);     // turn on pull-up resitor
  
  // outputs
  pinMode(LedPin, OUTPUT);        // initialize the LED pin as an output
  pinMode(LS_Led_Pin, OUTPUT);    // initialize the light barrier LED pin as an output
  pinMode(FET_On_Pin, OUTPUT);    // initialize the turn MOSFET pin as an output
  pinMode(FET_Brake_Pin, OUTPUT); // initialize the brake MOSFET pin as an output

  // outpus initial start state
  digitalWrite(LedPin, LOW);        // turn the signal LED off
  digitalWrite(LS_Led_Pin, LOW);    // turn the light barrier LED on
  digitalWrite(FET_On_Pin, LOW);    // turn the turn MOSFET off
  digitalWrite(FET_Brake_Pin, LOW); // turn the brake MOSFET off

  digitalWrite(LedPin, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(50);                    // wait for 50ms
  digitalWrite(LedPin, LOW);    // turn the LED off by making the voltage LOW
  delay(50);                    // wait for 50ms
  digitalWrite(LedPin, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(50);                    // wait for 50ms
  digitalWrite(LedPin, LOW);    // turn the LED off by making the voltage LOW
}

void loop() {
  // read the state of the pushbutton value:
  MagState = digitalRead(Mag_Sig_Pin);
  TriggerState = digitalRead(Trig_Sig_Pin);
  FaState = digitalRead(FA_Sig_Pin);
  LsState = digitalRead(LS_Input_Pin);

  // check if the pushbutton is pressed. If it is, the buttonState is HIGH:
  if (MagState == LOW || TriggerState == LOW || FaState == LOW || LsState == HIGH) {
    // turn LED on:
    digitalWrite(LedPin, LOW);
    digitalWrite(FET_On_Pin, HIGH);
    digitalWrite(FET_Brake_Pin, HIGH);
    
  } else {
    // turn LED off:
    digitalWrite(LedPin, HIGH);
    digitalWrite(FET_On_Pin, LOW);
    digitalWrite(FET_Brake_Pin, LOW);
  }
}
