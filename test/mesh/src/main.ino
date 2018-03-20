#include <Arduino.h>
#include<SPI.h>
#include<RF24.h>
#include<RF24Network.h>
#include<RF24Mesh.h>

#if defined(ARDUINO_ARCH_ESP8266) || defined(ESP8266)
  RF24 radio=RF24(2,15);
  #define MODE_PIN  0
  #define BOARD "ESP88266"
#else // Asume is AVR
  RF24 radio=RF24(7,8);
  #define BOARD "AVR"
  #define MODE_PIN  3
#endif

#define MAX_STRING 64

RF24Network network(radio);
RF24Mesh mesh(radio,network);

bool master= false;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.println(F("RF24 Mesh Testing"));
  Serial.print(F("MODEL:")); Serial.println(BOARD);
  Serial.println(F("This firmaware create a mesh network with master and nodes"));
  Serial.println(F("Reading D3 (IO0 on ESP8266) to define master or random node"));
  pinMode(MODE_PIN,INPUT);
  master=(digitalRead(MODE_PIN)) ? false : false;

  radio.begin();
  radio.setPALevel(RF24_PA_MAX);
  Serial.print(F("Radio Present:")); Serial.println(radio.isChipConnected());
  Serial.print(F("is + variant:")); Serial.println(radio.isPVariant());
  uint8_t id=0;
  if(master) Serial.println(F("MASTER MODE SELECTED"));
  else{
    randomSeed(analogRead(0));
    id=random(2,250);
  }
  mesh.setNodeID(id);
  Serial.print(F("Node addr:")); Serial.println(mesh.getNodeID());
  mesh.begin(76);
  Serial.println(F("RUN!"));

}

uint32_t displayTimer = 0;
uint32_t checkTimer = 0;
uint32_t sendTimer =0;
uint32_t sendTimer2 =0;

void loop() {
    mesh.update();
    if(master){
      mesh.DHCP();
      // Check for incoming data from the sensors
      if(network.available()){
        RF24NetworkHeader header;
        network.peek(header);

        uint32_t dat=0;
        char buff[MAX_STRING+1];
        int16_t l;
        switch(header.type){
          // Display the incoming millis() values from the sensor nodes
          case 'M': network.read(header,&dat,sizeof(dat));
                    Serial.print(mesh.getNodeID(header.from_node));
                    Serial.print("/M->");Serial.println(dat); break;
          case 'S':
                    memset(buff, 0, MAX_STRING);
                    l=network.read(header,buff,MAX_STRING);
                    Serial.print(mesh.getNodeID(header.from_node));
                    Serial.print("S->");Serial.print(l);Serial.print("/");Serial.println(buff); break;

          default: network.read(header,0,0); Serial.println(header.type);break;
        }
      }

      // Display the currently assigned addresses and nodeIDs
      if (millis() - displayTimer > 10000) {
        displayTimer = millis();
        Serial.println(" ");
        Serial.println(F("********Assigned Addresses********"));
        for (int i = 0; i < mesh.addrListTop; i++) {
          Serial.print("NodeID: ");
          Serial.print(mesh.addrList[i].nodeID);
          Serial.print(" RF24Network Address: 0");
          Serial.println(mesh.addrList[i].address, OCT);
        }
        Serial.println(F("**********************************"));
        if(mesh.addrListTop>0) {
          uint8_t p_node=random(0,mesh.addrListTop);
          Serial.print("P"); Serial.print(mesh.addrList[p_node].nodeID); Serial.print("->");
          Serial.println(mesh.write(&displayTimer, 'P', sizeof(displayTimer), mesh.addrList[p_node].nodeID));
        }
      }
    }
    else {
      // node
      if(network.available()) {
        RF24NetworkHeader header;
        network.peek(header);
        if(header.type=='P') {
           uint32_t dat=0;
           network.read(header,&dat,sizeof(dat));
           Serial.println(" ");
           Serial.print("Ping:"); Serial.print(dat);
        }else network.read(header,0,0);
      }
      // If a write fails, check connectivity to the mesh network
      if ( millis() - checkTimer > 10000) {
         Serial.print(".");
         check_renew();
         checkTimer=millis();
      }
      if(millis() - sendTimer > 5000){
        sendTimer=millis();
        Serial.print("D");
        if (!mesh.write(&sendTimer, 'M', sizeof(sendTimer))) check_renew();
      }
      if(millis() - sendTimer2 > 15000) {
        sendTimer2=millis();
        char buff[MAX_STRING+1];
        int i=0,l=random(1,MAX_STRING);
        for(i=0;i<l;i++) buff[i]=random(65,90);
        buff[i]='\0';
        Serial.print("S");
        if(!mesh.write(buff,'S',l)) check_renew();
      }
    }
}

void check_renew() {
  Serial.print("C");
  if(!mesh.checkConnection() ) {
    //refresh the network address
    Serial.println("Renewing Address");
    mesh.renewAddress();
  }
}
