'use strict';

const rf=require("../build/Release/nRF24");
const readline = require('readline');

const CE=24,CS=0;
//const CE=25,CS1=1;
const IRQ=-1;
//const IRQ=27;
console.log("Cofiguration pins CE->",CE," CS->",CS," IRQ->",IRQ);
var radio=new rf.nRF24(CE,CS);

radio.begin(false);

var ready=radio.present();
if(!ready) {
  console.log("ERROR:Radio is not ready!");
  process.exit(1);
}

var config={PALevel:rf.RF24_PA_MIN,
            EnableLna: false,
            DataRate:rf.RF24_1MBPS,
            Channel:76,
            AutoAck:true,
            Irq: IRQ
          };


radio.config(config,true);

var Pipes= ["0x65646f4e31","0x65646f4e32"]; // "1Node", "2Node"

function rcv(){

  var rpipe=radio.addReadPipe(Pipes[0]);
  console.log("Read Pipe "+ rpipe + " opened for " + Pipes[0]);
  radio.useWritePipe(Pipes[1]);
  radio.read(function(d,items) {
    for(var i=0;i<items;i++){
      let r=d[i].data.readUInt32LE(0);
      console.log("Payload Rcv [" + r +"], reply result:" + radio.write(d[i].data));
    }
  },function() { console.log("STOP!"); });

}

function snd(do_sync) {

  radio.useWritePipe(Pipes[1]);
  radio.addReadPipe(Pipes[0]);
  console.log("Open Write Pipe "+ Pipes[1]+ " reading from " + Pipes[0]);

  var tmstp;
  var roundtrip;
  var sender=function(){
    var data=Buffer.alloc(4);
    tmstp=Math.round(+new Date()/1000);
    data.writeUInt32LE(tmstp);
    process.stdout.write("\nSending....");
    roundtrip=process.hrtime();
    if(do_sync) {
       if(radio.write(data)) process.stdout.write("[Sync] Sended " + tmstp);
       else process.stdout.write("[Sync] Failed to send " + tmstp +" ");
    } else {
      radio.write(data,function(success){
        if(success) process.stdout.write("[Async] Sended " + tmstp);
        else process.stdout.write("[Async] Failed to send " + tmstp +" ");
      });
    }
  };
  radio.read(function(d,n) {
    if(n==1){
      let t=process.hrtime(roundtrip);
      process.stdout.write(" | response received |")
      let ret=d[0].data.readUInt32LE(0);
      if( ret == tmstp) process.stdout.write(" reponse matchs ");
      else process.stdout.write(" response does not match "+ ret + " != " + tmstp);
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
  if(mode.toUpperCase()=="S") {
    rl.question("Send [s]ync or [A]sync?",(resp2)=>{
       var asy=resp2.charAt(0);
       console.log("Starting snd... press CTRL+C to stop");
       if(asy.toUpperCase()=="S") snd(true);
       else snd(false);
       rl.close(); 
    }); 
  }else {
    console.log("Starting rcv.... press CTRL+C to stop");
    rcv();
    rl.close();
  }
});
