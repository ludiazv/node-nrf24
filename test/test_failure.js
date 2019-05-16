'use strict';

const rf=require("../build/Release/nRF24");

var radio=new rf.nRF24(14,0); // Wrong CE pin

var config={PALevel:rf.RF24_PA_LOW,
    DataRate:rf.RF24_1MBPS,
    Channel:7};

radio.begin(false);
radio.config(config,true);
radio.useWritePipe("0x65646f4e41");

if(radio.write(Buffer.from("it should fail")) ) {
    console.log("something went wrong");
} else {
    console.log("Failure detected:" + radio.hasFailure());
}