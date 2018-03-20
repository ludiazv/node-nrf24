
#include <Arduino.h>
#include <SPI.h>
#include "RF24.h"

#if defined(ARDUINO_ARCH_ESP8266) || defined(ESP8266)
  RF24 radio=RF24(2,15);
  #define ADDR_PIN  0
  #define BOARD "ESP88266"
#else // Asume is AVR
  #include <printf.h>
  #if defined(NANO)
    RF24 radio=RF24(7,8);
    #define BOARD "AVR-NANO"
  #else
    RF24 radio=RF24(7,8);
    #define BOARD "AVR"
  #endif
  #define ADDR_PIN  3
#endif

/****************** User Config ***************************/
/***      Set this radio as radio number 0 or 1         ***/
uint8_t radioNumber = 0;

bool role = 0; // RCV

// 1Node is the node-rf.
byte addresses[][6] = {"1Node","2Node","3Node"};
byte raddresses[][6] ={"1Nodr","2Nodr","3Nodr"};

void setup() {
    // put your setup code here, to run once:
    Serial.begin(115200);
    Serial.println(F("RF24 simple / Based on Gettingstarted"));
    Serial.print(F("MODEL:")); Serial.println(BOARD);
    Serial.println(F("This scripts send or receive to/from node-rf24 is running"));
    Serial.println(F("Reading D3 (IO0 on ESP8266) to define pipe address"));

    pinMode(ADDR_PIN,INPUT);
    radioNumber=(digitalRead(ADDR_PIN)) ? 2 : 1;

    Serial.print(F("Write to:")); Serial.println((char *)addresses[radioNumber]);
    Serial.print(F("Read in:")); Serial.println((char *)raddresses[radioNumber]);

    radio.begin();
    Serial.print(F("Radio Present:")); Serial.println(radio.isChipConnected());
    Serial.print(F("is + variant:")); Serial.println(radio.isPVariant());
    radio.setPALevel(RF24_PA_LOW);
    radio.setAutoAck(true);
    radio.setRetries(15, 15);


    radio.openWritingPipe(addresses[radioNumber]);
    radio.openReadingPipe(1, raddresses[radioNumber]);


    radio.startListening();
    radio.printDetails();
    Serial.println(F("*** PRESS 'T' to begin transmitting to 1Node"));


}

void loop() {


/****************** Ping Out Role ***************************/
if (role == 1)  {

  radio.stopListening();                                    // First, stop listening so we can talk.


  Serial.println(F("Now sending"));

  unsigned long start_time = micros();                             // Take the time, and send it.  This will block until complete
   if (!radio.write( &start_time, sizeof(unsigned long) )){
     Serial.println(F("failed"));
   }

  radio.startListening();                                    // Now, continue listening

  unsigned long started_waiting_at = micros();               // Set up a timeout period, get the current microseconds
  boolean timeout = false;                                   // Set up a variable to indicate if a response was received or not

  while ( ! radio.available() ){                             // While nothing is received
    if (micros() - started_waiting_at > 200000 ){            // If waited longer than 200ms, indicate timeout and exit while loop
        timeout = true;
        break;
    }
  }

  if ( timeout ){                                             // Describe the results
      Serial.println(F("Failed, response timed out."));
  }else{
      unsigned long got_time;                                 // Grab the response, compare, and send to debugging spew
      radio.read( &got_time, sizeof(unsigned long) );
      unsigned long end_time = micros();

      // Spew it
      Serial.print(F("Sent "));
      Serial.print(start_time);
      Serial.print(F(", Got response "));
      Serial.print(got_time);
      Serial.print(F(", Round-trip delay "));
      Serial.print(end_time-start_time);
      Serial.println(F(" microseconds"));
  }

  // Try again 1s later
  delay(1000);
}



/****************** Pong Back Role ***************************/

if ( role == 0 )
{
  unsigned long got_time;

  if( radio.available()){
                                                                  // Variable for the received timestamp
    while (radio.available()) {                                   // While there is data ready
      radio.read( &got_time, sizeof(unsigned long) );             // Get the payload
    }

    radio.stopListening();                                        // First, stop listening so we can talk
    radio.write( &got_time, sizeof(unsigned long) );              // Send the final one back.
    radio.startListening();                                       // Now, resume listening so we catch the next packets.
    Serial.print(F("Sent response "));
    Serial.println(got_time);
 }
}




/****************** Change Roles via Serial Commands ***************************/

if ( Serial.available() )
{
  char c = toupper(Serial.read());
  if ( c == 'T' && role == 0 ){
    Serial.println(F("*** CHANGING TO TRANSMIT ROLE -- PRESS 'R' TO SWITCH BACK"));
    role = 1;                  // Become the primary transmitter (ping out)

 }else
  if ( c == 'R' && role == 1 ){
    Serial.println(F("*** CHANGING TO RECEIVE ROLE -- PRESS 'T' TO SWITCH BACK"));
     role = 0;                // Become the primary receiver (pong back)
     radio.startListening();

  }
}

}
