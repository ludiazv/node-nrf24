'use strict';
const rf=require("../build/Release/nRF24");
// Use this constructor with SPIDEV in Opi zero
//var rfo=new rf.nRF24(6,10);

// Std contructur Rpi
//var rfo=new rf.nRF24(25,0);

// Constructor for Boros RF2 radio 0
var rfo=new rf.nRF24(24,0);

var addresses=["0x65646f4e31","0x65646f4e32","0x65646f4e33"];
var raddresses=["0x72646f4e31","0x72646f4e32","0x72646f4e33"];

console.log("node-rf24 Testing");
console.log(`This processor architecture is ${process.arch}`);
console.log("This test configure the node-rf24 in 1Node Pipe.");
console.log("Starting RADIO  ->" + rfo.begin(true));
console.log("Chip is present ->" + rfo.present());
// minimal config
rfo.config({PALevel:rf.RF24_PA_MAX,DataRate:rf.RF24_1MBPS});

var pipes=[];
for(var i=1;i<addresses.length;i++) {
  pipes[i-1]=rfo.addReadPipe(addresses[i],true); // "2Node" + autoACK
  console.log("Opened reading pipe " + pipes[i-1] + " for address:" + addresses[i]);
}
var role=0;
if(process.argv.length >2) role=parseInt(process.argv[2]);

if(role==0) {
  console.log("Waiting incoming frames...");
  var count=0;
  rfo.read(function(d,pipe) {
    rfo.useWritePipe(raddresses[pipe]);
    rfo.write(d); // Pong back
    var n=d.readUInt32LE(0);
    console.log("<" + count + ">Read on pipe " + pipe + " ["+raddresses[pipe]+"] value ->" + n);
    count=count+1;
  },
   function() { console.log("stopped");}
  )
  // Rec
}
if(role==1) {
  console.log("Sending frames...");
  var count=0;
  setInterval(function() {
    var data=Buffer.alloc(4);
    var tmstp=Math.round(+new Date()/1000);
    data.writeUInt32LE(tmstp);
    for(var i=1;i<raddresses.length;i++) {
      rfo.useWritePipe(raddresses[i]);
      console.log("<"+count+">" + " Write ["+ tmstp +"] to " + raddresses[i] + "->" + rfo.write(data));
    }
    count++;
    //if(count>10) rfo.stop_read();
  } , 2000);
  rfo.read(function(d,pipe) {
      var n=d.readUInt32LE(0);
      console.log("Read response from pipe " + pipe + " address:" + addresses[pipe] + " -> " + n);
  },
  function() { console.log("stopped"); }
  );

}
console.log("Ready!");
