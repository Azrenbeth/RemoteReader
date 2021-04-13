#ifndef _REMOTE_READER_H_
    #define _REMOTE_READER_H_
// ^^ This prevents accidental include loops
// ^^ Keep at the very top of the file

#include "Arduino.h"
#include "HardwareSerial.h"

// How often in microseconds to call the sample function
#define GAP_BETWEEN_SAMPLES 50

// How many transitions in the longest signals
#define MAX_NUMBER_OF_TRANSITIONS 100 

// Sets the minimum length in microseconds of the gap between codes
// This is needed to distinguish betweeen a space betwen pulses and
// the gap between codes
#define MINIMUM_GAP_TIME 5000
#define MIN_GAP_LENGTH (MINIMUM_GAP_TIME/GAP_BETWEEN_SAMPLES)

// The input is 0 when the IR LED is pulsing
#define PULSE 0
#define SPACE 1

// These states are used in the interurupt handler
#define STATE_IDLE     2
#define STATE_PULSING  3
#define STATE_SPACE    4
#define STATE_STOP     5

// information for the interrupt handler
typedef struct {
  // pin for IR data from detector
  uint8_t receiverPin;          

  // the current state
  uint8_t receiverState;

  // increments on every interrupt
  unsigned int timer;   

  // stores length of pulses and spaces
  unsigned int timings[MAX_NUMBER_OF_TRANSITIONS]; 

  // how many entries there are in timings buffer
  uint8_t length;               
} 
irparams_t;

// This will store the information from one code
class IRCode {
public:
    // stores length of pulses and spaces
    volatile unsigned int *timings;

    // how many entries there are in timings buffer
    int length;
};

// This class abstracts away the code reading
class RemoteReader
{
public:
    // Constructor to set the input pin number
    RemoteReader(int receiverPin);
    // Sets up the interrupt timer to trigger sampleInput()
    void enableIRIn();

    // Copies the code information from irparams to results
    int readCode(IRCode *results);

public:
    // Prints out the timing data to the serial port
    static void sendToSerial(IRCode *results);
  
private:
    // Indicates that new codes can be read
    void resume();
};

// vv Part of the "#ifndef _REMOTE_READER_H_" at the top
// vv Keep at the very end of the file
#endif