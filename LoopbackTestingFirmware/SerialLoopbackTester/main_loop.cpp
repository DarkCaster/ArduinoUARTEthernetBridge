#include <Arduino.h>
#include "main_loop.h"
#include "databuffer.h"

static DataBuffer ringBuffer;

void setup()
{
    pinMode(LED_BUILTIN,OUTPUT);
    digitalWrite(LED_BUILTIN,0);
    Serial.begin(250000,SERIAL_8N1);
    digitalWrite(LED_BUILTIN,1);
}

void loop()
{
    //read data from serial to the ring buffer
    auto handle=ringBuffer.GetHead();
    auto buffAvail=handle.maxSz;
    auto serialAvail=Serial.available();
    if(buffAvail>static_cast<unsigned int>(serialAvail))
        buffAvail=static_cast<unsigned int>(serialAvail);
    if(buffAvail>0)
        ringBuffer.Commit(handle,Serial.readBytes(handle.buffer,buffAvail));

    //write data from ring buffer back to the serial
    handle=ringBuffer.GetTail();
    buffAvail=handle.maxSz;
    serialAvail=Serial.availableForWrite();
    if(buffAvail>static_cast<unsigned int>(serialAvail))
        buffAvail=static_cast<unsigned int>(serialAvail);
    if(buffAvail>0)
        ringBuffer.Commit(handle,Serial.write(handle.buffer,buffAvail));
}
