#include <Arduino.h>
#include <Adafruit_MCP4725.h>
#include <Wire.h>
#include <Encoder.h>
#include "scales.h"
#include "settings.h"

#define MCP4725_1 0x61
#define MCP4725_2 0x60 //TODO 60 oder 62?
byte buffer[3];
unsigned int adc;
Encoder enc(PIN_ENC_1, PIN_ENC_2);
long encPosition = -999;

byte range_semitones;
#define NUM_OF_STEPS 8
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

int countsToInterrupt = 36690;

byte value_out; //calculated value -> to scale in volt: value*1/12

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

  /* set bpm
    float tmp = (65536 - 3750000.0/bpm);
    countsToInterrupt = (int) tmp;
  */
}

byte getRandomNote(){
  byte tone = random(0, range_semitones);
  tone = picked_scale.values[tone % picked_scale.length] + tone / picked_scale.length * 12;
  return tone;
}

void generateNextStep(){
  // set value_out
  if(play_mode == RANDOM){
    if(random(0, 1023) <= possibility){
      random_steps[(random_step_pointer++) % NUM_OF_STEPS] = getRandomNote();
      value_out = random_steps[random_step_pointer];
    }
  } else if(play_mode == UP){
    scale_pointer = (scale_pointer + 1) % range_semitones;
    value_out = picked_scale.values[scale_pointer % picked_scale.length] + 12 * scale_pointer / picked_scale.length;
  } else if(play_mode == DOWN){
    scale_pointer = (scale_pointer - 1) % range_semitones; // TODO: check negative modulo 
    value_out = picked_scale.values[scale_pointer % picked_scale.length] + 12 * scale_pointer / picked_scale.length;
  } else if(play_mode == INPUT){
    byte tone = map(analogRead(PIN_CV_INPUT1), 0, 1023, 0, 5*picked_scale.length);
    value_out = picked_scale.values[tone % picked_scale.length] + 12 * tone / picked_scale.length;
  }
}

void writeVoltage(int output){
  // TODO: Testen
  buffer[0] = 0b01000000;
  adc = value_out; // TODO richtige Formel!
  // O/P Voltage = (Reference Voltage / Resolution) x Digital Value
  // O/P Voltage = (5/ 4096) x 2048 = 2.5V
  float ipvolt = (5.0/4096.0)*adc;
  buffer[1] = adc >> 4;
  buffer[2] = adc << 4;

  if(output == 1){
    Wire.beginTransmission(MCP4725_1);
  } else{
    Wire.beginTransmission(MCP4725_2);
  }
  
  Wire.write(buffer[0]);
  Wire.write(buffer[1]);
  Wire.write(buffer[2]);
  Wire.endTransmission();
}

// Input signal interrupt method
void play_tone1(){
  // TODO: implementieren ?
  writeVoltage(1);
  generateNextStep();
}
void play_tone2(){
  writeVoltage(2);
}

void setup() {
  for(int i=0; i<AVG_LENGTH; i++){
    possibility_AVG[i] = (unsigned int) analogRead(PIN_POSSIBILITY);
    range_AVG[i] = (unsigned int) map(analogRead(PIN_RANGE), 0, 1023, 0, 5*picked_scale.length);
  }
  updateControls();
  for(int i=0; i<NUM_OF_STEPS; i++){
    random_steps[i] = getRandomNote();
  }
  Wire.begin(); // begins I2C communication
  attachInterrupt(digitalPinToInterrupt(PIN_CLK_INPUT1), play_tone1, CHANGE);
  attachInterrupt(digitalPinToInterrupt(PIN_CLK_INPUT2), play_tone2, CHANGE);

  noInterrupts();
  TCCR1A = 0;
  TCNT1 = 0;
  TCCR1B |= (1 << CS12);  // prescale 256
  TIMSK1 |= (1 << TOIE1);
  interrupts();
}

void loop() {
  updateControls();
}

ISR(TIMER1_OVF_vect){
  TCNT1 = countsToInterrupt;
  // TODO: implementieren ?
  writeVoltage(1);
  generateNextStep();
}

//TODO: Interrupt Methode
// bei raise von Input oder bei eigener clock