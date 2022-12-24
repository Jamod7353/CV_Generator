#include <Arduino.h>
#include "settings.h"

#define SCR_NUMBER 0
#define SCR_RAISE 1
#define SCR_TIMER 2

void printScreen(int16_t number, byte mode){
    byte out = 0;
    if(mode == SCR_NUMBER){
        if(number > 9){
            out = 0B00000001;
        }
        number = number % 10;
        switch (number)
        {
        case 0:
            out |= 0B11111100;
            break;
        case 1:
            out |= 0B01100000;
            break;
        case 2:
            out |= 0B11011010;
            break;
        case 3:
            out |= 0B11110010;
            break;    
        case 4:
            out |= 0B01100110;
            break;
        case 5:
            out |= 0B10110110;
            break;
        case 6:
            out |= 0B10111110;
            break;
        case 7:
            out |= 0B11100000;
            break;
        case 8:
            out |= 0B111111110;
            break;
        case 9:
            out |= 0B11110110;
            break;
        default:
            break;
        }

    } else if(mode == SCR_TIMER) {
        if(number == 0){
            out = 0B10011100;
            //adefg
        } else {
            out = 0B00001100;
            //ef
        }
    } else if(mode == SCR_RAISE){

        switch (number)
        {
        case 0:
            out = 0;
            break;
        case 1:
            out = 0B00010000;
            break;
        case 2:
            out = 0B00011000;
            break;
        case 3:
            out = 0B00111000;
            break;
        case 4:
            out = 0B00111010;
            break; 
        case 5:
            out = 0B00111110;
            break;  
        case 6:
            out = 0B01111110;
            break;   
        case 7:
            out = 0B11111110;
            break; 
        case 8:
            out = 0B11111111;
            break;
        case 9:
            out = 0B11101111;
            break;  
        case 10:
            out = 0B11100111;
            break;   
        case 11:
            out = 0B11000111;
            break;   
        case 12:
            out = 0B11000101;
            break;
        case 13:
            out = 0B11000001;
            break;
        case 14:
            out = 0B10000001;
            break;
        case 15:
            out = 0B00000001;
            break;             
        default:
            out = 0;
            break;
        }
    }

    // write out to 7-Segment
    digitalWrite(PIN_SEG_LATCH, LOW);
    shiftOut(PIN_SEG_DATA, PIN_SEG_CLK, LSBFIRST, out);
    digitalWrite(PIN_SEG_LATCH, HIGH);
}

void clearScreen(){
    digitalWrite(PIN_SEG_LATCH, LOW);
    shiftOut(PIN_SEG_DATA, PIN_SEG_CLK, LSBFIRST, 0);
    digitalWrite(PIN_SEG_LATCH, HIGH);
}

/*
74HC595 <-> 7 Segment Pins
Q0 15 - 7  a
Q1  1 - 6  b
Q2  2 - 4  c
Q3  3 - 2  d
Q4  4 - 1  e
Q5  5 - 9  f
Q6  6 - 10 g
Q7  7 - 5  dp
  GND - 3,8 with Resistor 330

Segment 5161AS
10-9-8-7-6
  .a.
 f   b
  .g.
 e   c
  .d.  dp
1-2-3-4-5

*/