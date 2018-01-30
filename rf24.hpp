#ifndef _NRF24_NODE_HPP_
#define _NRF24_NODE_HPP_

#include <unistd.h>
#include <mutex>
#include "rf24_config.hpp"
#include "rf24_util.hpp"

// Config structures
typedef struct RF24_conf {
  uint8_t PALevel;
  uint8_t DataRate;
  uint8_t Channel;
  uint8_t CRCLength;
  uint8_t retriesDelay;
  uint8_t retriesCount;
  uint8_t PayloadSize;
  uint8_t AddressWidth;
} RF24_conf_t;

std::ostream& operator<<(std::ostream& out, RF24_conf_t &h); // Prototype

//typedef Nan::AsyncProgressQueueWorker<char> LFG;

class nRF24 : public Nan::ObjectWrap {
 public:
   // Reader nested class
   class ReaderWorker : public Nan::AsyncProgressWorker {
    public:
     ReaderWorker(
         Nan::Callback *progress
       , Nan::Callback *callback
       , nRF24& dev)
       : Nan::AsyncProgressWorker(callback), progress(progress),device(dev)
       , want_stop(false),stopped_(true), want_towrite(false), error_count(0)
       , poll_timeus(RF24_DEFAULT_POLLTIME) {
          dev.worker_=this;
        }
     ~ReaderWorker() { device.worker_=NULL; if(progress) delete progress; }

     // Main loop for pooling the reading
     void Execute(const Nan::AsyncProgressWorker::ExecutionProgress& progress);
     void HandleProgressCallback(const char *data, size_t size);
     void HandleOKCallback();

     void inline want_write(bool b=true) {want_towrite=b;}
     void stop();
     bool inline stopped() { return stopped_; }

   private:
    Nan::Callback *progress;
    nRF24 &device;
    volatile bool  want_stop,stopped_,want_towrite;
    int   error_count;
    useconds_t poll_timeus;

   };
   // MODULE INIT
  static NAN_MODULE_INIT(Init) {
      // Create a the function
      v8::Local<v8::FunctionTemplate> tpl = Nan::New<v8::FunctionTemplate>(New);
      tpl->SetClassName(Nan::New("nRF24").ToLocalChecked());
      tpl->InstanceTemplate()->SetInternalFieldCount(1);

      //Method registration
      Nan::SetPrototypeMethod(tpl, "begin", begin);
      Nan::SetPrototypeMethod(tpl, "config", config);
      Nan::SetPrototypeMethod(tpl, "read",read);
      Nan::SetPrototypeMethod(tpl, "stop_read",stop_read);
      Nan::SetPrototypeMethod(tpl, "write",write);
      Nan::SetPrototypeMethod(tpl, "useWritePipe",useWritePipe);
      Nan::SetPrototypeMethod(tpl, "addReadPipe",addReadPipe);
      Nan::SetPrototypeMethod(tpl, "removeReadPipe",removeReadPipe);
      Nan::SetPrototypeMethod(tpl, "present",present);
      Nan::SetPrototypeMethod(tpl, "isP",isP);
      Nan::SetPrototypeMethod(tpl, "powerUp",powerUp);
      Nan::SetPrototypeMethod(tpl, "powerDown",powerDown);

      // Set up class
      constructor().Reset(Nan::GetFunction(tpl).ToLocalChecked());
      Nan::Set(target, Nan::New("nRF24").ToLocalChecked(),
      Nan::GetFunction(tpl).ToLocalChecked());
      // Configure constants
      NANCONSTI("RF24_1MBPS",RF24_1MBPS);
      NANCONSTI("RF24_2MBPS ", RF24_2MBPS);
      NANCONSTI("RF24_250KBPS",RF24_250KBPS);
      NANCONSTI("RF24_PA_MIN ",RF24_PA_MIN);
      NANCONSTI("RF24_PA_LOW",RF24_PA_LOW);
      NANCONSTI("RF24_PA_HIGH",RF24_PA_HIGH);
      NANCONSTI("RF24_PA_MAX",RF24_PA_MAX);
      NANCONSTI("RF24_CRC_DISABLED",RF24_CRC_DISABLED);
      NANCONSTI("RF24_CRC_8",RF24_CRC_8);
      NANCONSTI("RF24_CRC_16",RF24_CRC_16);
  }

  bool _begin(bool print_details=false); // Start the radio
  void _config(); // Configure the readio
  RF24_conf_t *_get_config();      // Init or get current config
  inline bool _has_config() {return current_config!=NULL ; }
  inline RF24* _get_radio() {return radio_; }
  inline void  _get_cecs(int& ce,int&cs) { ce=ce_; cs=cs_; }
  inline void _enable(bool e=true) { is_enabled_ = e; } // enable disable.
  bool _available(uint8_t *pipe=NULL); // Check if data is avaialble
  bool _read(void *data,size_t r_length); // read data.
  void _stop_read();  // stop read
  bool _write(void *data,size_t r_length); // write data
  bool _useWritePipe(uint8_t *pipe_name);
  int32_t _addReadPipe(uint8_t *pipe_name,bool auto_ack=true);
  void    _removeReadPipe(int32_t number);
  bool _powerUp();
  bool _powerDown();
  bool _listen();
  bool _transmit();
  bool _present();
  bool _isP();



 private:
  explicit nRF24(int ce,int cs) : ce_(ce), cs_(cs) , radio_(NULL), worker_(NULL),
    current_config(NULL),
    is_powered_up_(true),is_listening_(false), is_enabled_(true){
      used_pipes_[0]=used_pipes_[1]=used_pipes_[2]=false;
      used_pipes_[3]=used_pipes_[4]=used_pipes_[5]=false;
    }

  ~nRF24() { if(worker_) worker_->stop(); if(radio_) delete radio_; if(current_config) delete current_config;}

  static NAN_METHOD(New) {
      if (info.IsConstructCall()) {
        int32_t ce,cs;
        std::string error;
        if(Nan::Check(info).ArgumentsCount(2)
           .Argument(0).Bind(ce)
           .Argument(1).Bind(cs).Error(&error)) {
            nRF24 *obj = new nRF24(ce,cs);
            obj->Wrap(info.This());
            info.GetReturnValue().Set(info.This());
        } else return Nan::ThrowTypeError("nRF24 constructor ERROR:Wrong argument number!");
      } else {
        return Nan::ThrowTypeError("nRF24 constructor ERROR: called constructor without new keyword!");
        /*const int argc = 1;
        v8::Local<v8::Value> argv[argc] = {info[0]};
        v8::Local<v8::Function> cons = Nan::New(constructor());
        info.GetReturnValue().Set(cons->NewInstance(argc, argv)); */

      }
  }

  // inteface methods
  static NAN_METHOD(begin) {
    auto THIS=MTHIS(nRF24);
    bool p_d=false;
    if(info.Length()==1) p_d=info[0]->BooleanValue();
    MRET(THIS->_begin(p_d));
  }

  static NAN_METHOD(config) {
      auto *THIS=MTHIS(nRF24);
      v8::Local<v8::Object> _obj;
      std::string error;

      if(Nan::Check(info).ArgumentsCount(1)
              .Argument(0).IsObject().Bind(_obj).Error(&error))
      {
        RF24_conf_t *cc=THIS->_get_config(); // Init or retrive config
        //std::cout << "Previous Config-->" << *cc << std::endl;
        if(ObjHas(_obj,"PALevel")) cc->PALevel=(uint8_t)ObjGetUInt(_obj,"PALevel");
        if(ObjHas(_obj,"Channel")) cc->Channel=(uint8_t)ObjGetUInt(_obj,"Channel");
        if(ObjHas(_obj,"DataRate")) cc->DataRate=(uint8_t)ObjGetUInt(_obj,"DataRate");
        if(ObjHas(_obj,"PayloadSize")) cc->PayloadSize=(uint8_t)ObjGetUInt(_obj,"PayloadSize");
        if(ObjHas(_obj,"retriesDelay")) cc->retriesDelay=(uint8_t)ObjGetUInt(_obj,"retriesDelay");
        if(ObjHas(_obj,"retriesCount")) cc->retriesCount=(uint8_t)ObjGetUInt(_obj,"retriesCount");
        if(ObjHas(_obj,"AddressWidth")) cc->AddressWidth=(uint8_t)ObjGetUInt(_obj,"AddressWidth");
        if(ObjHas(_obj,"CRCLength")) cc->CRCLength=(uint8_t)ObjGetUInt(_obj,"CRCLength");
        //std::cout << "New Confing-->" << *cc << std::endl;
        THIS->_config(); // do Config

      } else Nan::ThrowSyntaxError(error.c_str());

  }

  static NAN_METHOD(present) { MRET(MTHIS(nRF24)->_present()); }
  static NAN_METHOD(isP) { MRET(MTHIS(nRF24)->_isP()); }
  static NAN_METHOD(powerDown) { MRET(MTHIS(nRF24)->_powerDown()); }
  static NAN_METHOD(powerUp) { MRET(MTHIS(nRF24)->_powerUp()); }

  static NAN_METHOD(read) {
      auto THIS=MTHIS(nRF24);
      v8::Local<v8::Function> _progress;
      v8::Local<v8::Function> _callback;
      std::string error;

      if (Nan::Check(info).ArgumentsCount(2)
              .Argument(0).IsFunction().Bind(_progress)
              .Argument(1).IsFunction().Bind(_callback).Error(&error))
      {
            if(THIS->is_enabled_ && THIS->worker_==NULL) {
              Nan::Callback *progress = new Nan::Callback(_progress);
              Nan::Callback *callback = new Nan::Callback(_callback);
              Nan::AsyncQueueWorker(new nRF24::ReaderWorker(progress,callback,*THIS));
            }
      }
      else
      {
        return Nan::ThrowSyntaxError(error.c_str());
      }
  }
  static NAN_METHOD(stop_read) { auto THIS=MTHIS(nRF24); THIS->_stop_read();}
  static NAN_METHOD(write) {
      auto THIS=MTHIS(nRF24);
      v8::Local<v8::Object> buffer;
      std::string error;

      if(Nan::Check(info).ArgumentsCount(1)
         .Argument(0).IsBuffer().Bind(buffer).Error(&error)) {
           RF24_conf_t *cc=THIS->_get_config();
           size_t size= node::Buffer::Length(buffer);
           size=(size>cc->PayloadSize) ? cc->PayloadSize : size;
           MRET(THIS->_write(node::Buffer::Data(buffer),size));
      } else return Nan::ThrowSyntaxError(error.c_str());
  }
  static NAN_METHOD(useWritePipe) {
    auto THIS=MTHIS(nRF24);
    v8::Local<v8::String> addr;
    std::string error;
    uint8_t addrc[6]; addrc[5]='\0';

    if(Nan::Check(info).ArgumentsCount(1)
       .Argument(0).IsString().Bind(addr).Error(&error))
    {
      RF24_conf_t *cc=THIS->_get_config();
      if(ConvertHexAddress(addr,addrc,cc->AddressWidth)) {
        //std::cout << "Write address set to " << addrc << std::endl;
        return MRET(THIS->_useWritePipe(addrc));
      }else return Nan::ThrowSyntaxError("Invalid address");
    } else return Nan::ThrowSyntaxError(error.c_str());

  }
  static NAN_METHOD(addReadPipe) {
    auto THIS=MTHIS(nRF24);
    v8::Local<v8::String> addr;
    std::string error;
    uint8_t addrc[6]; addrc[5]='\0';
    bool auto_ack;

    if(Nan::Check(info).ArgumentsCount(2)
       .Argument(0).IsString().Bind(addr)
       .Argument(1).Bind(auto_ack).Error(&error)) {
         RF24_conf_t *cc=THIS->_get_config();
         if(ConvertHexAddress(addr,addrc,cc->AddressWidth)) {
           //std::cout << "Read Address to " << addrc << " auto ACK:" << auto_ack << std::endl;
           return MRET(THIS->_addReadPipe(addrc,auto_ack));
         }else return Nan::ThrowSyntaxError("Invalid address");
    } else return Nan::ThrowSyntaxError(error.c_str());
  }

  static NAN_METHOD(removeReadPipe) {
    auto THIS=MTHIS(nRF24);
    int32_t pipe;
    std::string error;
    if(Nan::Check(info).ArgumentsCount(1)
       .Argument(0).Bind(pipe).Error(&error)){
         THIS->_removeReadPipe(pipe);
    }else return Nan::ThrowSyntaxError(error.c_str());
  }

  static inline Nan::Persistent<v8::Function> & constructor() {
    static Nan::Persistent<v8::Function> my_constructor;
    return my_constructor;
  }


  // Class attributtes
  int ce_,cs_;
  std::mutex radio_mutex,radio_write_mutex;
  RF24 *radio_;
  nRF24::ReaderWorker *worker_;
  RF24_conf_t *current_config;
  volatile bool is_powered_up_;
  volatile bool is_listening_;
  volatile bool is_enabled_;
  //uint8_t pipes_[6][5]; // Pipes
  bool    used_pipes_[6]; // Pipeflags

  friend class nRF24::ReaderWorker;
};

#endif
