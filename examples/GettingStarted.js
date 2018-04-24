'use strict';

const rf=require("../build/Release/nRF24");
const readline = require('readline');

var radio=new rf.nRF24(24,0);

radio.begin(false);

var ready=radio.present();
if(!ready) {
  console.log("ERROR:Radio is not ready!");
  process.exit(1);
}

var config={PALevel:rf.RF24_PA_LOW,
            DataRate:rf.RF24_1MBPS,
            Channel:76};


radio.config(config,true);

var Pipes= ["0x65646f4e31","0x65646f4e32"];

function rcv(){

  var rpipe=radio.addReadPipe(Pipes[0]);
  console.log("Read Pipe "+ rpipe + " opened for " + Pipes[0]);
  radio.useWritePipe(Pipes[1]);
  radio.read(function(d,items) {
    for(var i=0;i<items;i++){
      console.log("Payload Rcv, replied:" + radio.write(d[i].data));
    }
  },function() { console.log("STOP!"); });

}

function snd() {

  radio.useWritePipe(Pipes[1]);
  radio.addReadPipe(Pipes[0]);
  console.log("Open Write Pipe "+ Pipes[0]);

  var tmstp;
  var roundtrip;
  var sender=function(){
    var data=Buffer.alloc(4);
    tmstp=Math.round(+new Date()/1000);
    data.writeUInt32LE(tmstp);
    process.stdout.write("\nSending....");
    roundtrip=process.hrtime();
    radio.write(data,function(success){
      if(success) process.stdout.write("Sended " + tmstp);
      else process.stdout.write("Failed to send " + tmstp +" ");
    });
  };
  radio.read(function(d,n) {
    if(n==1){
      let t=process.hrtime(roundtrip);
      process.stdout.write(" | response received |")
      let ret=d[0].data.readUInt32LE(0);
      if( ret == tmstp) process.stdout.write(" reponse matchs ");
      else process.stdout.write(" response does not match ");
      console.log("| roundtrip took", (t[1] /1000).toFixed(0)," us");
    }
  },function(){ console.log("STOPPED!"); });

  setInterval(sender,1000); // send every 1 sec.

}

const rl = readline.createInterface({
  input: process.stdin,
  output: process.stdout
});

console.log("RF24 GettingStarted");
console.log("This script assumes that this radio is radionumber=1 in ardunino.");
console.log("Other radio must be configured with radionumber=0")
rl.question("[s]ender or [R]eceiver?",(resp)=>{
  var mode=resp.charAt(0);
  console.log("Running... press CTRL+C to stop test");
  if(mode.toUpperCase()=="S") snd(); else rcv();
  rl.close();
});
