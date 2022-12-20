#include <Arduino.h>
#include <Adafruit_MCP4725.h>
#include <Wire.h>
#include <Encoder.h>
#include <Bounce2.h>
#include "scales.h"
#include "settings.h"

#define MCP4725_1 0x60
#define MCP4725_2 0x61 //TODO 60 oder 62?
Adafruit_MCP4725 dac1;
Adafruit_MCP4725 dac2;
byte buffer[3];
Encoder enc(PIN_ENC_1, PIN_ENC_2);
long encPosition = -999;

byte range_semitones;
#define NUM_OF_STEPS 16
byte random_steps[NUM_OF_STEPS];
byte random_step_pointer = 0;
int possibility; // value 0-1023
byte scale_pointer = 0;
scale picked_scale = scales[0];

#define AVG_LENGTH 8
unsigned int range_AVG[AVG_LENGTH];
byte range_AVG_pointer = 0;
unsigned int possibility_AVG[AVG_LENGTH];
byte possibility_AVG_pointer = 0;

#define UP 0
#define DOWN 1
#define RANDOM 2
#define INPUT 3
// TODO: more modes (slightly up/down (random -2 to +3))
byte play_mode;

#define CLK_MODE 0
#define TRIG_MODE 1
byte timer_mode;
boolean stepMarked1 = false;
boolean stepMarked2 = false;
long timeForNextStep;

int millisToInterrupt = 462; // 60000/bpm (130 bpm)

byte value_tone = 0; //calculated value -> to scale in volt: value*1/12

unsigned int avg(unsigned int array[]){
  // save some values in an array and calculate avarage to flatten values
  unsigned int sum = 0;
  for(int i=0; i<AVG_LENGTH; i++){
    sum += array[i];
  }
  double value = sum * 1.0 / AVG_LENGTH;
  return round(value);
}

void updateEncoder(){
  long newPosition = enc.read();
  if(newPosition != encPosition){
    encPosition = newPosition;
  }
}

void updateControls(){
  // read and flattern poti values
  range_AVG[range_AVG_pointer] = (unsigned int) map(analogRead(PIN_RANGE), 0, 1023, 0, 5*picked_scale.length);
  range_AVG_pointer = ++range_AVG_pointer % AVG_LENGTH;
  range_semitones = avg(range_AVG);
  possibility_AVG[possibility_AVG_pointer] = (unsigned int) analogRead(PIN_POSSIBILITY);
  possibility_AVG_pointer = ++possibility_AVG_pointer % AVG_LENGTH;
  possibility = avg(possibility_AVG);

  timer_mode = CLK_MODE; // TODO: Switch

  updateEncoder(); // mach was damit... TODO

  /* set bpm TODO: neue Formel...
    float tmp = (65536 - 3750000.0/bpm);
    countsToInterrupt = (int) tmp;
  */
}

byte getRandomNote(){
  byte tone = random(0, range_semitones);
  tone = picked_scale.values[tone % picked_scale.length] + tone / picked_scale.length * 12;
  return tone;
}

void generateNextStep(){ // TODO: Separater Step für output 2
  // set value_out

  if(play_mode == RANDOM){
    random_step_pointer = ++random_step_pointer % NUM_OF_STEPS;
    if(random(0, 1023) <= possibility){
      random_steps[random_step_pointer] = getRandomNote();
    }
    value_tone = random_steps[random_step_pointer];
    
  } else if(play_mode == UP){
    scale_pointer = ++scale_pointer % range_semitones;
    value_tone = picked_scale.values[scale_pointer % picked_scale.length] + 12 * (scale_pointer / picked_scale.length);
  } else if(play_mode == DOWN){
    if(scale_pointer == 0)
      scale_pointer = range_semitones-1;
    else
      --scale_pointer;
    value_tone = picked_scale.values[scale_pointer % picked_scale.length] + 12 * (scale_pointer / picked_scale.length);
  } else if(play_mode == INPUT){
    byte tone = map(analogRead(PIN_CV_INPUT1), 0, 1023, 0, 5*picked_scale.length);
    value_tone = picked_scale.values[tone % picked_scale.length] + 12 * (tone / picked_scale.length);
  }
}

void writeVoltage(){
  // O/P Voltage = (Reference Voltage / Resolution) x Digital Value
  // O/P Voltage = (5/ 4096) x 2048 = 2.5V
  unsigned int value_write = round(value_tone*1024/15.0); // *4096 for dac-scale, :5 for 5v, :12 for 12 semitones per octave
  
  if(stepMarked1){
    dac1.setVoltage(value_write, false);
    stepMarked1 = false;
  }
  if(stepMarked2){
    dac2.setVoltage(value_write, false); // TODO: value_write duplizierien!
    stepMarked2 = false;
  }
}

// Input signal interrupt method
void play_tone1(){
  if(timer_mode == TRIG_MODE){
    if(digitalRead(PIN_TRIG1) == HIGH){
      stepMarked1 = true;
    }
  }
}
void play_tone2(){
  if(timer_mode == TRIG_MODE){
    if(digitalRead(PIN_TRIG1) == HIGH){
      stepMarked2 = true;
    }
  }
}

void setup() {
  Serial.begin(9600);
  Serial.println("das gibts nicht");

  for(int i=0; i<AVG_LENGTH; i++){
    possibility_AVG[i] = (unsigned int) analogRead(PIN_POSSIBILITY);
    range_AVG[i] = (unsigned int) map(analogRead(PIN_RANGE), 0, 1023, 0, 5*picked_scale.length);
  }
  for(int i=0; i<NUM_OF_STEPS; i++){
    random_steps[i] = getRandomNote();
  }
  //updateControls();
  
  Wire.begin(); // begins I2C communication
  attachInterrupt(digitalPinToInterrupt(PIN_TRIG1), play_tone1, CHANGE);
  //attachInterrupt(digitalPinToInterrupt(PIN_TRIG2), play_tone2, CHANGE);

  dac1.begin(MCP4725_1);
  //dac2.begin(MCP4725_2);

  /*
  noInterrupts();
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1 = 0;
  TCCR1B |= (1 << CS12);  // prescale 256
  TIMSK1 |= (1 << TOIE1);
  interrupts();
  */

  Serial.println("fertig");

  // TODO: löschen/ zum Testen
  play_mode = UP;
  timer_mode = TRIG_MODE; // bpm = 130;
  picked_scale = scales[3];
  range_semitones = 10;
  possibility = 900;
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
      stepMarked1 = true;
      stepMarked2 = true;
    }    
  }
  
  if(stepMarked1){
    writeVoltage();
    generateNextStep();
  }
  
}

/*
ISR(TIMER1_OVF_vect){
  TCNT1 = countsToInterrupt;
  writeVoltage(1);
  generateNextStep();
  Serial.println("Hallo");
}
*/
