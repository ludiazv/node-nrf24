var noderf24=null;

if(process.platform != "linux") {
  console.log("Error loading node-rf24. This is LINUX only node addon");
} else {
  noderf24= require("./build/Release/nRF24");
}

module.exports= noderf24;
