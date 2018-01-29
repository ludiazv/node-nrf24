'use strict';
var rf=require("../build/Release/nRF24");
console.log("Setting up the radio...");
var rfo=new rf.nRF24(6,10);
rfo.begin();
rfo.config({PALevel:rf.RF24_PA_MAX,DataRate:rf.RF24_1MBPS,Channel:rf.MESH_DEFAULT_CHANNEL});
var mesh=new rf.nRF24Mesh(rfo,true);

var addr=0;
if(process.argv.length >2) addr=parseInt(process.argv[2]);


console.log("Starting the mesh with addr " + addr + " " + (addr==0) ? "MASTER MODE" : " REGULAR NODE");
console.log("Begin ->" + mesh.begin(addr));

if(addr==0) { // Master mode
  console.log("Seting up route table display...");
  setInterval(function(){
      var addrs=mesh.getAddrList();
      console.log("\n************** Address list ********");
      for(var i=0;i<addrs.length;i++){
        console.log(JSON.stringify(addrs[i]));
      }
      console.log("************************************");
      var ping_to=Math.floor(Math.random() * addrs.length);
      var data=Buffer.alloc(4);
      var tmstp=Math.round(+new Date()/1000);
      data.writeUInt32LE(tmstp);
      console.log("Ping [" +addrs[ping_to].nodeID + "] -> " + mesh.send("0x50",data,addrs[ping_to].nodeID)); // send "P" frame
  },10000);

  console.log("Accepting incoming frames type M(0x4d) with max length of 4 bytes (int32)");
  mesh.filter("0x4D",4);
  console.log("Accepting incoming frames type S(0x53) with max length of 64 (string)");
  mesh.filter("0x53",64);
  mesh.onRcv(function(header,data) {
      console.log("header:"+ JSON.stringify(header) + " data length:" + data.length);
      switch(header.type) {
        case 0x4d:
            console.log("Time received:" + data.readUInt32LE(0));
          break;
        case 0x53:
           console.log("String received:'" + data.toString('ascii') + "'");
           break;
      }

  },
  function() {
    console.log("stopped");
  }
 );
}
else {

}


console.log("Ready!");
