/* Simple test of Boros RF2 expansion board */
/* This board has dual radio that are used in the same js script
 * CE,CS pins are preconfigured for this board and test.
 *
 */

'use strict';

const rf=require("../build/Release/nRF24");
//const rf=require("../build/Debug/nRF24");
const os=require('os');
var events = require('events');
var Emitter = new events.EventEmitter();

const NUMBER_CPUS=os.cpus().length;

var radio0,radio1;
var Pipes=["0x65646f4e31","0x65646f4e32"];
const DEFAULT_CHANNEL=78;
const DEFAULT_PA=rf.RF24_PA_LOW;
const SHOW_CONFIG=false;
const IRQ=[27,22];
//const IRQ=[-1,-1];

function readFactory(n,p) {
    return function(data,items) {
      const b=data.reduce((a,v) => {return a+ v.data.length},0);
      process.stdout.write('.');
      if(p) console.log("R",n, " readed ->", b , " N items:",items);
    }
}

function sleep(ms){
    return new Promise(resolve=>{
        setTimeout(resolve,ms)
    })
}

function initRadios(details) {

  radio0=new rf.nRF24(24,0);
  radio1=new rf.nRF24(25,1);
  console.log("Radios Init...");
  console.log("Radio 0:",radio0," Radio 1:",radio1);
  console.log("RADIO 0");
  radio0.begin(details);
  console.log("RADIO 1");
  radio1.begin(details);

}

function destroyRadios() {
    radio0.destroy();
    radio1.destroy();
    radio0=radio1= null; // UnRef
    if(global.gc) global.gc(); // Force gc if expossed
}


function configRadios(conf) {
  var c0=Object.assign({Irq: IRQ[0]},conf);
  var c1=Object.assign({Irq: IRQ[1]},conf);
  console.log("CONFIG R0!"); radio0.config(c0,SHOW_CONFIG);
  console.log("CONFIG R1!"); radio1.config(c1,SHOW_CONFIG);
}



async function OneWay(seconds,ack,speed,w_async) {
    console.log("Testing OneWay for ",seconds,"s, Ack:",ack,", Speed:",speed);
    console.log("===============================================");
    var t;
    initRadios(false);
    configRadios({
              PALevel: DEFAULT_PA,
              DataRate: speed,
              Channel: DEFAULT_CHANNEL,
              AutoAck: ack
    });

    // R0 send to R1
    radio0.useWritePipe(Pipes[1]);
    radio1.addReadPipe(Pipes[1]);


    radio1.read(readFactory(1),function(){
      clearInterval(t);
      console.log("\nRead and test Stoped");
      console.log("====================");
      console.log("0 Sended:", radio0.getStats(0) , "<->", radio1.getStats(1), "1 Rcv");
      reportCpu(seconds);
      destroyRadios();
      test_next();
    });

    await sleep(5000);
    setTimeout(()=> {
      radio0.stopWrite();
      radio1.stopRead()},seconds*1000);

    t=setInterval(()=> {
      if(w_async){
        radio0.write(Buffer.from("msg12456async"),function(success) {
            if(success) process.stdout.write('w'); else process.stdout.write('x');
        });
      }
      else {
        if(radio0.write(Buffer.from("msg12456"))) process.stdout.write('W');
        else process.stdout.write('X');
      }
    },100);

}

async function Transfer(seconds,ack,speed) {
  console.log("Testing Transfer for ",seconds,"s, Ack:",ack,", Speed:",speed);
  console.log("===============================================");
  var t;
  const buffer_size=1024*8;
  var buffer=Buffer.alloc(buffer_size);
  buffer.fill('0');

  initRadios(false);
  configRadios({
            PALevel: DEFAULT_PA,
            DataRate: speed,
            Channel: DEFAULT_CHANNEL,
            AutoAck: ack,
            PollBaseTime:4000
  });


  // R0 send to R1
  radio0.useWritePipe(Pipes[1]);
  radio0.changeWritePipe(ack,buffer_size);
  var p=radio1.addReadPipe(Pipes[1]);
  radio1.changeReadPipe(p,ack,32);

  var total_readed=0;
  var total_read_cb=0;
  radio1.read(function(data,n){
    //process.stdout.write(data[0].data.length+" ");
    for(var i=0;i<n;i++){
      total_readed+=data[i].data.length; //data[0].data.length;
    }
    total_read_cb++;
  },function(){
    clearTimeout(t);
    console.log("\nRead and test Stoped");
    console.log("====================");
    console.log("0 Sended:", radio0.getStats(0) , "<->", radio1.getStats(1), "1 Rcv");
    console.log("Total Readed:",total_readed," b per cb:",total_readed/total_read_cb);
    reportCpu(seconds);
    destroyRadios();
    test_next();
  });

  await sleep(5000);
  setTimeout(()=> {
    radio0.stopWrite();
    radio1.stopRead()},(seconds+1)*1000);

  var txid=0;
  function stream() {
    if(global.gc) global.gc();
    var t0=process.hrtime();
    txid++;
    var tx=txid;
    if(radio0!=null) {
    radio0.stream(buffer,function(success,tx_ok,tx_bytes,req,pck,abt){
        var t1=process.hrtime(t0);
        console.log("{",tx,"}success:",success," abt:",abt," tx_ok:",tx_ok,"[",tx_bytes," b] req:",req," time:",t1);
        t=setTimeout(stream,0);
    });
    }
  }
  stream();
  var last_total=0;
  setInterval(()=>{
     var dr=total_readed-last_total;
     last_total=total_readed;
     console.log("Rcv Speed:",dr/1024,"Kb/s");
  },1000);

}
// REgister tests
Emitter.on('OneWay',OneWay);
Emitter.on('Transfer',Transfer);

console.log("Test Started... CTRL+C to close N_CPUS:",NUMBER_CPUS);

var cpu;
var test_nr=-1;
var TestSuite= [
  ['OneWay',5,false,rf.RF24_2MBPS,true],
  ['OneWay',5,false,rf.RF24_1MBPS,false],
  ['OneWay',5,false,rf.RF24_250KBPS,true],
  ['OneWay',5,true,rf.RF24_2MBPS,false],
  ['OneWay',5,true,rf.RF24_1MBPS,true],
  ['OneWay',5,true,rf.RF24_250KBPS,false]
  //['Transfer',180,true,rf.RF24_1MBPS]
];

function test_next() {
  cpu=process.cpuUsage();
  test_nr++;
  if(test_nr == TestSuite.length)
    console.log("TEST FINISHED!");
  else Emitter.emit(...TestSuite[test_nr]);
}
function reportCpu(s){
  const totalus=s*1000000*NUMBER_CPUS;
  var ncpu=process.cpuUsage();
  var total=ncpu.system + ncpu.user - cpu.system - cpu.user;
  console.log("% CPU:",(100.0*(total/totalus)).toFixed(1),"[% User:",100*((ncpu.user-cpu.user)/totalus),
              " , % System:",100*((ncpu.system-cpu.system)/totalus),"]");
}

test_next();
