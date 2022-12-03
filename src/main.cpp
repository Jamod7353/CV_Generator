#include <Arduino.h>
#include "scales.h"
#include "settings.h"

byte range_semitones = 1;
#define NUM_OF_STEPS 16
byte random_steps[NUM_OF_STEPS];
byte random_step_pointer = 0;
byte scale_pointer = 0;
int possibility;
scale picked_scale = scales[0];

#define UP 0
#define DOWN 1
#define RANDOM 2
byte play_mode;

byte value_out; //calculated value -> to scale in volt: value*1/12

void updateControls(){
  // TODO: Code
  range_semitones = map(analogRead(PIN_POSSIBILITY), 0, 1023, 0, 5*picked_scale.length); // TODO: glätten
  // TODO: possibility = ...
}

byte getRandomNote(){
  byte tone = random(0, range_semitones);
  tone = picked_scale.values[tone % picked_scale.length] + tone / picked_scale.length * 12;
  return tone;
}

void generateNextStep(){
  if(play_mode == RANDOM){
    if(random(0, 1023) <= possibility){
      value_out = random_steps[(random_step_pointer+1) % NUM_OF_STEPS] = getRandomNote(); 
    }
  } else if(play_mode == UP){
    scale_pointer = (scale_pointer + 1) % range_semitones;
    value_out = picked_scale.values[scale_pointer % picked_scale.length] + 12 * scale_pointer / picked_scale.length;
  } else if(play_mode == DOWN){
    scale_pointer = (scale_pointer - 1) % range_semitones; // TODO: check: negative modulo 
    value_out = picked_scale.values[scale_pointer % picked_scale.length] + 12 * scale_pointer / picked_scale.length;
  }
}

void setup() {
  updateControls();
  for(int i=0; i<NUM_OF_STEPS; i++){
    random_steps[i] = getRandomNote();
  }
}

void loop() {
  updateControls();
  //TODO: writeVoltage()
}

//TODO: Interrupt Methode
// bei raise von Input
// generateNextStep()