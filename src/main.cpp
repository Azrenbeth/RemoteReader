#include "Arduino.h"
#include "HardwareSerial.h"
#include "RemoteReader.h"

// the pin you have the input going to
#define RECVPIN 5

// a RemoteReader that uses this pin
RemoteReader reader(RECVPIN);

// the object to store the remote code timings in
IRCode code;

// Called on startup
void setup()
{
    // setup the serial port
    Serial.begin(9600);
    // start reading codes 
    reader.enableIRIn();
}

// run's in a loop forever
void loop()
{
    // spins until readCode = 1
    // i.e. that a code has been read
    if(reader.readCode(&code))
    {
        // sends the code information to the serial port
        RemoteReader::sendToSerial(&code);
    }
}