//#define SCL A5 // I2C
//#define SDA A4 // I2C

// Input Jacks
#define PIN_CV_INPUT_1 A1
#define PIN_CV_INPUT_0 A0

// Input Jacks
#define PIN_TRIG0 2
#define PIN_TRIG1 3

// rotary encoder
#define PIN_ENC_0 4
#define PIN_ENC_1 5
#define PIN_ROTARY_BUTTON 6  // Pullup Input

// buttons - settings 1 and 2
#define PIN_SETTINGS_0 7  // Pullup Input
#define PIN_SETTINGS_1 8  // Pullup Input
#define PIN_SET_MENU 9    // Pullup Input

// LED
#define PIN_LED_0 10
#define PIN_LED_1 11

// 7 segment
#define PIN_SEG_LATCH A2   // IC:Pin 12
#define PIN_SEG_DATA A3     // IC:Pin 14 / Ard:Pin TX 
#define PIN_SEG_CLK 12     // IC:Pin 11

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
  GND - 3,8 + Resistor 330

Segment 5161AS
10-9-8-7-6
  .a.
 f   b
  .g.
 e   c
  .d.  dp
1-2-3-4-5

*/


/*
Potis:
    possibility
    range (steps)
    scale
    tone mode (incl. input)
    random/save length
    speed/ shift(?)

Buttons:
    set1
    set2
    record (over random array)

Jacks in:
    trigger1
    trigger2
    cv_in1
    cv_in2

Jacks out:
    out1
    out2
*/