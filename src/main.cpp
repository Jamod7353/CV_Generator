#include <Arduino.h>
#include <Adafruit_MCP4725.h>
#include <Wire.h>
#include <Encoder.h>
#include <Bounce2.h>
#include "scales.h"
#include "settings.h"

#define MCP4725_0 0x60
#define MCP4725_1 0x61 //TODO 60 oder 62?
Adafruit_MCP4725 dac0;
Adafruit_MCP4725 dac1;
byte buffer[3];
Encoder enc(PIN_ENC_0, PIN_ENC_1);
long encPosition = -999;

#define NUM_OF_STEPS 16  // Array length for random/record
byte range_semitones[2] = {NUM_OF_STEPS, NUM_OF_STEPS};
byte random_steps0[NUM_OF_STEPS];
byte random_steps1[NUM_OF_STEPS];
byte random_step_pointer[2] = {0, 0};
int possibility[2] = {0, 0}; // value 0-1023
byte scale_pointer[2] = {0, 0};
scale picked_scale[2] = {scales[0], scales[0]};

#define UP 0
#define DOWN 1
#define RANDOM 2
#define SLOWLY_UP 3
#define SLOWLY_DOWN 4
#define SLOWLY_BOUNCE 5
#define INPUT 6
// TODO: more modes (slightly up/down (random -2 to +3))
byte play_mode[2];

#define CLK_MODE 0
#define TRIG_MODE 1
byte timer_mode[2] = {1, 1}; // TODO: Wert initialisieren und hier löschen
boolean stepMarked[2] = {false, false};

long timeForNextStep;
int millisToInterrupt = 462; // 60000/bpm (130 bpm)

byte value_tone[2] = {0, 0}; //calculated value -> to scale in volt: value*1/12

void updateEncoder(){
  long newPosition = enc.read();
  if(newPosition != encPosition){
    encPosition = newPosition;
  }
}

void updateControls(){
  timer_mode[0] = CLK_MODE; // TODO: Switch

  updateEncoder(); // mach was damit... TODO

  /* set bpm TODO: neue Formel...
    bpm = ...;
    float tmp = 60000/bpm;
    millisToInterrupt = (int) tmp;
  */
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

void generateNextStep(byte channel){ // TODO: Separater Step für output 2
  // set value_out

  if(play_mode[channel] == RANDOM){
    random_step_pointer[channel] = (random_step_pointer[channel] + 1) % NUM_OF_STEPS;
    if(random(0, 1023) <= possibility[channel]){
      if(channel == 0){
        random_steps0[random_step_pointer[channel]] = getRandomNote(channel);
        value_tone[channel] = random_steps0[random_step_pointer[channel]];
      }else{
        random_steps1[random_step_pointer[channel]] = getRandomNote(channel);
        value_tone[channel] = random_steps1[random_step_pointer[channel]];
      }
    }
    
  } else if(play_mode[channel] == UP){
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
    if(pointer <0 || pointer > range_semitones[channel]){
      pointer = 0;
    }
    scale_pointer[channel] = pointer % range_semitones[channel];
    value_tone[channel] = picked_scale[channel].values[scale_pointer[channel] % picked_scale[channel].length]
        + 12 * (scale_pointer[channel] / picked_scale[channel].length);
  }
  else if(play_mode[channel] == SLOWLY_DOWN){
    // TODO: copy slowly up (inverted)
  }
  else if(play_mode[channel] == SLOWLY_BOUNCE){
    // TODO: copy with up or down mode + globale variable
  }
  else if(play_mode[channel] == INPUT){
    byte tone = 0;
    if(channel == 0){
      tone = map(analogRead(PIN_CV_INPUT0), 0, 1023, 0, 5*picked_scale[channel].length);
    } else {
      tone = map(analogRead(PIN_CV_INPUT1), 0, 1023, 0, 5*picked_scale[channel].length);
    }
    value_tone[channel] = picked_scale[channel].values[tone % picked_scale[channel].length] + 12 * (tone / picked_scale[channel].length);
  }
}

void writeVoltage(byte channel){
  // O/P Voltage = (Reference Voltage / Resolution) x Digital Value
  // O/P Voltage = (5/ 4096) x 2048 = 2.5V
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

void setup() {
  Serial.begin(9600);
  Serial.println("das gibts nicht");

  for(int i=0; i<NUM_OF_STEPS; i++){
    random_steps0[i] = getRandomNote(0);
    random_steps1[i] = getRandomNote(1);
  }
  //updateControls();
  
  Wire.begin(); // begins I2C communication
  attachInterrupt(digitalPinToInterrupt(PIN_TRIG0), play_tone0, CHANGE);
  attachInterrupt(digitalPinToInterrupt(PIN_TRIG1), play_tone1, CHANGE);

  dac0.begin(MCP4725_0);
  dac1.begin(MCP4725_1);

  Serial.println("fertig");

  // TODO: löschen/ zum Testen
  play_mode[0] = UP;
  timer_mode[0] = TRIG_MODE; // bpm = 130;
  picked_scale[0] = scales[3];
  range_semitones[0] = 10;
  possibility[0] = 900;
}

void loop() {
  //updateControls();
  delay(5);

  // TODO timer_mode ändert sich -> set-timeForNextStep
  if(timer_mode == CLK_MODE){
    long timeNow = millis();
    if(timeNow > timeForNextStep){
      timeForNextStep += millisToInterrupt;
      // TODO: abhängig von der Wahl: Mark setzen
      stepMarked[0] = true;
      stepMarked[1] = true;
    }    
  }
  

  if(stepMarked[0] || play_mode[0] == INPUT){
    writeVoltage(0);
    generateNextStep(0);
  }
  if(stepMarked[1] || play_mode[1] == INPUT){
    writeVoltage(1);
    generateNextStep(1);
  } 
}
