#include "rf24mesh.hpp"
#include "tryabort.hpp"
#include<fstream>
#include<thread>


// Main loop for pooling the reading

void nRF24Mesh::MeshWorker::Execute(const RF24MeshAsyncWorker::ExecutionProgress& progress_) {
  stopped_=false;
  useconds_t half=poll_timeus/4;
  nRF24Mesh::MeshFrame frame;

  NRF24DBG(std::cout << "Starting the mesh worker" << std::endl);
  while(!want_stop && error_count < 2) {
    int res=mesh._loop(frame);
    switch(res) {
      case -1: // Internal error on the loop
          error_count++;
          break;
      case 0: // No frames available
          usleep(half); // aditional sleep time
          error_count=0;
          break;
      case 1:
          progress_.Send((char *)&frame,sizeof(frame));
          error_count=0;
          break;
      default:
          usleep(half);
    }
    usleep(half);
  }
  stopped_=true;
}

void nRF24Mesh::MeshWorker::HandleProgressCallback(const char *data, size_t size) {
   Nan::HandleScope scope;
   const nRF24Mesh::MeshFrame *f=(const nRF24Mesh::MeshFrame *)data;
   auto h= Nan::New<v8::Object>(); // return object
   Nan::Set(h,Nan::New("from_node").ToLocalChecked(),Nan::New(f->header.from_node));
   Nan::Set(h,Nan::New("to_node").ToLocalChecked(),Nan::New(f->header.to_node));
   Nan::Set(h,Nan::New("type").ToLocalChecked(),Nan::New((uint32_t)f->header.type));
   if(mesh._isMaster()) Nan::Set(h,Nan::New("from_nodeID").ToLocalChecked(),Nan::New(mesh._getNodeID(f->header.from_node)));
   v8::Local<v8::Value> argv[] = {
     h,
     //Nan::NewBuffer((char*)f->data,f->size).ToLocalChecked()
     Nan::CopyBuffer((char*)f->data,f->size).ToLocalChecked()
   };

   progress->Call(2, argv,this->async_resource);
}

void nRF24Mesh::MeshWorker::HandleOKCallback() {
  Nan::HandleScope scope;
  v8::Local<v8::Value> argv[] = {
    Nan::New(stopped_),
    Nan::New(want_stop),
    Nan::New(error_count) };
    callback->Call(3,argv,this->async_resource);
}

void nRF24Mesh::MeshWorker::stop() {
  if(!stopped_) {
     want_stop=true;
     usleep(poll_timeus*2);
  }
}

void nRF24Mesh::_stop(){
  if(worker_) {
    worker_->stop();
    worker_=NULL;
  }
}

bool nRF24Mesh::_begin(uint8_t nodeID,uint8_t channel, uint8_t data_rate,uint8_t pa,uint32_t timeout) {
  bool res=false;
  std::lock_guard<std::mutex> guard(mesh_mutex);
  if(reset_dhcp) std::remove("dhcplist.txt"); // Clean DHCP if required
  //on_abort(); // Do nothing on abort
  try_and_catch_abort([&]() -> void {
          int ce,cs;
          // Disable referenced radio object
          nrf24._begin(false);
          nrf24._config();
          nrf24._stop_read();
          nrf24._enable(false); // Mark radio as unsuable
          // Recreate radio
          nrf24._get_cecs(ce,cs);
          NRF24DBG(std::cout << "CE:" << ce << "CS:" << cs << std::endl);
          if(radio) delete radio;
          radio= new RF24(ce,cs);
          if(radio) {
            radio->begin();
            radio->setPALevel(pa);
            NRF24DBG(radio->printDetails());
            if(network) delete network;
            // Build the network
            network= new RF24Network(*radio);
          }
          //Build the mesh
          if(mesh) delete mesh;
          if(network) mesh= new RF24Mesh(*radio,*network);
          if(mesh) {
            mesh->setNodeID(nodeID);
            res=mesh->begin(channel,(rf24_datarate_e)data_rate,timeout);
          }
  });
  return res;
}

int16_t nRF24Mesh::_getNodeID(uint16_t address) {
  int16_t res=-1;
  std::lock_guard<std::mutex> guard(rules_mutex);
  std::lock_guard<std::mutex> guard2(mesh_mutex);
  try_and_catch_abort([&]() -> void {
          res=mesh->getNodeID(address);
  });
  return res;
}

int32_t nRF24Mesh::_addrcv(uint8_t type,uint16_t max_len) {
  int32_t res;
  std::lock_guard<std::mutex> guard(rules_mutex);
  try {
    nRF24Mesh::MeshRule rule={type,max_len};
    rules.push_back(rule);
    res=rules.size()-1;
  } catch(...) {
      res=-1;
  }
  return res;
}

bool nRF24Mesh::_send(void* data, uint8_t msg_type, uint16_t size, uint8_t nodeID){
  bool res=false;
  std::lock_guard<std::mutex> guard(mesh_mutex);
  try_and_catch_abort([&]() -> void {
          res=mesh->write(data,msg_type,size,nodeID);
          if(!res && !_isMaster()) {
            if(!mesh->checkConnection()) mesh->renewAddress();
            res=mesh->write(data,msg_type,size,nodeID);
          }
  });
  return res;
}

int nRF24Mesh::_loop(nRF24Mesh::MeshFrame &frame) {
  int res=-1;
  std::lock_guard<std::mutex> guard(mesh_mutex);
  std::lock_guard<std::mutex> guard2(rules_mutex);
  try_and_catch_abort([&]()-> void {
                          mesh->update();
                          if(_isMaster()) mesh->DHCP();
                          if(network->available()) {
                            RF24NetworkHeader header;  // Check the header and try to match rules
                            network->peek(header);
                            NRF24DBG(std::cout << "[Loop] type:" << header.type << std::endl);
                            auto i=rules.begin();
                            for( ;i != rules.end(); ++i) {
                                if(i->type == 0 || i->type == header.type) { // type match
                                  auto lng=network->read(frame.header,frame.data,i->max_len);
                                  frame.size=lng;
                                  res=1;
                                  break;
                                }
                            }
                          if(i==rules.end()) {network->read(header,0,0); res=0; } // Ignore frame
                        } else res=0;
  });
  return res;
}
