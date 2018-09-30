/* Sample script to test speed and steaming
 * capabilities of the nodejs module
 * Is interoperable with examples in cpp library
 * Reference speeds in CPP lib are:
 *   2MBPS:    68Kb/s  with Ack , 200Kb/s w/o Ack
 *   1MBPS:    47Kb/s  with Ack , 100Kb/s w/o Ack
 *   250Kbps:  16Kb/s  with Ack , 23Kb/s  w/0 Ack
 */

 'use strict';
 const nrf24 = require("../build/Release/nRF24"); // Load the module
 const fs = require('fs');
 const os=require('os');
 const readline = require('readline');
 const NUMBER_CPUS=os.cpus().length;

 // Init the radio
 //const rf24 = new nrf24.nRF24(22, 0);
 // Boros RF2 radio 2
 // Change the constructor to fit the wiring
 //const rf24 = new nrf24.nRF24(24,0); // R0
 const rf24 = new nrf24.nRF24(25,1); // R1
 //const IRQ=-1;
 //const IRQ=27;   // Radio 0
 const IRQ=22; // Radio 1

 rf24.begin(false);
 var ready=rf24.present();
 console.log('Radio connected:' + ready);
 console.log('Is + Variant:' + rf24.isP()); // Prints true/false if radio is + Variant

 if(!ready) {
   console.log("ERROR:Radio is not ready!");
   process.exit(1);
 }

var Pipes=["0xABCDABCD71","0x544d52687C"];

// Change the configuration to
// test difrent speed o


const STREAM_SIZE=1024;
var config={
  PALevel: nrf24.RF24_PA_HIGH,
  //PALevel: nrf24.RF24_PA_MAX,
  DataRate: nrf24.RF24_1MBPS,
  //DataRate: nrf24.RF24_2MBPS,
  //DataRate: nrf24.RF24_250KBPS,
  //CRCLength: nrf24.RF24_CRC_DISABLED,
  CRCLength: nrf24.RF24_CRC_8,
  AutoAck: true,
  //AutoAck: false,
  Channel:70,
  retriesCount: 2,
  retriesDelay: 15,
  Irq:IRQ
};

rf24.config(config,true);

function rcv() {

  const pipe=rf24.addReadPipe(Pipes[1]);
  rf24.changeReadPipe(pipe,config.AutoAck,nrf24.RF24_MAX_MERGE); // Max merge pckts
  console.log("Stating Reading on pipe addr:", Pipes[1], " #:", pipe);

  var r_total=[];
  rf24.read(function(data,items){
    for(var i=0;i<items;i++){
      r_total.push(data[i].data.length); // add rcv bytes
    }
  },function(){
    console.log("Reading stoped");
  });

  const totalus=1000000*NUMBER_CPUS; // Total us CPU in 1s
  var cpu=process.cpuUsage();
  var avg_speed=0;
  setInterval(function(){
    var ncpu=process.cpuUsage();
    var b=r_total.reduce((a,v)=>{ return a+v;},0);  // Accumulate rcv bytes;
    r_total=[]; // Clean
    var consumed_s= ncpu.system - cpu.system;
    var consumed_u= ncpu.user - cpu.user;
    var consumed= consumed_s + consumed_u;
    avg_speed= (avg_speed + (b/1024.0))/2;
    console.log('INCOMING Bytes: ', b, 'data rate:', (b/1024.0).toFixed(2), 'kb/s',
                ' Avg Speed:', avg_speed.toFixed(2), " kb/s ", 
                ' CPU Usage %:', ((consumed/totalus)*100.0).toFixed(2),
                '[ System %:',  ((consumed_s/totalus)*100.0).toFixed(2),
                ' User %:', ((consumed_u/totalus)*100.0).toFixed(2),"]");
     cpu=ncpu;
  },1000);

}

function snd() {
    rf24.useWritePipe(Pipes[1]);
    rf24.changeWritePipe(config.AutoAck,STREAM_SIZE);

    var buffer=Buffer.alloc(STREAM_SIZE);
    buffer.fill('#');
    console.log("Start streaming with ", STREAM_SIZE/1024 , " K buffer size");

    var w_total=[];
    var e_total=[];
    var fnTx=function() {
      rf24.stream(buffer,function(success,tx_ok,tx_bytes,req,pck,abt){
          //if(!success) console.log("[F]");
          w_total.push(tx_bytes);
          e_total.push((req-tx_ok)*pck);
      });
      setTimeout(fnTx,0); // Continue streaming
    };
    setTimeout(fnTx,1000); // Start test in 1 second

    const totalus=1000000*NUMBER_CPUS; // Total us CPU in 1s
    var cpu=process.cpuUsage();
    var avg_speed=0;
    setInterval(function(){
      var ncpu=process.cpuUsage();
      var b=w_total.reduce((a,v)=>{ return a+v;},0);  // Accumulate tx bytes;
      var e=e_total.reduce((a,v)=>{ return a+v;},0);  // Acc error
      w_total=[]; // Clean
      e_total=[];
      var consumed_s= ncpu.system - cpu.system;
      var consumed_u= ncpu.user - cpu.user;
      var consumed= consumed_s + consumed_u;
      avg_speed= (avg_speed + (b/1024.0))/2;
      console.log('OUTGOING Bytes: ', b, 'data rate:', (b/1024.0).toFixed(2), 'kb/s ',
                  'TxErrors:', (e/1024.0).toFixed(2), ' kb',
                  ' Avg Speed:', avg_speed.toFixed(2), " kb/s ", 
                  ' CPU Usage %:', ((consumed/totalus)*100.0).toFixed(2),
                  '[ System %:',  ((consumed_s/totalus)*100.0).toFixed(2),
                  ' User %:', ((consumed_u/totalus)*100.0).toFixed(2),"]");
       cpu=ncpu;
    },1000);

}

const rl = readline.createInterface({
  input: process.stdin,
  output: process.stdout
});

console.log("RF24 Stream speed test");
rl.question("[s]ender or [R]eceiver?",(resp)=>{
  var mode=resp.charAt(0);
  console.log("Running... press CTRL+C to stop test");
  if(mode.toUpperCase()=="S") snd(); else rcv();
  rl.close();
});
