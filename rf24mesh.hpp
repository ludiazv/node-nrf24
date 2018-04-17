#ifndef _NRF24_NODE_MESH_HPP_
#define _NRF24_NODE_MESH_HPP_

#include "rf24.hpp"
#include <RF24Network.h>
#include <RF24Mesh.h>



class nRF24Mesh : public Nan::ObjectWrap {
 public:

   struct MeshRule {
     uint8_t type;
     uint16_t max_len;
   };

   struct MeshFrame {
     RF24NetworkHeader header;
     uint8_t data[MAX_PAYLOAD_SIZE+1];
     uint16_t size;
   };

   // Worker class
   class MeshWorker : public RF24MeshAsyncWorker {
    public:
      MeshWorker(
          Nan::Callback *_progress
        , Nan::Callback *callback
        , nRF24Mesh& _mesh)
        : RF24MeshAsyncWorker(callback), progress(_progress),mesh(_mesh)
        , want_stop(false),stopped_(true), error_count(0)
        , poll_timeus(RF24_DEFAULT_POLLTIME) {
           mesh.worker_=this;
         }
      ~MeshWorker() { mesh.worker_=NULL; if(progress) delete progress; }

      // Main loop for pooling the reading
      void Execute(const RF24MeshAsyncWorker::ExecutionProgress& progress_);
      void HandleProgressCallback(const char *data, size_t size);
      void HandleOKCallback();

      void stop();
      bool inline stopped() { return stopped_; }

    private:
     Nan::Callback *progress;
     nRF24Mesh &mesh;
     bool  want_stop,stopped_;
     int   error_count;
     useconds_t poll_timeus;

    };

    // MODULE INIT
   static NAN_MODULE_INIT(Init) {
       // Create a the function
       v8::Local<v8::FunctionTemplate> tpl = Nan::New<v8::FunctionTemplate>(New);
       tpl->SetClassName(Nan::New("nRF24Mesh").ToLocalChecked());
       tpl->InstanceTemplate()->SetInternalFieldCount(1);

       //Method registration
       Nan::SetPrototypeMethod(tpl, "begin", begin_mesh);
       Nan::SetPrototypeMethod(tpl, "stop", stop_mesh);
       Nan::SetPrototypeMethod(tpl, "filter", accept_mesh);
       Nan::SetPrototypeMethod(tpl, "onRcv",onrcv_mesh);
       Nan::SetPrototypeMethod(tpl, "send",send_mesh);
       Nan::SetPrototypeMethod(tpl, "getNodeID",getNodeID);
       Nan::SetPrototypeMethod(tpl, "getAddrList",getAddrList);

       // Set up class
       constructor().Reset(Nan::GetFunction(tpl).ToLocalChecked());
       Nan::Set(target, Nan::New("nRF24Mesh").ToLocalChecked(),
       Nan::GetFunction(tpl).ToLocalChecked());
       // Configure constants
      NANCONSTI("MESH_RENEWAL_TIMEOUT",MESH_RENEWAL_TIMEOUT);
      NANCONSTI("MESH_DEFAULT_CHANNEL",MESH_DEFAULT_CHANNEL);
      NANCONSTI("MAX_PAYLOAD_SIZE",MAX_PAYLOAD_SIZE);
      NANCONST("MESH_ANY","0x00");

   }

   // Class inteface
   bool _begin(uint8_t nodeID,uint8_t channel, uint8_t data_rate,uint8_t pa,uint32_t timeout);
   int32_t _addrcv(uint8_t type,uint16_t max_len);
   bool _send(void* data, uint8_t msg_type, uint16_t size, uint8_t nodeID=0);
   int _loop(nRF24Mesh::MeshFrame& frame);
   void _stop();
   int16_t _getNodeID(uint16_t address=MESH_BLANK_ID);
   inline bool _isMaster() { return this->nodeID_ == 0;}

 private:
   explicit nRF24Mesh(nRF24& _nrf24,bool clean_dhcp) : nrf24(_nrf24), radio(NULL), reset_dhcp(clean_dhcp), nodeID_(0),
                                                      network(NULL), mesh(NULL) , worker_(NULL) {
     rules.clear();
   }
   ~nRF24Mesh() { _stop(); if(network) delete network; if(mesh) delete mesh; }

   // New @param nRF24Mesh wraped object
   static NAN_METHOD(New) {
       if (info.IsConstructCall()) {
         v8::Local<v8::Object> _obj;
         bool dhcp=false;
         std::string error;

         if(Nan::Check(info).ArgumentsCount(2)
            .Argument(0).IsObject().Bind(_obj)
            .Argument(1).Bind(dhcp).Error(&error)) {
             nRF24 *trf24 = Nan::ObjectWrap::Unwrap<nRF24>(_obj); // TODO check that is real nRF24
             nRF24Mesh *mesh=new nRF24Mesh(*trf24,dhcp); //
             mesh->Wrap(info.This());
             info.GetReturnValue().Set(info.This());
         } else return Nan::ThrowTypeError("nRF24Mesh constructor ERROR:Wrong arguments");
       } else {
         return Nan::ThrowTypeError("nRF24Mesh constructor ERROR: called constructor without new keyword!");
       }
   }
   // Begin methods
   static NAN_METHOD(begin_mesh) {

      auto THIS=MTHIS(nRF24Mesh);
      int32_t nodeID=0;
      uint32_t timeout=MESH_RENEWAL_TIMEOUT;
      uint8_t  channel=MESH_DEFAULT_CHANNEL,data_rate=RF24_1MBPS;
      uint8_t  paLevel=RF24_PA_LOW;
      std::string error;
      // If radio has been configured usthe
      if(THIS->nrf24._has_config()) {
        auto cc=THIS->nrf24._get_config();
        channel=cc->Channel;
        data_rate=cc->DataRate;
        paLevel=cc->PALevel;
      }
      if(channel <=0 || channel > 127) channel=MESH_DEFAULT_CHANNEL;
      if(Nan::Check(info).ArgumentsCount(1)
        .Argument(0).Bind(nodeID).Error(&error)) {
        // Called only with nodeID
        if(nodeID<0 || nodeID>255) return Nan::ThrowTypeError("NodeID invalid 0-255");
      }
      else {
         if(Nan::Check(info).ArgumentsCount(2)
            .Argument(0).Bind(nodeID)
            .Argument(1).Bind(timeout).Error(&error)) {
            if(nodeID<0 || nodeID>255) return Nan::ThrowTypeError("NodeID invalid 0-255");
            timeout=timeout*1000; // s -> ms
      } else return Nan::ThrowTypeError("Mesh begin invalid parameters");
    }
      THIS->nodeID_=nodeID;
      MRET(THIS->_begin((uint8_t)nodeID,channel,data_rate,paLevel,timeout));
}
static NAN_METHOD(accept_mesh) {
  auto THIS=MTHIS(nRF24Mesh);
  v8::Local<v8::String> type;
  int max_len;
  std::string error;
  uint8_t typec[2]; typec[1]='\0';
  if(info.Length()==0) {
    MRET(THIS->_addrcv(0,MAX_PAYLOAD_SIZE));
  } else
  {
    if(Nan::Check(info).ArgumentsCount(2)
       .Argument(0).IsString().Bind(type)
       .Argument(1).Bind(max_len).Error(&error)) {
        if(!ConvertHexAddress(type,typec,1) /*|| typec[0]<0 */ || typec[0] >127) return Nan::ThrowSyntaxError("Type must be 0x00 to 0x7F");
        if(max_len > MAX_PAYLOAD_SIZE || max_len <=0) max_len=MAX_PAYLOAD_SIZE;
        NRF24DBG(std::cout << "Accepting frames for type " << typec[0] << " len:" << max_len << std::endl);
        MRET(THIS->_addrcv(typec[0],max_len));
       } else return Nan::ThrowSyntaxError(error.c_str());
  }

}

static NAN_METHOD(onrcv_mesh) {
  auto THIS=MTHIS(nRF24Mesh);
  v8::Local<v8::Function> _progress;
  v8::Local<v8::Function> _callback;
  std::string error;

  if (Nan::Check(info).ArgumentsCount(2)
          .Argument(0).IsFunction().Bind(_progress)
          .Argument(1).IsFunction().Bind(_callback).Error(&error))
  {

        Nan::Callback *progress = new Nan::Callback(_progress);
        Nan::Callback *callback = new Nan::Callback(_callback);
        Nan::AsyncQueueWorker(new nRF24Mesh::MeshWorker(progress,callback,*THIS));

  }
  else
  {
    return Nan::ThrowSyntaxError(error.c_str());
  }

}
static NAN_METHOD(send_mesh) {
  auto THIS=MTHIS(nRF24Mesh);
  v8::Local<v8::String> type;
  v8::Local<v8::Object> buffer;
  int to;
  std::string error;
  uint8_t typec[2]; typec[1]='\0';

  if(Nan::Check(info).ArgumentsCount(3)
     .Argument(0).IsString().Bind(type)
     .Argument(1).IsBuffer().Bind(buffer)
     .Argument(2).Bind(to)
     .Error(&error)) {

      if(to< 0 || to >255) return Nan::ThrowSyntaxError("Destination must be 0-255");
      if(!ConvertHexAddress(type,typec,1) || typec[0]<1 || typec[0] >127) return Nan::ThrowSyntaxError("Type must be 0x01 to 0x7F");
      size_t lg= node::Buffer::Length(buffer);
      if(lg > MAX_PAYLOAD_SIZE) return Nan::ThrowSyntaxError("Buffer too big");
      MRET(THIS->_send(node::Buffer::Data(buffer),typec[0],(uint16_t)lg,(uint8_t)to));

  } else return Nan::ThrowSyntaxError(error.c_str());

}

static NAN_METHOD(getNodeID) {
  auto THIS=MTHIS(nRF24Mesh);
  int addr;
  std::string error;
  if(Nan::Check(info).ArgumentsCount(1).Argument(0).Bind(addr).Error(&error)) {
    MRET(THIS->_getNodeID((uint16_t) addr));
  } else
  {
    MRET(THIS->_getNodeID());
  }
}
static NAN_METHOD(getAddrList) {
  auto THIS=MTHIS(nRF24Mesh);
  if(THIS->_isMaster()) {
    std::lock_guard<std::mutex> guard(THIS->mesh_mutex);
    std::lock_guard<std::mutex> guard2(THIS->rules_mutex);
    int lg=THIS->mesh->addrListTop;
    v8::Local<v8::Array> arr = Nan::New<v8::Array>(lg);
    for(int i=0;i<lg;i++) {
      v8::Local<v8::Object> o=Nan::New<v8::Object>();
      Nan::Set(o,Nan::New("nodeID").ToLocalChecked(),Nan::New(THIS->mesh->addrList[i].nodeID));
      Nan::Set(o,Nan::New("address").ToLocalChecked(),Nan::New(THIS->mesh->addrList[i].address));
      Nan::Set(arr,i,o);
    }
    MRET(arr);
  } else MRET(Nan::False()); // If no master then is not valid

}

static NAN_METHOD(stop_mesh) { MTHIS(nRF24Mesh)->_stop(); }


   static inline Nan::Persistent<v8::Function> & constructor() {
     static Nan::Persistent<v8::Function> my_constructor;
     return my_constructor;
   }


   nRF24 &nrf24;
   RF24  *radio;
   bool  reset_dhcp;
   uint8_t nodeID_;
   RF24Network *network;
   RF24Mesh *mesh;
   nRF24Mesh::MeshWorker *worker_;
   std::mutex mesh_mutex,rules_mutex;
   std::vector<MeshRule> rules;

   friend class nRF24Mesh::MeshWorker;

};

#endif
