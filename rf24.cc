/*********************************************************************

 ********************************************************************/
#include "rf24.hpp"
#include "tryabort.hpp"

std::ostream& operator<<(std::ostream& out, RF24_conf_t &h) {
   return out << "PALevel:" << (int)h.PALevel << " DataRate:" << (int)h.DataRate
              << " Channel:" << (int)h.Channel << " CRCLength:" << (int)h.CRCLength
              << " Retries:("<< (int)h.retriesDelay << "," << (int)h.retriesCount << ")"
              << " PayloadSize:" << (int)h.PayloadSize << " AddressWidth:" << (int)h.AddressWidth;
}

static RF24_conf_t DEFAULT_RF24_CONF={ RF24_PA_MIN , RF24_1MBPS , 76 , RF24_CRC_16 , 5, 15, 32, 5 };

/* Reader */
void nRF24::ReaderWorker::Execute(const Nan::AsyncProgressWorker::ExecutionProgress& progress_) {
  char frame[32+1];
  uint8_t pipe;
  useconds_t half=poll_timeus/4;
  stopped_=false;
  RF24_conf_t *cc=device._get_config();

  while(!want_stop) {
    device.radio_write_mutex.lock(); // avoid writes during read polling
    // Assure that listening
    device._listen();
    if(device._available(&pipe)) {
      memset(frame,0,sizeof(frame)); // reset all zero frame
      frame[0]=pipe;
      if(device._read(&frame[1], cc->PayloadSize)){
         progress_.Send(frame,(cc->PayloadSize)+1);
          error_count=0; // Reset Error count
      }
      else {
         if(++error_count>RF24_MAX_ERROR_COUNT) {
           device.radio_write_mutex.unlock();
           break; // Extit loop
         }
      }
    }
    if(!want_stop && !device._available() && want_towrite) {
        device.radio_write_mutex.unlock();
        usleep(poll_timeus); // Give some time write
    } else device.radio_write_mutex.unlock();
    usleep(half);
  }
  want_stop=false;
  stopped_=true;
}

void nRF24::ReaderWorker::HandleProgressCallback(const char *data, size_t size) {
  Nan::HandleScope scope;
  int pipe=(int)data[0];
  v8::Local<v8::Value> argv[] = {
    //Nan::NewBuffer( ((char*)data)+1,size-1).ToLocalChecked(), // Buffer with the data
    Nan::CopyBuffer( ((char*)data)+1,size-1).ToLocalChecked(),
    Nan::New(pipe)
  };
  progress->Call(2, argv);
}

void nRF24::ReaderWorker::HandleOKCallback() {
    Nan::HandleScope scope;
    v8::Local<v8::Value> argv[] = {
      Nan::New(stopped_),
      Nan::New(want_stop),
      Nan::New(error_count) };
      callback->Call(3,argv);
}

void nRF24::ReaderWorker::stop() {
  if(!stopped_) {
     want_stop=true;
     usleep(poll_timeus*2);
  }
}

/* MAIN CLASS */
bool  nRF24::_begin(bool print_details) {
    bool res=false;
    std::lock_guard<std::mutex> guard(radio_mutex);
    if(radio_==NULL) radio_=new RF24(ce_,cs_);
    if(radio_) {
      try_and_catch_abort( [&] () -> void {
                                    res=radio_->begin();
                                    res= res && radio_->isChipConnected();
                                    if(res && print_details) radio_->printDetails();
     });
    }
    return res;
}

bool nRF24::_present() {
   bool res=false;
   if(radio_==NULL) return false;
   std::lock_guard<std::mutex> guard(radio_mutex);
   try_and_catch_abort([&]() -> void {
        res=radio_->isChipConnected();
   });
   return res;
}
bool nRF24::_isP() {
  bool res=false;
  if(radio_==NULL) return false;
  std::lock_guard<std::mutex> guard(radio_mutex);
  try_and_catch_abort([&]() -> void {
       res=radio_->isPVariant();
  });
  return res;
}

RF24_conf_t *nRF24::_get_config() {
   std::lock_guard<std::mutex> guard(radio_mutex);
   if(current_config == NULL) current_config=new RF24_conf_t(DEFAULT_RF24_CONF);
   return current_config;
}

void nRF24::_config() {
  if(radio_ == NULL || !is_enabled_) return;
  RF24_conf_t *cc=this->_get_config(); // get config or init
  std::lock_guard<std::mutex> guard(radio_mutex);
  // Perform changes of current config
  try_and_catch_abort([&]() -> void {
    radio_->setAutoAck(true);
    radio_->setPALevel(cc->PALevel);
    radio_->setDataRate((rf24_datarate_e)cc->DataRate);
    radio_->setChannel(cc->Channel);
    radio_->setPayloadSize(cc->PayloadSize);
    radio_->setRetries(cc->retriesDelay,cc->retriesCount);
    radio_->setAddressWidth(cc->AddressWidth);
    if(cc->CRCLength == RF24_CRC_DISABLED )
          radio_->disableCRC();
    else  radio_->setCRCLength((rf24_crclength_e)cc->CRCLength);
  });

}

bool nRF24::_write(void *data,size_t r_length){
    bool res=false;
    if(!is_enabled_) return false;
    if(worker_!=NULL) worker_->want_write(true);
    std::lock_guard<std::mutex> guard2(radio_write_mutex); // write lock
    //std::cout << "try to w" << std::endl;
    if(_powerUp() && _transmit()) {
      std::lock_guard<std::mutex> guard(radio_mutex); // radio lock
      try_and_catch_abort([&]() -> void {
              res=radio_->write(data,r_length);
      });
    }
    if(worker_!=NULL) worker_->want_write(false);
    _listen(); // Back to listen
    return res;
}
bool nRF24::_useWritePipe(uint8_t *pipe_name){
  if(!is_enabled_) return false;
  bool res=false;
  std::lock_guard<std::mutex> guard(radio_mutex); // radio lock
  try_and_catch_abort([&]() -> void {
          radio_->openWritingPipe(pipe_name);
          res=used_pipes_[0]=true;
  });
  return res;
}
int32_t nRF24::_addReadPipe(uint8_t *pipe_name,bool auto_ack) {
   int i=1;
   if(!is_enabled_) return -1;
   while(used_pipes_[i] && i<6) i++;
   //std::cout << "Free pipe " << i << std::endl;
   if(i<6) {
      std::lock_guard<std::mutex> guard(radio_mutex); // radio lock
      try_and_catch_abort([&]() -> void {
              radio_->openReadingPipe((uint8_t)i,pipe_name);
              radio_->setAutoAck((uint8_t)i,auto_ack);
              used_pipes_[i]=true;
      });
      return i;
   }else return -1;
}
void nRF24::_removeReadPipe(int32_t number){
    if(number<=0 || number>6 || !is_enabled_) return;
    std::lock_guard<std::mutex> guard(radio_mutex); // radio lock
    try_and_catch_abort([&]() -> void {
            radio_->closeReadingPipe((uint8_t)number);
            used_pipes_[number]=false;
    });
    bool all_closed=true;
    for(int i=1;i<6;i++) all_closed=all_closed && !used_pipes_[i];
    if(all_closed)  _stop_read();
}

bool nRF24::_available(uint8_t *pipe) {
      bool res=false;
      if(!is_enabled_) return false;
      //if(_powerUp() && _listen()) {
          std::lock_guard<std::mutex> guard(radio_mutex); // radio lock
          try_and_catch_abort([&]() -> void {
                  res= (pipe == NULL ) ? radio_->available() : radio_->available(pipe);
          });
      //}
      return res;
}

bool nRF24::_read(void *data,size_t r_length) {
    bool res=false;
    if(!is_enabled_) return false;
    if(_powerUp() && _listen()) {
      std::lock_guard<std::mutex> guard(radio_mutex);
      if(r_length==0 || r_length>radio_->getPayloadSize()) r_length=radio_->getPayloadSize();
      try_and_catch_abort([&]() -> void {
              radio_->read(data,r_length);
              res=true;
      });
    }
    return res;
}
void nRF24::_stop_read(){
  if(!is_enabled_ || worker_==NULL) return;
  worker_->stop();
  worker_=NULL;
}

bool nRF24::_powerUp() {
    if(!is_enabled_) return false;
    if(is_powered_up_) return true;
    bool res=false;
    std::lock_guard<std::mutex> guard(radio_mutex);
    try_and_catch_abort([&]() -> void {
           radio_->powerUp();
           res=is_powered_up_=true;
    });
    return res;
}

bool nRF24::_powerDown() {
    if(!is_enabled_) return false;
    if(!is_powered_up_) return true;
    bool res=false;
    std::lock_guard<std::mutex> guard(radio_mutex);
    try_and_catch_abort( [&]() -> void {
        radio_->powerDown();
        res=true; is_powered_up_=false;
    });
    return res;
}

bool nRF24::_listen() {
    if(!is_enabled_) return false;
    if(is_listening_) return true;
    bool res=false;
    std::lock_guard<std::mutex> guard(radio_mutex);
    try_and_catch_abort( [&]() -> void {
        radio_->startListening();
        res=is_listening_=true;
    });
    return res;
}

bool nRF24::_transmit() {
    if(!is_enabled_) return false;
    if(!is_listening_) return true;
    bool res=false;
    std::lock_guard<std::mutex> guard(radio_mutex);
    try_and_catch_abort( [&]() -> void {
        radio_->stopListening();
        res=true; is_listening_=false;
    });
    return res;
}
