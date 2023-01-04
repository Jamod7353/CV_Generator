#include <Arduino.h>
#include <Wire.h>
//#define ENCODER_DO_NOT_USE_INTERRUPTS
#include <Encoder.h>
#include <Adafruit_MCP4725.h>
#include <Bounce2.h>
#include "scales.h"
#include "settings.h"
#include "screen.h"

int16_t debugVar = 0; // TODO: löschen

// DAC (I2C)
#define MCP4725_0 0x60
#define MCP4725_1 0x61 //TODO 60 oder 62? -> löten!
Adafruit_MCP4725 dac0;
Adafruit_MCP4725 dac1;

// Encoder
Encoder enc(PIN_ENC_0, PIN_ENC_1);
long encPosition = -999;

// Buttons
#define SET_MENU 0
#define ROTARY_BUTTON 1
#define SEQ_0 2
#define SEQ_1 3
#define NUM_OF_BUTTONS 4
Bounce *b = new Bounce[NUM_OF_BUTTONS];
// TODO: record-button + alles andere!

// Menu
boolean seq_0 = false;
boolean seq_1 = false;
#define PLAY_MODE 0
#define RANGE 1
#define SCALE 2
#define POSSIBILIY 3
#define TIMER_MODE 4
#define SPEED 5
#define SHIFT 6
#define MENU_NUMBER 7
byte menu_number = PLAY_MODE;
long encPos_tmp;
long menuValue;
#define CLK_MODE 0
#define TRIG_MODE 1
byte timer_mode[2];
#define MENU_TIME 1000
long menuTimer;
boolean menuOnScreen = false;

// Play Control
byte value_tone[2] = {0, 0}; //calculated value -> to scale in volt: value*1/12
#define NUM_OF_STEPS 16  // Array length for random/record
byte range_semitones[2] = {NUM_OF_STEPS, NUM_OF_STEPS};
byte random_steps0[NUM_OF_STEPS];
byte random_steps1[NUM_OF_STEPS];
byte random_step_pointer[2] = {0, 0};
#define MAX_POSSIBILITY 101 // -1 => 100
int possibility[2] = {0, 0}; // value 0-1023
byte scale_pointer[2] = {0, 0};
scale picked_scale[2] = {scales[0], scales[0]};

#define UP 0
#define DOWN 1
#define RANDOM 2
#define RANDOM_KEEP 3
#define RANDOM_AREA 4
#define SLOWLY_UP 5
#define SLOWLY_DOWN 6
#define SLOWLY_BOUNCE 7
#define INPUT 8
#define PLAYMODE_NUMBER 9
byte play_mode[2];
boolean stepMarked[2] = {false, false};
boolean bounceUp[2] = {true, true};

// Clock
long timeForNextStep;
int millisToInterrupt;
#define MAX_BPM 520
int16_t bpm;

void updateEncoder(){
  long newPosition = enc.read();
  if(newPosition != encPosition){
    encPosition = newPosition;
    menuTimer = millis() + MENU_TIME;
  }
}

void updateControls(){
  for(int i=0; i<NUM_OF_BUTTONS; i++){
    b[i].update();
  }
  if(b[SEQ_0].fell()){
    seq_0 = !seq_0;
    digitalWrite(PIN_LED_0, seq_0); 
  }
  if(b[SEQ_1].fell()){
    seq_1 = !seq_1;
    digitalWrite(PIN_LED_1, seq_1);
  }
  if(b[SET_MENU].fell()){
    menu_number = ++menu_number % MENU_NUMBER;
    encPos_tmp = encPosition;
    menuTimer = millis() + MENU_TIME;
    menuOnScreen = true;
  }

  updateEncoder();

  // calculate actual value
  if(menu_number == PLAY_MODE){
    menuValue = ((encPosition - encPos_tmp)/4 + PLAYMODE_NUMBER) % PLAYMODE_NUMBER;
  } else if(menu_number == RANGE){
    if(seq_1 && !seq_0){
      menuValue = (range_semitones[1] + (encPosition - encPos_tmp)/4 + NUM_OF_STEPS) % NUM_OF_STEPS + 1;
    } else {
      menuValue = (range_semitones[0] + (encPosition - encPos_tmp)/4 + NUM_OF_STEPS) % NUM_OF_STEPS + 1;
    }
  } else if(menu_number == SCALE){
    menuValue = (7 + (encPosition - encPos_tmp)/4 + SCALE_LENGTH) % SCALE_LENGTH;
  } else if(menu_number == POSSIBILIY){ // TODO: schnell/langsam (button pressed?)
    if(seq_1 && !seq_0){
      menuValue = (possibility[1] + (encPosition - encPos_tmp)/4 + MAX_POSSIBILITY) % MAX_POSSIBILITY;
    } else {
      menuValue = (possibility[0] + (encPosition - encPos_tmp)/4 + MAX_POSSIBILITY) % MAX_POSSIBILITY;
    }
  } else if(menu_number == TIMER_MODE){
    menuValue = (encPosition/4 + 2) % 2;
  } else if(menu_number == SPEED){
    menuValue = (bpm + (encPosition - encPos_tmp)/4 + MAX_BPM) % MAX_BPM; // TODO: schnell/langsam (button pressed?)
  } else if(menu_number == SHIFT){
    menuValue = ((encPosition - encPos_tmp)/4 + NUM_OF_STEPS) % NUM_OF_STEPS;
  } else{
    menu_number = 0;
    menuValue = 0;
  }

  if(b[ROTARY_BUTTON].fell()){ // set calculated value
    if(menu_number == PLAY_MODE){
      if(seq_1){
        play_mode[1] = menuValue;
      }
      if(seq_0) {
        play_mode[0] = menuValue;
      }
    } else if(menu_number == RANGE){
      if(seq_1){
        range_semitones[1] = menuValue;
      }
      if(seq_0) {
        range_semitones[0] = menuValue;
      }
    } else if(menu_number == SCALE){
       if(seq_1){
        picked_scale[1] = scales[menuValue];
        scale_pointer[1] = 0;
      }
      if(seq_0) {
        picked_scale[0] = scales[menuValue];
        scale_pointer[0] = 0;        
      }     
    } else if(menu_number == POSSIBILIY){
      if(seq_1){
        possibility[1] = menuValue;
      }
      if(seq_0) {
        possibility[0] = menuValue;
      }
    } else if(menu_number == TIMER_MODE){
      if(seq_1){
        timer_mode[1] = menuValue;
      }
      if(seq_0) {
        timer_mode[0] = menuValue;
      }
    } else if(menu_number == SPEED){
      bpm = menuValue;
      millisToInterrupt = round(60000.0/bpm);
    } else if(menu_number == SHIFT){
      if(seq_1){
        random_step_pointer[1] += menuValue;
      }
      if(seq_0){
        random_step_pointer[0] += menuValue;
      }
    }
  }
}

void buildScreen(){
  if(menu_number == PLAY_MODE){
    printScreen(menuValue, SCR_NUMBER);
  } else if(menu_number == RANGE){
    printScreen(menuValue, SCR_NUMBER);
  } else if(menu_number == SCALE){
    printScreen(menuValue, SCR_NUMBER);
  } else if(menu_number == POSSIBILIY){
    byte val = map(menuValue, 0, MAX_POSSIBILITY-1, 0, 15);
    printScreen(val, SCR_RAISE);
  } else if(menu_number == TIMER_MODE){
    printScreen(menuValue, SCR_TIMER);
  } else if(menu_number == SPEED){
    byte val = map(menuValue, 0, MAX_BPM, 0, 15);
    printScreen(val, SCR_RAISE);
  } else if(menu_number == SHIFT){
    printScreen(menuValue, SCR_NUMBER);
  }
}

byte getRandomNote(byte channel){
  byte tone = random(0, range_semitones[channel]);
  return picked_scale[channel].values[tone % picked_scale[channel].length] + tone / picked_scale[channel].length * 12;
}

byte getRandomDiff(){
    byte rnd = random(0,10);
    byte diff = 0;
    switch(rnd){
      case 0:
        diff = -2;
        break;
      case 1:
      case 2:
        diff = -1;
        break;
      case 3:
        diff = 0;
        break;
      case 4:
      case 5:
      case 6:
        diff = 1;
        break;
      case 7:
      case 8:
        diff = 2;
        break;
      case 9:
        diff = 3;
        break;
      default:
        diff = 0;
        break;
    }
    return diff;
}

void generateNextStep(byte channel){
  // set value_out
  if(play_mode[channel] == RANDOM){
    random_step_pointer[channel] = (random_step_pointer[channel] + 1) % NUM_OF_STEPS;
    if(random(0, MAX_POSSIBILITY) <= possibility[channel]){
      if(channel == 0){
        random_steps0[random_step_pointer[channel]] = getRandomNote(channel);
        value_tone[channel] = random_steps0[random_step_pointer[channel]];
      }else{
        random_steps1[random_step_pointer[channel]] = getRandomNote(channel);
        value_tone[channel] = random_steps1[random_step_pointer[channel]];
      }
    }
  }

  else if(play_mode[channel] == RANDOM_KEEP){
      if(random(0, MAX_POSSIBILITY) <= possibility[channel]){
      if(channel == 0){
        value_tone[channel] = getRandomNote(channel);
      }else{
        value_tone[channel] = getRandomNote(channel);
      }
    }
  }

  else if(play_mode[channel] == RANDOM_AREA){
   random_step_pointer[channel] = (random_step_pointer[channel] + 1) % NUM_OF_STEPS;
    if(random(0, MAX_POSSIBILITY) <= possibility[channel]){
      if(channel == 0){
        value_tone[channel] = random_steps0[random_step_pointer[channel]] + getRandomDiff();
      } else {  
        value_tone[channel] = random_steps1[random_step_pointer[channel]] + getRandomDiff();
      }
      if(value_tone[channel] < 0)
        value_tone[channel] = 0;
      if(value_tone[channel] >= range_semitones[channel])
        value_tone[channel] = range_semitones[channel]-1; 
    }
  }
    
  else if(play_mode[channel] == UP){
    scale_pointer[channel] = (scale_pointer[channel] + 1) % range_semitones[channel];
    value_tone[channel] = picked_scale[channel].values[scale_pointer[channel] % picked_scale[channel].length]
        + 12 * (scale_pointer[channel] / picked_scale[channel].length);
  }

  else if(play_mode[channel] == DOWN){
    if(scale_pointer == 0)
      scale_pointer[channel] = range_semitones[channel]-1;
    else
      --scale_pointer[channel];
    value_tone[channel] = picked_scale[channel].values[scale_pointer[channel] % picked_scale[channel].length]
        + 12 * (scale_pointer[channel] / picked_scale[channel].length);
  }

  else if(play_mode[channel] == SLOWLY_UP){
    byte diff = getRandomDiff();
    byte pointer = scale_pointer[channel] + diff;
    if(pointer <0 || pointer >= range_semitones[channel]){
      pointer = 0;
    }
    scale_pointer[channel] = pointer;
    value_tone[channel] = picked_scale[channel].values[scale_pointer[channel] % picked_scale[channel].length]
        + 12 * (scale_pointer[channel] / picked_scale[channel].length);
  }

  else if(play_mode[channel] == SLOWLY_DOWN){
    byte diff = - getRandomDiff();
    byte pointer = scale_pointer[channel] + diff;
    if(pointer <0 || pointer >= range_semitones[channel]){
      pointer = 0;
    }
    scale_pointer[channel] = pointer;
    value_tone[channel] = picked_scale[channel].values[scale_pointer[channel] % picked_scale[channel].length]
        + 12 * (scale_pointer[channel] / picked_scale[channel].length);
  }

  else if(play_mode[channel] == SLOWLY_BOUNCE){
    byte diff;
    if(bounceUp[channel])
      diff = getRandomDiff();
    else
      diff = - getRandomDiff();

    byte pointer = scale_pointer[channel] + diff;
    if(pointer < 0){
      pointer = 0;
      bounceUp[channel] = true;
    } else if(pointer >= range_semitones[channel]){
      pointer = range_semitones[channel] - 1;
      bounceUp[channel] = false;
    }
    scale_pointer[channel] = pointer;
    value_tone[channel] = picked_scale[channel].values[scale_pointer[channel] % picked_scale[channel].length]
        + 12 * (scale_pointer[channel] / picked_scale[channel].length);
  }

  else if(play_mode[channel] == INPUT){
    byte tone = 0;
    if(channel == 0){
      tone = map(analogRead(PIN_CV_INPUT_0), 0, 1023, 0, 5*picked_scale[channel].length);
    } else {
      tone = map(analogRead(PIN_CV_INPUT_1), 0, 1023, 0, 5*picked_scale[channel].length);
    }
    value_tone[channel] = picked_scale[channel].values[tone % picked_scale[channel].length] + 12 * (tone / picked_scale[channel].length);
  }
}

void writeVoltage(byte channel){
  unsigned int value_write = round(value_tone[channel]*1024/15.0); // *4096 for dac-scale, :5 for 5v, :12 for 12 semitones per octave
  
  if(channel == 0)
    dac0.setVoltage(value_write, false);
  else
    dac1.setVoltage(value_write, false);

  stepMarked[channel] = false;
}

// Input signal interrupt method
void play_tone0(){
  if(timer_mode[0] == TRIG_MODE){
    if(digitalRead(PIN_TRIG0) == HIGH){
      stepMarked[0] = true;
    }
  }
}
void play_tone1(){
  if(timer_mode[1] == TRIG_MODE){
    if(digitalRead(PIN_TRIG1) == HIGH){
      stepMarked[1] = true;
    }
  }
}

void record(){
  if(seq_0){ // TODO: testen
    random_steps0[random_step_pointer[0]] = map(analogRead(PIN_CV_INPUT_0), 0, 1023, 0, 5*picked_scale[0].length);
    if(play_mode[0] != RANDOM)
      random_step_pointer[0]++;
  }
  if(seq_1){
    random_steps1[random_step_pointer[1]] = map(analogRead(PIN_CV_INPUT_1), 0, 1023, 0, 5*picked_scale[1].length);
    if(play_mode[1] != RANDOM)
      random_step_pointer[1]++;
  }
}

void initControls(){
  bpm = 130;
  millisToInterrupt = 462; // 60000/bpm (130 bpm)

  // TODO: check- alle Menüpunkte (pattern-length)
  play_mode[0] = UP;
  timer_mode[0] = CLK_MODE;
  picked_scale[0] = scales[4];
  range_semitones[0] = 10;
  possibility[0] = 10;
  
  play_mode[1] = RANDOM;
  timer_mode[1] = TRIG_MODE;
  picked_scale[1] = scales[1];
  range_semitones[1] = 8;
  possibility[1] = 15;
}

void setup() {
  Serial.begin(9600);
  Serial.println("initialising initialisation");

  // setup digital pins
  pinMode(PIN_TRIG0, INPUT);
  pinMode(PIN_TRIG1, INPUT);
  pinMode(PIN_LED_0, OUTPUT);
  pinMode(PIN_LED_1, OUTPUT);
  pinMode(PIN_SEG_LATCH, OUTPUT);
  pinMode(PIN_SEG_CLK, OUTPUT);
  pinMode(PIN_SEG_DATA, OUTPUT);
  // setup buttons
  b[ROTARY_BUTTON].attach(PIN_ROTARY_BUTTON, INPUT_PULLUP);
  b[SEQ_0].attach(PIN_SETTINGS_0, INPUT_PULLUP);
  b[SEQ_1].attach(PIN_SETTINGS_1, INPUT_PULLUP);
  b[SET_MENU].attach(PIN_SET_MENU, INPUT_PULLUP);
  for(int i=0; i<NUM_OF_BUTTONS; i++){
    b[i].interval(25);
  }

  initControls();

  // generate random pattern
  for(int i=0; i<NUM_OF_STEPS; i++){
    random_steps0[i] = getRandomNote(0);
    random_steps1[i] = getRandomNote(1);
  }
  
  // begins I2C communication
  Wire.begin(); 
  dac0.begin(MCP4725_0);
  dac1.begin(MCP4725_1);
  
  // enable interrupts for trigger input
  attachInterrupt(digitalPinToInterrupt(PIN_TRIG0), play_tone0, CHANGE);
  attachInterrupt(digitalPinToInterrupt(PIN_TRIG1), play_tone1, CHANGE);
  
  Serial.println("init done");
}

/* debug controls
void loop(){
  updateControls();
  debugVar++;
 if(debugVar > 1000){
  debugVar = 0;
  Serial.print("menu=");
  Serial.print(menu_number);
  
  Serial.print("\tPlay_Mode=");
  Serial.print(play_mode[0]);
  Serial.print("\trange=");
  Serial.print(range_semitones[0]);
  Serial.print("\tscale=");
  Serial.print(picked_scale[0].name);
  Serial.print("\tpos=");
  Serial.print(possibility[0]);
  Serial.print("\ttimer=");
  Serial.print(timer_mode[0]);
  Serial.print("\tspeed=");
  Serial.print(bpm);
  
 
  Serial.print("\trotary=");
  Serial.print(encPosition);
  Serial.print("\tValue=");
  Serial.println(menuValue);

 }
}
*/


void loop() {
  updateControls();
  //delay(5);

  // TODO timer_mode ändert sich -> set-timeForNextStep

  // check timer for clock mode
  if(timer_mode == CLK_MODE){
    long timeNow = millis();
    if(timeNow > timeForNextStep){
      timeForNextStep += millisToInterrupt;
      if(timer_mode[0])
        stepMarked[0] = true;
      if(timer_mode[1])
      stepMarked[1] = true;
    }    
  }
  
  if(stepMarked[0] || play_mode[0] == INPUT){
    generateNextStep(0);
    writeVoltage(0);  // TODO: maye tauschen (aktuellere Werte... vs delay wg. Ladezeit)
  }
  if(stepMarked[1] || play_mode[1] == INPUT){
    generateNextStep(1);
    writeVoltage(1);
  } 

  // print everything
  if(millis() < menuTimer){
    if(menuOnScreen){
      printScreen(menu_number, SCR_NUMBER);
    } else {
      buildScreen();
    }
  } else {
    menuOnScreen = false;
    clearScreen();
  }
}