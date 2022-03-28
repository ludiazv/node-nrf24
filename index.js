var noderf24=undefined;

if(process.platform != "linux") {
  console.error("Error loading nRF24. This is LINUX only node addon");
} else {
  //noderf24= require("./build/Release/nRF24");
  //noderf24=require('bindings')('nRF24.node');
  noderf24 = require('node-gyp-build')(__dirname);
}

module.exports= noderf24;
