/*********************************************************************

 ********************************************************************/
#include "rf24.hpp"
#include "tryabort.hpp"


NAN_METHOD(nRF24::read) {
    auto THIS=MTHIS(nRF24);
    v8::Local<v8::Function> _progress;
    v8::Local<v8::Function> _callback;
    std::string error;
    char name[128];

    if (Nan::Check(info).ArgumentsCount(2)
            .Argument(0).IsFunction().Bind(_progress)
            .Argument(1).IsFunction().Bind(_callback).Error(&error))
    {
          if(THIS->is_enabled_ && THIS->worker_==NULL) {
            RF24_conf_t *cc=THIS->_get_config();
            sprintf(name,"rf24:r:%d",_sequencer++);
            Nan::Callback *progress = new Nan::Callback(_progress);
            Nan::Callback *callback = new Nan::Callback(_callback);
            Nan::AsyncQueueWorker(new nRF24::ReaderWorker(name,progress,callback,*THIS,cc->PollBaseTime));
          }
    }
    else
    {
      return Nan::ThrowSyntaxError(error.c_str());
    }
}

void nRF24::_cleanBuffers(const std::set<uint8_t>*pipes){
  if(pipes!=NULL){ // Reset selected
    for(auto it=pipes->begin();it!=pipes->end(); it++) read_buffer_[*it].clear();
  } else {
    for(int i=0;i<6;i++) read_buffer_[i].clear();
  } // Reset all

}

void nRF24::_copyBuffers(const std::set<uint8_t>*pipes,std::vector<uint8_t> *to){
  for(auto it=pipes->begin();it!=pipes->end(); it++) to[*it]=read_buffer_[*it];
}

void nRF24::_stop_read(){
  if(!is_enabled_ || radio_==NULL || worker_==NULL) return;
  worker_->stop();
  worker_=NULL;
}

int nRF24::_read_buffered(std::set<uint8_t> &pending,bool &more_available){
  if(radio_==NULL || !is_enabled_) return -1; // Return error
  int ret=-1;
  uint32_t n_err=0;

  RF24_conf_t *cc=this->_get_config();
  try {
    std::lock_guard<std::mutex> guard(radio_mutex);
    more_available=false;
  //try_and_catch_abort([&]() -> void {
      uint8_t  pipe;
      bool no_maxlen=true;
      uint8_t frame[32+1]; //frame[32]='\0';

      while(radio_->available(&pipe) && no_maxlen && n_err< RF24_MAX_BUFFREAD_ERR) {
         if(pipe<1 || pipe >5 ) { // Detect pipe glich
           n_err++;
           sleep_us(1000);
           continue;
         }
         radio_->read(&frame,cc->PayloadSize); // Read the frame
         // Buffer the frame
         read_buffer_[pipe].insert(read_buffer_[pipe].end(),frame,frame+cc->PayloadSize);
         no_maxlen= (read_buffer_[pipe].size() < ((pipe_conf_[pipe].stream_info)*cc->PayloadSize)); // Reached max merge
         ret++;
         pending.insert(pipe);
         stats_[pipe].rcv++; // Colect stats
      }
      more_available=radio_->available(); // report if there are more pending
      ret++; // 0 if nothing read, or number of frames
  //});
  }catch(const std::exception& e){
      NRF24DBG(std::cout << "Exception _read_buffered " << e.what() << std::endl);
  }catch(...) {
      std::cout << "Exception _read_buffered ..." << std::endl;
  }
  //if(n_err>0) NRF24DBG(std::cout << "Pipe glich #:" << n_err);
  return ret;
}

int nRF24::_waitIrq(int32_t timeout_ms,bool clear) {
  if(irq_){
    return irq_->wait(clear,timeout_ms);
  }
  else {
    if(timeout_ms>0) sleep_us(1000*timeout_ms); // Fallback to sleep
    return 0;  // Return timewait
  }
}

NAN_METHOD(nRF24::write) {
    auto THIS=MTHIS(nRF24);
    v8::Local<v8::Object> buffer;
    v8::Local<v8::Function> _callback;
    std::string error;

    if(info.Length()>=1 && Nan::Check(info)
       .Argument(0).IsBuffer().Bind(buffer).Error(&error)) {
         RF24_conf_t *cc=THIS->_get_config();
         size_t size= node::Buffer::Length(buffer);
         size=(size>cc->PayloadSize) ? cc->PayloadSize : size;
         if(info.Length()>=2) {
           if(Nan::Check(info).Argument(1)
             .IsFunction().Bind(_callback).Error(&error)) {
               Nan::Callback *callback = new Nan::Callback(_callback);
               if(callback != NULL) MRET(THIS->addWriterWorker(callback,node::Buffer::Data(buffer),size));
               else MRET(Nan::False());
            } else Nan::ThrowSyntaxError(error.c_str());
         }else { // SyncWrite
           MRET(THIS->_write(node::Buffer::Data(buffer),size,1,cc->PayloadSize));
         }
    } else return Nan::ThrowSyntaxError(error.c_str());

}

NAN_METHOD(nRF24::stream){
  auto THIS=MTHIS(nRF24);
  v8::Local<v8::Object> buffer;
  v8::Local<v8::Function> _callback;
  std::string error;

  if( Nan::Check(info).ArgumentsCount(2)
     .Argument(0).IsBuffer().Bind(buffer)
     .Argument(1).IsFunction().Bind(_callback).Error(&error)) {
       size_t size= node::Buffer::Length(buffer);
       size= (size > THIS->pipe_conf_[0].stream_info) ? THIS->pipe_conf_[0].stream_info : size; // Crop to steam max size
       Nan::Callback *callback = new Nan::Callback(_callback);
       if(callback!=NULL)
          MRET(THIS->addWriterWorker(callback,node::Buffer::Data(buffer),size));
       else MRET(Nan::False());
  } else return Nan::ThrowSyntaxError(error.c_str());

}


int nRF24::_write(void *data,size_t r_length,size_t n_packets, size_t p_size){
    int ret=0;
    if(!is_enabled_ || radio_==NULL) return false;
    if(worker_!=NULL) worker_->want_write();
    try {
    std::lock_guard<std::mutex> guard2(radio_write_mutex); // write lock

    if(_powerUp() && _transmit()) {
      std::lock_guard<std::mutex> guard(radio_mutex); // radio lock
      //try_and_catch_abort([&]() -> void {
             if(n_packets==1) {
              bool res=radio_->write(data,r_length); // Simple write
              if(res) {stats_[0].snd++; ret++; } else stats_[0].sndError++;
             }
             else { // Streaming

               size_t i=0,r=r_length % p_size;
               struct timespec pause;
               bool ares=true;
               //NRF24DBG(std::cout << "Stream:" << r_length << " n_packets:" << n_packets << "remainder:" << r << std::endl);
               m_start(&pause); // Start measuring transmission (only used for non ACK)
               while(i<n_packets && ares) {
                 ares= radio_->writeFast( (uint8_t*)data+(p_size*i) , p_size );
                 i++;
                 // if no ACK dont hold the radio tx more than 4ms (hw requirement)
                 if(!pipe_conf_[0].ackmode && m_end(&pause) > 3150){
                   m_start(&pause);
                   radio_->txStandBy(); // Wait to finish pending transfer.
                 }
               }
               if(r>0 && ares) {
                 ares=radio_->writeFast( (uint8_t*)data+(p_size*i) , r);
                 i++;
               }
               bool tx= radio_->txStandBy(); // Wait for final StandBy;
               if(ares && tx) {
                    stats_[0].snd+=i;
                    ret=i;
               } else {
                  ret= i - 1;
                  stats_[0].snd+=ret;
                  stats_[0].sndError+=n_packets-i+1;
                  stats_[0].sndError+= (r>0) ? 1 : 0;
                }
             }
      //});
      
     }
    }catch(const std::exception& e) {
      NRF24DBG(std::cout << "Exeception write_ " << e.what() << std::endl);
    } catch(...){
      std::cout << "Exeception write_ ..."<< std::endl;
    }
    if(worker_!=NULL) worker_->no_want_write();

    if(ret==0) {
      if(radio_->failureDetected >0 ) {
          failure_stat_++;
          auto cc=_get_config();
          if(cc!=NULL && cc->AutoFailureRecovery) _restart();
      }
    }

    return ret;
}

bool nRF24::addWriterWorker(Nan::Callback *cb,const char *buff,size_t size) {
  std::lock_guard<std::mutex> guard(write_queue_mutex);
  _sequencer++;
  char name[128];
  sprintf(name,"rf24:w:%d",_sequencer);

  auto ww=new nRF24::WriterWorker(name, cb ,*this,buff,size);
  if(ww!=NULL) {
     write_queue_.push(ww);
     Nan::AsyncQueueWorker(ww);
     return true;
  }
  else return false;
}

void nRF24::removeWriterWorker() {
  std::lock_guard<std::mutex> guard(write_queue_mutex);
  if(!write_queue_.empty()) write_queue_.pop();
}

void nRF24::_stop_write() {
  std::lock_guard<std::mutex> guard(write_queue_mutex);
  while(!write_queue_.empty()){
    auto w=write_queue_.front();
    if(!w->started()) w->abort(); // Runing is not aborted
    write_queue_.pop(); // Remove from queue
  }

}

/* Legacy functions to remove in the future
 * deprecated
 */
bool nRF24::_available(uint8_t *pipe) {
      bool res=false;
      if(!is_enabled_ || radio_==NULL) return false;
      //if(_powerUp() && _listen()) {
          std::lock_guard<std::mutex> guard(radio_mutex); // radio lock
          //try_and_catch_abort([&]() -> void {
                  res= (pipe == NULL ) ? radio_->available() : radio_->available(pipe);
          //});
      //}
      return res;
}

bool nRF24::_read(void *data,size_t r_length) {
    bool res=false;
    if(!is_enabled_ || radio_==NULL) return false;
    if(_powerUp() && _listen()) {
      std::lock_guard<std::mutex> guard(radio_mutex);
      if(r_length==0 || r_length>radio_->getPayloadSize()) r_length=radio_->getPayloadSize();
      //try_and_catch_abort([&]() -> void {
              radio_->read(data,r_length);
              res=true;
      //});
    }
    return res;
}
