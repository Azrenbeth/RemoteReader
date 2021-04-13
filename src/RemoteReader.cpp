#include "RemoteReader.h"
#include "esp32-hal-timer.h"
#include "HardwareSerial.h"

// the irparams_t struct that the interrupt code can access
volatile irparams_t irparams;

// the IRAM_ATTR ensures that this code is always kept in 
// RAM not on the Flash storage (so that it can start fast)
void IRAM_ATTR sampleInput()
{
    // this reads the current value from the IR photransistor
    // it's cast to uint8_t so that comparisons are quicker
    uint8_t irdata = (uint8_t)digitalRead(irparams.receiverPin);

    // increments timer to keep track of how long in this state 
    irparams.timer++; 

    // Checks to see if there has been a bufferoverflow
    if (irparams.length >= MAX_NUMBER_OF_TRANSITIONS) {
        irparams.receiverState = STATE_STOP;
    }

    // The state machine code
    switch(irparams.receiverState) {

    // Haven't yet detected a pulse since started looking
    case STATE_IDLE: 
        if (irdata == PULSE) {		
            // Ensure pulse not part of the last code
            if (irparams.timer < MIN_GAP_LENGTH) {
                irparams.timer = 0;
            }

            // A new code has started to be sent
            else {
                // length is 0 as start of code
                irparams.length = 0;
                // reset timer
                irparams.timer = 0;
                // update state
                irparams.receiverState = STATE_PULSING;
            }
        }
        break;

    // The remote was pulsing on last interrupt
    case STATE_PULSING: 
        // PULSE ended
        if (irdata == SPACE) {   
            // save how long the pulse lasted
            irparams.timings[irparams.length++] = irparams.timer;
            // reset timer
            irparams.timer = 0;
            // uodate state
            irparams.receiverState = STATE_SPACE;
        }
        // otherwise still pulsing
        break; 

    // The remote wasn't transmitting on last interrupt    
    case STATE_SPACE:
        // SPACE ended
        if (irdata == PULSE) {
            // save how long the space lasted
            irparams.timings[irparams.length++] = irparams.timer;
            // reset timer
            irparams.timer = 0;
            // update state
            irparams.receiverState = STATE_PULSING;
        } 
        // SPACE not ended
        else {
            // check whether the space has been long enough to
            // be the gap after the end of a code
            if (irparams.timer > MIN_GAP_LENGTH) {
                // set state to STOP to indicate end of code
                irparams.receiverState = STATE_STOP;
            } 
        }
        break;

    // Not looking for a code any more
    // Can now read the data from the timings buffer
    // Set the state to IDLE to restart
    case STATE_STOP: 
        // If pulses take place reset the timer
        // Needed to prevent reading part-codes after restart
        if (irdata == PULSE) { 
            irparams.timer = 0;
        }
        break;
    }
}

// A hardware timer that we'll setup to trigger sampleInput() 
hw_timer_t * timer = NULL;

// The constructor - used to set the input pin number
RemoteReader::RemoteReader(int receiverPin)
{
    irparams.receiverPin = receiverPin;
}

// Starts the timer that triggers sampleInput()
void RemoteReader::enableIRIn()
{
    // Mark the input pin as an input pin
    pinMode(irparams.receiverPin, INPUT);

    //Creates a timer that increments every 80 APB_CLK cycles
    //The APB_CLK on the esp32 has a default speed of 80MHz
    //So this timer increments every 1 microsecond
    timer = timerBegin(0, 80, true);

    // Attach the sample function to our timer 
    timerAttachInterrupt(timer, &sampleInput, true);

    // Trigger the interupt every GAP_BETWEEN_SAMPLES microsecs
    // "true" indicates that this timer should repeat
    timerAlarmWrite(timer, GAP_BETWEEN_SAMPLES, true);

    // Setup initial values in irparams
    irparams.timer = 0;
    irparams.length = 0;
    irparams.receiverState = STATE_IDLE;

    // Enables the interrupts
    timerAlarmEnable(timer);
    // sampleInput() is now being called every interrupt!!!
}

// Used to indicate that new codes can now be read
void RemoteReader::resume()
{
    // reset irparam values to initial state
    irparams.length = 0;
    irparams.receiverState = STATE_IDLE;
    // note that timer not set to 0 as have been
    // keeping track of time since last pulse
}

// Decodes the received IR message
// Returns 0 if no data ready, 1 if data ready.
// Results of decoding are stored in results
int RemoteReader::readCode(IRCode *results)
{   
    // has a complete code been read in
    if (irparams.receiverState != STATE_STOP) {
        // failed to store data
        return 0;
    }
    // data is ready to be read
    // copy the lengths of pulses and spaces to results
    results->timings = irparams.timings;
    results->length = irparams.length;

    // indicate that new data can now be read
    resume();

    // successfully stored data
    return 1;
}

// Prints out the timing data to the serial port
void RemoteReader::sendToSerial(IRCode *results)
{
    // Send the number of pulses and spaces in the code
    int count = results->length;
    Serial.print("Size of transmission: ");
    Serial.println(count);

    // Now send the length of each of these
    for (int i = 0; i < count; i++) {
        // The timings alternate between pulses and spaces
        // Pulses will be indicated by positive numbers
        // Spaces will the indicated by negative ones
        if ((i % 2) == 0) {
            // is a pulse duration
            Serial.print(results->timings[i]*GAP_BETWEEN_SAMPLES, DEC);
        } 
        else {
            // is a space duration
            Serial.print(-(int)results->timings[i]*GAP_BETWEEN_SAMPLES, DEC);
        }
        // adds spacing between the numbers
        Serial.print(" ");
    }

    // sends a newline character at end of the code
    Serial.println("");
}

