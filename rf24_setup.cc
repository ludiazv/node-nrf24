#include "rf24.hpp"
#include "tryabort.hpp"


std::ostream& operator<<(std::ostream& out, RF24_conf_t &h) {
   return out << " PALevel:" << (int)h.PALevel << " EnableLna:" << (int)h.EnableLna << " DataRate:" << (int)h.DataRate
              << " Channel:" << (int)h.Channel << " CRCLength:" << (int)h.CRCLength
              << " Retries:("<< (int)h.retriesDelay << "+1*250us," << (int)h.retriesCount << " attempts)"
              << " PayloadSize:" << (int)h.PayloadSize << " AddressWidth:" << (int)h.AddressWidth
              << " AutoAck:" << (bool)h.AutoAck
              << " TxDelay:" << h.TxDelay
              << " PollBaseTime:" << h.PollBaseTime
              << " IRQ (negative is unused):" << h.irq
              << " AutoFailureRecovery:" << h.AutoFailureRecovery;
}





static RF24_conf_t DEFAULT_RF24_CONF={ RF24_PA_MIN ,  //PALevel
                                       1,             // EnableLna
                                       RF24_1MBPS ,   // DataRate
                                       76 ,           // Channel;
                                       RF24_CRC_16 ,  // CRCLength;
                                       5,             // retriesDelay
                                       15,            // retriesCount
                                       32,            // PayloadSize
                                       5 ,            // AddressWidth
                                       true ,         // AutoAck
                                       250,           // TxDelay
                                       RF24_DEFAULT_POLLTIME, // PollBaseTime
                                       -1,                     // IRQ
                                       false                   // AutoFailureRecovery        
                                     };

uint32_t  nRF24::_sequencer=0;

/* Setup and config methods */

/* Initialization methods */

NAN_METHOD(nRF24::begin) {
  auto THIS=MTHIS(nRF24);
  bool p_d=false;
  if(info.Length()==1 && info[0]->IsBoolean()) p_d=Nan::To<bool>(info[0]).FromJust();
  bool r=THIS->_begin(p_d); // init the radio

  if(r) THIS->_config(p_d);  // Configure with default configuration.
  MRET(r);
}

bool  nRF24::_begin(bool print_details) {
    bool res=false;
    if(radio_==NULL) radio_=new RF24(ce_,cs_,spi_speed_);

    if(radio_) {
      try{
        std::lock_guard<std::mutex> guard(radio_mutex);
      //try_and_catch_abort( [&] () -> void {
                                    res=radio_->begin();
                                    res= res && radio_->isChipConnected();
                                    if(res) {
                                      radio_->powerUp();  // Powerup and mask all IRQ
                                      radio_->maskIRQ(1,1,1); // Mask all IRQ
                                    }
                                    if(res && print_details) {
                                          radio_->printDetails();
                                    }
                                    if(res) _resetState(); // Reset State of radios To TX standby
     //});
      }catch(const std::exception& e){
       NRF24DBG(std::cout << "Exception _begin " << e.what() << std::endl);
     }catch(...) {
       std::cout << "Exception _begin ..." << std::endl;
     }
    } // if readio
    return res;
}

void nRF24::_resetState() {
  radio_->startListening();
  sleep_us(5000);
  radio_->flush_rx(); // Discart any bogus frame
  radio_->stopListening();
  sleep_us(1000);
  radio_->flush_tx(); // Make shure transmit buffer is empty
  is_listening_=false;
  radio_->failureDetected = 0; // clean any failure signal
}

NAN_METHOD(nRF24::destroy_object) {
  auto THIS=MTHIS(nRF24);
  THIS->_stop_read();
  THIS->_stop_write();
  THIS->Unref();
}

NAN_METHOD(nRF24::New) {
    if (info.IsConstructCall()) {
      int32_t ce,cs;
      int32_t spi_speed;
      std::string error;
      nRF24 *obj=NULL;
      if(Nan::Check(info).ArgumentsCount(2)
         .Argument(0).Bind(ce)
         .Argument(1).Bind(cs).Error(&error)) {
          obj = new nRF24(ce,cs);   
      } else if (Nan::Check(info).ArgumentsCount(3)
                .Argument(0).Bind(ce)
                .Argument(1).Bind(cs)
                .Argument(2).Bind(spi_speed).Error(&error)) {
                 obj = new nRF24(ce,cs,spi_speed);  
       } else return Nan::ThrowTypeError("nRF24 constructor ERROR:Wrong argument number!");

       // Wrap the radio object
       if(obj==NULL) return Nan::ThrowError("FATAL could not allocate nrf24 object.");
       obj->Wrap(info.This());
       obj->Ref(); // Avoid GC to purge the object prematurely
       info.GetReturnValue().Set(info.This());
    } else {
      return Nan::ThrowTypeError("nRF24 constructor ERROR: called constructor without new keyword!");
      /*const int argc = 1;
      v8::Local<v8::Value> argv[argc] = {info[0]};
      v8::Local<v8::Function> cons = Nan::New(constructor());
      info.GetReturnValue().Set(cons->NewInstance(argc, argv)); */

    }
}

nRF24::nRF24(int ce,int cs,int spi_speed) :
   ce_(ce), cs_(cs),spi_speed_(spi_speed),irq_(NULL),
  radio_(NULL), worker_(NULL),
  current_config(NULL),
  is_powered_up_(true),is_listening_(false), is_enabled_(true), failure_stat_(0) {
    for(int i=0;i<6;i++) {
      memset(&pipe_conf_[i],0,sizeof(RF24_pipe_configuration_t));
      pipe_conf_[i].ackmode=true;
      pipe_conf_[i].stream_info=1;
      memset(&stats_[i],0,sizeof(RF24_stats_t)); // reset stats
      read_buffer_[i].reserve(32);
      read_buffer_[i].clear();
    }
    pipe_conf_[0].stream_info=1024; // patch stream info for pipe 0 to default
  }

// Destructor freeResources
nRF24::~nRF24() {
    if(irq_) { irq_->stop(); delete irq_; }
    _stop_write();
    if(worker_) worker_->stop();
    if(radio_) delete radio_;
    if(current_config) delete current_config;
    current_config=NULL;
    radio_=NULL;
    worker_=NULL;
    irq_=NULL;
  }

/* Config */
NAN_METHOD(nRF24::config) {
    auto THIS=MTHIS(nRF24);
    v8::Local<v8::Object> _obj;
    bool print_details=false;
    std::string error;

    if(info.Length() > 0 && Nan::Check(info)
            .Argument(0).IsObject().Bind(_obj).Error(&error))
    {
      RF24_conf_t *cc=THIS->_get_config(); // Init or retrive config

      if(ObjHas(_obj,"PALevel"))   cc->PALevel=(uint8_t)ObjGetUInt(_obj,"PALevel");
      if(ObjHas(_obj,"EnableLna")) cc->EnableLna=(uint8_t)ObjGetBool(_obj,"EnableLna");
      if(ObjHas(_obj,"Channel"))  cc->Channel=(uint8_t)ObjGetUInt(_obj,"Channel");
      if(ObjHas(_obj,"DataRate")) cc->DataRate=(uint8_t)ObjGetUInt(_obj,"DataRate");
      if(ObjHas(_obj,"PayloadSize")) cc->PayloadSize=(uint8_t)ObjGetUInt(_obj,"PayloadSize");
      if(ObjHas(_obj,"retriesDelay")) cc->retriesDelay=(uint8_t)ObjGetUInt(_obj,"retriesDelay");
      if(ObjHas(_obj,"retriesCount")) cc->retriesCount=(uint8_t)ObjGetUInt(_obj,"retriesCount");
      if(ObjHas(_obj,"AddressWidth")) cc->AddressWidth=(uint8_t)ObjGetUInt(_obj,"AddressWidth");
      if(ObjHas(_obj,"CRCLength")) cc->CRCLength=(uint8_t)ObjGetUInt(_obj,"CRCLength");
      if(ObjHas(_obj,"AutoAck")) cc->AutoAck=(uint8_t)ObjGetBool(_obj,"AutoAck");
      if(ObjHas(_obj,"TxDelay")) cc->TxDelay=ObjGetUInt(_obj,"TxDelay");
      if(ObjHas(_obj,"Irq")) cc->irq=(int)ObjGetInt(_obj,"Irq");
      if(ObjHas(_obj,"PollBaseTime")) cc->PollBaseTime=ObjGetUInt(_obj,"PollBaseTime");
      if(ObjHas(_obj,"AutoFailureRecovery")) cc->AutoFailureRecovery=(uint8_t)ObjGetBool(_obj,"AutoFailureRecovery");
      // Validate Fieds and set to default if invalid

      if(cc->PALevel>3)   cc->PALevel=DEFAULT_RF24_CONF.PALevel;
      if(cc->Channel>126) cc->Channel=DEFAULT_RF24_CONF.Channel; // valid channels 0-126
      if(cc->DataRate>2) cc->DataRate=DEFAULT_RF24_CONF.DataRate;
      if(cc->PayloadSize <1 || cc->PayloadSize >32) cc->PayloadSize=DEFAULT_RF24_CONF.PayloadSize;
      if(cc->retriesCount>15) cc->retriesCount=DEFAULT_RF24_CONF.retriesCount;
      if(cc->retriesDelay>15) cc->retriesDelay=DEFAULT_RF24_CONF.retriesDelay;
      if(cc->AddressWidth<3 || cc->AddressWidth>5)  cc->AddressWidth=DEFAULT_RF24_CONF.AddressWidth;
      if(cc->CRCLength>2) cc->CRCLength=DEFAULT_RF24_CONF.CRCLength;
      if(cc->PollBaseTime < RF24_MIN_POLLTIME) cc->PollBaseTime=DEFAULT_RF24_CONF.PollBaseTime;


      if(info.Length() >= 2 && info[1]->IsBoolean()) print_details=Nan::To<bool>(info[1]).FromJust();
      THIS->_config(print_details); // do Config

    } else Nan::ThrowSyntaxError(error.c_str());

}

RF24_conf_t *nRF24::_get_config() {
   //std::lock_guard<std::mutex> guard(radio_mutex);
   if(current_config == NULL) current_config=new RF24_conf_t(DEFAULT_RF24_CONF);
   return current_config;
}

void nRF24::_config(bool print_details) {
  if(radio_ == NULL || !is_enabled_) return;
  RF24_conf_t *cc=this->_get_config(); // get config or init
  try {
  std::lock_guard<std::mutex> guard(radio_mutex);
  // Perform changes of current config
  //try_and_catch_abort([&]() -> void {
    radio_->setAutoAck(cc->AutoAck);
    radio_->setPALevel(cc->PALevel,cc->EnableLna);
    radio_->setDataRate((rf24_datarate_e)cc->DataRate);
    radio_->setChannel(cc->Channel);
    radio_->setPayloadSize(cc->PayloadSize);
    radio_->setRetries(cc->retriesDelay,cc->retriesCount);
    radio_->setAddressWidth(cc->AddressWidth);
    if(cc->CRCLength == RF24_CRC_DISABLED )
          radio_->disableCRC();
    else  radio_->setCRCLength((rf24_crclength_e)cc->CRCLength);
    radio_->txDelay= cc->TxDelay;
    // Adjust txDelay with a factor of speed based on RF24 code for LINUX
    if(cc->DataRate==RF24_250KBPS) radio_->txDelay = (uint32_t)(radio_->txDelay * 0.765) + 1;
    if(cc->DataRate==RF24_2MBPS) radio_->txDelay = (uint32_t)(radio_->txDelay * 1.82) + 1;
    //Configure the IRQ
    if(cc->irq>=0) {
      if(irq_!=NULL) {
          irq_->stop();
          delete irq_;
      }
      irq_=new RF24Irq(cc->irq);
      if(!irq_->begin(RF24Irq::DIR_INPUT,RF24Irq::EDGE_FALLING)) {
        delete irq_;
        irq_=NULL;
        cc->irq=-1; // Fallback to pooling
      } else {
        irq_->clear();
        radio_->maskIRQ(0,0,0); // No mask any interrupt
      }
    }  
    pipe_conf_[0].ackmode=cc->AutoAck; // set mode to write pipe the default
    if(print_details) {
        std::cout << "Radio details after config:" << std::endl;
        std::cout << "===========================" << std::endl;
        radio_->printDetails();
        std::cout << "Config internals:" << std::endl;
        std::cout << "check the values as incorrect values fallback to default values." << std::endl;
        std::cout << *cc << std::endl;
        std::cout << "===========================" << std::endl;
   }
    _resetState(); // Rest radio state.
  //});


  }catch(const std::exception& e){
    NRF24DBG(std::cout << "Exception _config " << e.what() << std::endl);
  }catch(...) {
    std::cout << "Exception _config ..." << std::endl;
  }

}

/* Pipe Management */
NAN_METHOD(nRF24::useWritePipe) {
  auto THIS=MTHIS(nRF24);
  v8::Local<v8::String> addr;
  std::string error;
  uint8_t addrc[6];
  bool auto_ack;

  if(info.Length() >=1 ) { // only one argument
    if(Nan::Check(info)
      .Argument(0).IsString().Bind(addr).Error(&error))
      {
          RF24_conf_t *cc=THIS->_get_config();
          auto_ack=cc->AutoAck;
          memset(addrc,0,6); // Clear addr.
          if(!ConvertHexAddress(addr,addrc,cc->AddressWidth)) return Nan::ThrowSyntaxError("Invalid address");
          if(info.Length()>=2 && info[1]->IsBoolean()) auto_ack=Nan::To<bool>(info[1]).FromJust();
          MRET(THIS->_useWritePipe(addrc,auto_ack));

      } else return Nan::ThrowSyntaxError(error.c_str());
  } else return Nan::ThrowSyntaxError("Missing parameters");

}

bool nRF24::_useWritePipe(uint8_t *pipe_name,bool auto_ack){
  if(!is_enabled_ || radio_==NULL) return false;
  bool res=false;
  try {
  std::lock_guard<std::mutex> guard(radio_mutex); // radio lock
  //try_and_catch_abort([&]() -> void {
          radio_->openWritingPipe(pipe_name);
          radio_->setAutoAck(0,auto_ack);
          memcpy(pipe_conf_[0].addr,pipe_name,5); // Copy address
          pipe_conf_[0].ackmode=auto_ack;
          pipe_conf_[0].stream_info=1024; // reset stream info
          res=pipe_conf_[0].in_use=true;
  //});
  }catch(const std::exception& e){
      NRF24DBG(std::cout << "Exception _useWritePipe " << e.what() << std::endl);
  }catch(...) {
     std::cout << "Exception _useWritePipe ..." << std::endl;
  }
  return res;
}

NAN_METHOD(nRF24::changeWritePipe) {
  auto THIS=MTHIS(nRF24);
  uint32_t maxstream;
  bool auto_ack;
  std::string error;

  if(Nan::Check(info).ArgumentsCount(2)
    .Argument(0).Bind(auto_ack)
    .Argument(1).Bind(maxstream).Error(&error)) {
      MRET(THIS->_changeWritePipe(auto_ack,maxstream));
  } else return Nan::ThrowSyntaxError(error.c_str());

}

bool nRF24::_changeWritePipe(bool auto_ack,uint16_t mm){
  if(!is_enabled_ || radio_==NULL) return false;
  bool res=false;
  try {
  std::lock_guard<std::mutex> guard(radio_mutex); // radio lock
  //try_and_catch_abort([&]() -> void {

          radio_->setAutoAck(0,auto_ack);
          pipe_conf_[0].ackmode=auto_ack;
          pipe_conf_[0].stream_info=mm; // reset stream info
          res=pipe_conf_[0].in_use=true;
  //});
  }catch(const std::exception& e){
      NRF24DBG(std::cout << "Exception _changeWritePipe " << e.what() << std::endl);
  }catch(...) {
      std::cout << "Exception _changeWritePipe ..." << std::endl;
  }
  return res;
}

NAN_METHOD(nRF24::addReadPipe) {
  auto THIS=MTHIS(nRF24);
  v8::Local<v8::String> addr;
  std::string error;
  uint8_t addrc[6];
  bool auto_ack;

  if(info.Length() >=1 ) { // only one argument
    if(Nan::Check(info)
      .Argument(0).IsString().Bind(addr).Error(&error))
      {
        RF24_conf_t *cc=THIS->_get_config();
        auto_ack=cc->AutoAck;
        if(!ConvertHexAddress(addr,addrc,cc->AddressWidth)) return Nan::ThrowSyntaxError("Invalid address");
        if(info.Length()>=2 && info[1]->IsBoolean()) auto_ack=Nan::To<bool>(info[1]).FromJust();
        MRET(THIS->_addReadPipe(addrc,auto_ack));

    } else return Nan::ThrowSyntaxError(error.c_str());
  } else return Nan::ThrowSyntaxError("Missing parameters");

}

int32_t nRF24::_addReadPipe(uint8_t *pipe_name,bool auto_ack) {
   int i=1;
   if(!is_enabled_ || radio_==NULL) return -1;
   while(pipe_conf_[i].in_use && i<6) i++;

   if(i<6 && i>0) {
     try {
      std::lock_guard<std::mutex> guard(radio_mutex); // radio lock
      //try_and_catch_abort([&]() -> void {
              radio_->openReadingPipe((uint8_t)i,pipe_name);
              radio_->setAutoAck((uint8_t)i,auto_ack);
              read_buffer_[i].reserve(32); // Reserve buffer
              read_buffer_[i].clear();
              memcpy(pipe_conf_[i].addr,pipe_name,5); // Save addr.
              pipe_conf_[i].stream_info=1; // Reset max stream to default
              pipe_conf_[i].ackmode=auto_ack;
              pipe_conf_[i].in_use=true;
      //});
      }catch(const std::exception& e){
          NRF24DBG(std::cout << "Exception _addReadPipe " << e.what() << std::endl);
      }catch(...) {
          std::cout << "Exception _addReadPipe ..." << std::endl;
      }
      return i;
   }else return -1;

}

NAN_METHOD(nRF24::changeReadPipe) {
  auto THIS=MTHIS(nRF24);
  int32_t  pipe;
  uint32_t maxmerge;
  bool auto_ack;
  std::string error;

  if(Nan::Check(info).ArgumentsCount(3)
    .Argument(0).Bind(pipe)
    .Argument(1).Bind(auto_ack)
    .Argument(2).Bind(maxmerge).Error(&error)) {
      if(maxmerge > RF24_MAX_MERGEFRAMES) return Nan::ThrowSyntaxError("Max merge frames is too high");
      MRET(THIS->_changeReadPipe(pipe,auto_ack,maxmerge));
  } else return Nan::ThrowSyntaxError(error.c_str());

}

bool nRF24::_changeReadPipe(int32_t number,bool auto_ack,uint16_t maxmerge){
  if(!is_enabled_ || radio_==NULL || number<1 || number>5 || !pipe_conf_[number].in_use) return false;
  bool res=false;
  RF24_conf_t *cc=this->_get_config();
  size_t reserve=cc->PayloadSize * maxmerge;
  try {
  std::lock_guard<std::mutex> guard(radio_mutex); // radio lock
  //try_and_catch_abort([&]() -> void {
        radio_->setAutoAck(number,auto_ack);
        read_buffer_[number].reserve(reserve); // Reserve buffer
        read_buffer_[number].clear();
        pipe_conf_[number].ackmode=auto_ack; // chang ack mode
        pipe_conf_[number].stream_info=maxmerge; // change max merge
        res=true;
  //});
  }catch(const std::exception& e){
      NRF24DBG(std::cout << "Exception _begin " << e.what() << std::endl);
  }catch(...) {
      std::cout << "Exception _begin ..." << std::endl;
  }
  return res;
}

NAN_METHOD(nRF24::removeReadPipe) {
  auto THIS=MTHIS(nRF24);
  int32_t pipe;
  std::string error;
  if(Nan::Check(info).ArgumentsCount(1)
     .Argument(0).Bind(pipe).Error(&error)){
       THIS->_removeReadPipe(pipe);
  }else return Nan::ThrowSyntaxError(error.c_str());
}

void nRF24::_removeReadPipe(int32_t number){
    if(number<=0 || number>5 || !is_enabled_ || radio_==NULL) return;
    bool all_closed=true;

    try {
    std::lock_guard<std::mutex> guard(radio_mutex); // radio lock
    //try_and_catch_abort([&]() -> void {
            radio_->closeReadingPipe((uint8_t)number);
            read_buffer_[number].reserve(32);
            read_buffer_[number].clear();
            pipe_conf_[number].in_use=false;
    //});

    for(int i=1;i<6;i++) all_closed=all_closed && !pipe_conf_[i].in_use;

    } catch(const std::exception& e){
        NRF24DBG(std::cout << "Exception _removeReadPipe " << e.what() << std::endl);
    } catch(...) {
      std::cout << "Exception _removeReadPipe ..." << std::endl;
    }

    if(all_closed)  _stop_read();
}

/* Failure management */

NAN_METHOD(nRF24::hasFailure) {
  auto THIS=MTHIS(nRF24);
  MRET(THIS->_hasFailure());
}

bool nRF24::_hasFailure() {
  if(!is_enabled_ || radio_==NULL) return false;
  std::lock_guard<std::mutex> guard(radio_mutex); // radio lock
  return radio_->failureDetected > 0;
}
NAN_METHOD(nRF24::restart) {
  auto THIS=MTHIS(nRF24);
  THIS->_restart();
}

void nRF24::_restart(){

    if(radio_!=NULL || radio_->failureDetected == 0) return;
    std::lock_guard<std::mutex> guard2(radio_write_mutex); // write lock avoid any write during recover
    bool wpack=pipe_conf_[0].ackmode; // back up write pipe ackmode that is reseted by configure.
    _begin(false); // Restart radio 
    _config(false); // Reconfigure radio
    pipe_conf_[0].ackmode=wpack;
    // Exclusion block for pipes adjustment
    {
      std::lock_guard<std::mutex> guard(radio_mutex);
      // Restore write pipe configuration
      if(pipe_conf_[0].in_use) {
         radio_->openWritingPipe(pipe_conf_[0].addr);
         radio_->setAutoAck(0,wpack);
      }
      // Restore reading pipes
      for(int i=1;i<6;i++) {
        if(pipe_conf_[i].in_use) {
          radio_->openReadingPipe(i,pipe_conf_[i].addr); // restore address
          radio_->setAutoAck(i,pipe_conf_[0].ackmode); // resore ack mode
        } else radio_->closeReadingPipe(i); // not in use make sure is closed
      }
    }
}


/* Common functions */
bool nRF24::_present() {
   bool res=false;
   if(radio_==NULL) return false;
   try {
     std::lock_guard<std::mutex> guard(radio_mutex);
   //try_and_catch_abort([&]() -> void {
        res=radio_->isChipConnected();
   //});
    }catch(const std::exception& e){
      NRF24DBG(std::cout << "Exception _present " << e.what() << std::endl);
    }catch(...) {
      std::cout << "Exception _present ..." << std::endl;
    }

   return res;
}

bool nRF24::_isP() {
  bool res=false;
  if(radio_==NULL) return false;
  try {
  std::lock_guard<std::mutex> guard(radio_mutex);
  //try_and_catch_abort([&]() -> void {
       res=radio_->isPVariant();
  //});
  }catch(const std::exception& e){
    NRF24DBG(std::cout << "Exception _isP " << e.what() << std::endl);
  }catch(...) {
    std::cout << "Exception _isP ..." << std::endl;
  }
  return res;
}


bool nRF24::_powerUp() {
    if(!is_enabled_ || radio_==NULL) return false;
    if(is_powered_up_) return true;
    bool res=false;

    try {
    std::lock_guard<std::mutex> guard(radio_mutex);
    //try_and_catch_abort([&]() -> void {
           radio_->powerUp();
           res=is_powered_up_=true;
    //});
    }catch(const std::exception& e){
      NRF24DBG(std::cout << "Exception _powerUp " << e.what() << std::endl);
    }catch(...) {
      std::cout << "Exception _powerUp ..." << std::endl;
    }
    return res;
}

bool nRF24::_powerDown() {
    if(!is_enabled_ || radio_==NULL) return false;
    if(!is_powered_up_) return true;
    bool res=false;
    try {
    std::lock_guard<std::mutex> guard(radio_mutex);
    //try_and_catch_abort( [&]() -> void {
        radio_->powerDown();
        res=true; is_powered_up_=false;
    //});
    }catch(const std::exception& e){
        NRF24DBG(std::cout << "Exception _powerDown " << e.what() << std::endl);
    } catch(...) {
        std::cout << "Exception _powerDown ..." << std::endl;
    }
    return res;
}

bool nRF24::_listen() {
    if(is_listening_) return true;
    if(!is_enabled_ || radio_==NULL) return false;
    bool res=false;
    try {
    std::lock_guard<std::mutex> guard(radio_mutex);
    //try_and_catch_abort( [&]() -> void {
        radio_->startListening();
        res=is_listening_=true;
    //});
    } catch(const std::exception& e){
        NRF24DBG(std::cout << "Exception _listen " << e.what() << std::endl);
    }catch(...) {
        std::cout << "Exception _listen ..." << std::endl;
    }
    return res;
}

bool nRF24::_transmit() {
    if(!is_listening_) return true;
    if(!is_enabled_ || radio_==NULL) return false;
    bool res=false;
    try {
    std::lock_guard<std::mutex> guard(radio_mutex);
    //try_and_catch_abort( [&]() -> void {
        radio_->stopListening();
        res=true; is_listening_=false;
    //});
    }catch(const std::exception& e){
        NRF24DBG(std::cout << "Exception _begin " << e.what() << std::endl);
    }catch(...) {
        std::cout << "Exception _begin ..." << std::endl;
    }
    return res;
}
