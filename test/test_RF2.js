/* Simple test of Boros RF2 expansion board */
/* This board has dual radio that are used in the same js script
 * CE,CS pins are preconfigured for this board and test
 */

'use strict';

const rf=require("../build/Release/nRF24");

var radio0=new rf.nRF24(24,0);
var radio1=new rf.nRF24(25,1);
console.log("Radio 0:",radio0," Radio 1:",radio1);
console.log("RADIO 0\n");
radio0.begin(true);
console.log("\n RADIO 1\n");
radio1.begin(true);
const IRQ=[27,22];

var config={PALevel:rf.RF24_PA_LOW,
             DataRate:rf.RF24_1MBPS,
             Channel:77};

var c0=Object.assign({Irq: IRQ[0]},config);
var c1=Object.assign({Irq: IRQ[1]},config);
console.log("CONFIG R0");
radio0.config(c0,true);
console.log("CONFIG R1");
radio1.config(c1,true);

// Open Pipes
var Pipes=["0x65646f4e41","0x65646f4e42","0x65646f4e43","0x65646f4e44"];
//var rPipes=["0x72646f4e31","0x72646f4e32"];

// Two-way binding
radio0.useWritePipe(Pipes[1]);
radio1.useWritePipe(Pipes[0]);
radio0.addReadPipe(Pipes[0],true);
radio1.addReadPipe(Pipes[1],true);
radio0.addReadPipe(Pipes[2],false);
radio1.addReadPipe(Pipes[3],false);



var it0=0,rx=[0,0];
var it1=0

function readFactory(n) {
    return function(data,items) {
      //var d=data.toString();
      console.log("R" + n + " readed ->" + JSON.stringify(data) , "items:",items);
      rx[n]++;
    }
}

radio0.read(readFactory(0),function(){});
radio1.read(readFactory(1),function(){});



setInterval(function(){
  var b=Buffer.from("" + it0);
  if((it0 % 2) == 0) radio0.useWritePipe(Pipes[1],true); else radio0.useWritePipe(Pipes[3],false);
  console.log("R0 Send " + it0 + "[" + radio0.write(b) + "]");
  it0++;
},1000);

setInterval(function(){
  var b=Buffer.from("" +it1);
  if((it1 % 2) == 0) radio1.useWritePipe(Pipes[0],true); else radio1.useWritePipe(Pipes[2],false);
  console.log("R1 Send " + it1 + "[" + radio1.write(b) + "]");
  it1++;
},2000); 

setInterval(function(){
  console.log("R0->R1 " + (rx[1]/it0)*100 + "% / R1->R0 " + (rx[0]/ it1)*100 +"%");
},20000);

console.log("Test Started... CTRL+C to close");
