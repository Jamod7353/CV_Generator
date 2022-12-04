#include <Arduino.h>
#include "scales.h"
#include "settings.h"

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
byte play_mode;

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

void updateControls(){
  // read and flattern poti values
  range_AVG[range_AVG_pointer] = (unsigned int) map(analogRead(PIN_RANGE), 0, 1023, 0, 5*picked_scale.length);
  range_AVG_pointer = ++range_AVG_pointer % AVG_LENGTH;
  range_semitones = avg(range_AVG);
  possibility_AVG[possibility_AVG_pointer] = (unsigned int) analogRead(PIN_POSSIBILITY);
  possibility_AVG_pointer = ++possibility_AVG_pointer % AVG_LENGTH;
  possibility = avg(possibility_AVG);
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
    byte tone = map(analogRead(PIN_CV_INPUT), 0, 1023, 0, 5*picked_scale.length);
    value_out = picked_scale.values[tone % picked_scale.length] + 12 * tone / picked_scale.length;
  }
}
void writeVoltage(){
  // TODO: DAC implementieren
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
}

void loop() {
  updateControls();
}

//TODO: Interrupt Methode
// bei raise von Input oder bei eigener clock
// writeVoltage()
// generateNextStep()