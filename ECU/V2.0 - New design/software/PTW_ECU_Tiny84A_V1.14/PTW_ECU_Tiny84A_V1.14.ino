/*
  PTW ECU V1.14

  Main controller programm for Systema PTW improved ECU. NOT pinout compatible with original ECU.

  The circuit:
  - ATtiny84A
  - compatible boardlayout with same pinout

  created 06. Nov.2019
  by Benjamin Burg <https://github.com/ChefBenni/PTW-Electronics>
  modified 11 May 2020
  by Benjamin Burg
                                                                        
  This example code is in the public domain.
  https://github.com/ChefBenni/PTW-Electronics
*/


//   ---   --  |\  | -- -----   /\   |\  | -----
//  |     |  | | \ | \    |    /__\  | \ |   |
//   ---   --  |  \| --   |   /    \ |  \|   |
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

// constant time values
const int MaxCycleTime = 150; // maximum cycle time before error occures  [ms]
const int BrakeDelay = 100;   // breaking delay to avoid shot through     [µs]
const int BrakeTime = 100;    // breaking time of the motor               [ms]
const int DebounceTime = 200; // time for trigger debounce                [µs]
const int LSDelay = 500;      // light barrier delay *measured 250µs*     [µs]


//  \    /   /\   |  |  |   /\    
//   \  /   /--\  |-\   |  /--\   
//    \/   /    \ |  \  | /    \  
// variables will change
int MagState = LOW;             // variable for reading the magazine button status
int TriggerState = LOW;         // variable for reading the trigger button status
int FaState = LOW;              // variable for reading the full auto status
int LsState = HIGH;             // variable for reading the light barrier status
int BrakeState = LOW;           // saves if motor is still breaking
int ErrorBrake = LOW;           // saves if motor breaked because of an error
int BreakeIndicator = HIGH;     // indicator if motor is stopped
long previousMillis = 0;        // internal calculation time for rotation
unsigned long currentMillis = 0;// internal calculation time for rotation


//  -- |-- ----- |  | |--|
//  \  |--   |   |  | |--|
//  -- |--   |   |__| |
void setup() {
  // inputs
  pinMode(Mag_Sig_Pin, INPUT);    // initialize the magazine button pin as an input
  pinMode(Trig_Sig_Pin, INPUT);   // initialize the trigger button pin as an input
  pinMode(FA_Sig_Pin, INPUT);     // initialize the full auto pin as an input
  pinMode(LS_Input_Pin, INPUT);   // initialize the light barrier pin as an input
  pinMode(Jumper1_Pin, INPUT);    // initialize the jumper 1 pin as an input
  pinMode(Jumper2_Pin, INPUT);    // initialize the jumper 2 pin as an input

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
  digitalWrite(LedPin, HIGH);        // turn the signal LED off
  digitalWrite(LS_Led_Pin, HIGH);   // turn the light barrier LED off
  digitalWrite(FET_On_Pin, LOW);    // turn the turn MOSFET off
  digitalWrite(FET_Brake_Pin, LOW); // turn the brake MOSFET off

  // blink at startup
  digitalWrite(LedPin, LOW);    // turn the LED on (LOW is the voltage level)
  delay(50);                    // wait for 50ms
  digitalWrite(LedPin, HIGH);   // turn the LED off by making the voltage LOW
  delay(50);                    // wait for 50ms
  digitalWrite(LedPin, LOW);    // turn the LED on
  delay(50);                    // wait for 50ms
  digitalWrite(LedPin, HIGH);   // turn the LED off
}


//  |\/|  /\  || |\ |
//  |  | /--\ || | \|
void loop() {
  // read the state of the pushbutton values:
  MagState = digitalRead(Mag_Sig_Pin);          // reads magazine state
  TriggerState = digitalRead(Trig_Sig_Pin);     // reads trigger state
  FaState = digitalRead(FA_Sig_Pin);            // reads semi / fullauto state
  BrakeTimeCheck();                             // check if breaking time reached

  if (MagState == LOW && TriggerState == LOW){  // checks if the trigger is pressed and magazine not empty
    if (FaState == HIGH){                       // if it's on semi
      semi();                                   // jump to semi()
    }
    else{                                       // if it's on auto
      automode();                               // jump to automode()
    }
  }

  else if (MagState == HIGH && TriggerState == LOW){  // blink if trigger is pressed but magazine is empty
    digitalWrite(LedPin, LOW);                       // turn the LED on (HIGH is the voltage level)
    delay(50);                                        // wait for 50ms
    BrakeTimeCheck();                                 // check if breaking time reached
    digitalWrite(LedPin, HIGH);                        // turn the LED off by making the voltage LOW
    delay(50);                                        // wait for 50ms
    BrakeTimeCheck();                                 // check if breaking time reached
      
    while (TriggerState == LOW){                      // wait while trigger is pushed
      TriggerState = digitalRead(Trig_Sig_Pin);       // reads trigger state
      BrakeTimeCheck();                               // check if breaking time reached
    }
  }
}


//  --  |-- |\_/| |
//  \   |-- |   | |
//  --  |-- |   | |
void semi()
{
  MotorOn();      // turn motor on
  Cycle();        // do a cycle
  MotorOff();     // turn motor off and brake
  Debounce();     // wait for trigger not pressed including debounce
  loop();         // jumps to main
}


//    /\   |    | -----  /--\
//   /--\  |    |   |   |    |
//  /    \  \__/    |    \__/
void automode()
{
  MotorOn();                                      // turn motor on
  while (MagState == LOW && TriggerState == LOW){ // cycle while trigger is pushed
    MagState = digitalRead(Mag_Sig_Pin);          // reads magazine state
    TriggerState = digitalRead(Trig_Sig_Pin);     // reads trigger state
    Cycle();                                      // do a cycle
  }
  MotorOff();                         // turn motor off and brake
  Debounce();                         // wait for trigger not pressed including debounce
  loop();                             // jumps to main
}


//  |-- |  |  |  |
//  |-- |-\   |-\
//  |-- |  \  |  \

void problem()
{
  if (ErrorBrake == LOW){             // if errorprogram starts for the first time brake motor
    MotorOff();                       // turn motor off and brake
    ErrorBrake = HIGH;                // write that the motor alreade stopped
  }
  
  // let the LED flash for signalling the error
  digitalWrite(LedPin, LOW);         // turn the LED on
  delay(100);                         // wait for 100ms
  digitalWrite(LedPin, HIGH);          // turn the LED off
  delay(100);                         // wait for 100ms

  problem();                          // jump back to the start of the error programm for contious flashing
}


void MotorOn()
{
  if (BreakeIndicator == LOW){        // check if motor is already stopped or still braking
    digitalWrite(FET_Brake_Pin, LOW); // turns the brake MOSFET off
    delayMicroseconds(BrakeDelay);    // turn-on-off delay time to avoid shot-through
  }
  digitalWrite(FET_On_Pin, HIGH);     // turn the turn MOSFET on
  digitalWrite(LS_Led_Pin, LOW);      // turn the light barrier LED on
  delayMicroseconds(LSDelay);         // wait for light barrier to start detecting
}

void MotorOff()
{
  digitalWrite(FET_On_Pin, LOW);      // turn the turn MOSFET off
  digitalWrite(LS_Led_Pin, HIGH);     // turn the light barrier LED off
  delayMicroseconds(BrakeDelay);      // turn-off delay time to avoid shot-through
  digitalWrite(FET_Brake_Pin, HIGH);  // turn the brake MOSFET on
  previousMillis = millis();          // read start time before brake time starts
  BreakeIndicator = LOW;              // reset braking indicator that motor is still spinning
}

void BrakeTimeCheck()
{
  currentMillis = millis();                       // reads current time and stores it to save timers
  // if current time - start time is bigger then braking time and motor is still spinning
  if(currentMillis - previousMillis > BrakeTime && BreakeIndicator == LOW){
    BrakingEnd();                                 // jump to end of breaking time           
  }
}

void BrakingEnd()
{
  digitalWrite(FET_Brake_Pin, LOW);   // turns the brake MOSFET off
  delayMicroseconds(BrakeDelay);      // turn-on-off delay time to avoid shot-through
  BreakeIndicator = HIGH;             // save that motor is stopped
}

void Debounce()
{
  TriggerState = digitalRead(Trig_Sig_Pin);       // reads trigger state
  while (TriggerState == LOW){                    // waits till trigger is not pressed
    delayMicroseconds(DebounceTime);              // button debounce delay time
    TriggerState = digitalRead(Trig_Sig_Pin);     // reads trigger state
    BrakeTimeCheck();                             // check if breaking time reached
  }
  
  // second test if trigger not pressed and debounced to avoid double shots
  delayMicroseconds(DebounceTime);                // button debounce delay time
  TriggerState = digitalRead(Trig_Sig_Pin);       // reads trigger state
  while (TriggerState == LOW){                    // waits till trigger is not pressed
    delayMicroseconds(DebounceTime);              // button debounce delay time
    TriggerState = digitalRead(Trig_Sig_Pin);     // reads trigger state
    BrakeTimeCheck();                             // check if breaking time reached
  } 
}

void Cycle()
{
  LsState = digitalRead(LS_Input_Pin);                  // reads in the gearposition from the light barrier
  previousMillis = millis();                            // read start time before cycle start
  while (LsState == LOW){                               // if starts at detection position wait for cycle
    LsState = digitalRead(LS_Input_Pin);                // reads in the gearposition from the light barrier
    currentMillis = millis();                           // reads current time and stores it to save timers
    if(currentMillis - previousMillis > MaxCycleTime){  // if current time - start time bigger then max cycle time
      problem();                      
    }
  }
  while (LsState == HIGH){                              // waits for full cycle completed
    LsState = digitalRead(LS_Input_Pin);                // reads in the gearposition from the light barrier
    currentMillis = millis();                           // reads current time and stores it to save timers
    if(currentMillis - previousMillis > MaxCycleTime){  // if current time - start time bigger then max cycle time
      problem();                      
    }
  }
}
