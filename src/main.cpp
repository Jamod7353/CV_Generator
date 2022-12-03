#include <Arduino.h>
#include "scales.h"
#include "settings.h"

byte range_semitones = 12;
#define NUM_OF_STEPS 16
byte steps[NUM_OF_STEPS];
byte step_pointer = 0;
int possibility;
scale picked_scale = scales[0];

void updateControls(){
  // TODO: Code
}

byte getRandomNote(){
  byte octaves = range_semitones/12;
  byte tone = 255;
  while (tone > range_semitones){
    tone = picked_scale.values[(0, picked_scale.length)] + 12*random(0, octaves); //TODO: Error? ggf. random(0,0) ausschließen
  }
  return tone;
}

void generateNextStep(){
  if(random(0, 1023) <= possibility){
    steps[(step_pointer+1) % NUM_OF_STEPS] = getRandomNote();
  }
}

void setup() {
  updateControls();
}

void loop() {
  updateControls();
  
}

//TODO: Interrupt Methode
// bei raise von Input
// nextTone()