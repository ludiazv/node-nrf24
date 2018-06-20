var nrf24=require("../build/Release/nRF24");
var readline = require('linebyline');

var rf24= new nrf24.nRF24(25, 1);
console.log("begin->",rf24.begin(true));
rf24.config({
  PALevel: nrf24.RF24_PA_MAX,
  DataRate: nrf24.RF24_1MBPS
},true);
rf24.useWritePipe("0x544e4c4331");

var rl = readline(process.stdin);

rl.on('line', function(line){
        success = rf24.write(Buffer.from(line));
        console.log("Sent " + ( success ? "OK" : "KO" ));
});
