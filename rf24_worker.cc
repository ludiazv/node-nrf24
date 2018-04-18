#include "rf24.hpp"
#include "tryabort.hpp"
#include "irq.hpp"


/* Reader Class */
/* ============ */
void nRF24::ReaderWorker::Execute(const RF24AsyncWorker::ExecutionProgress& progress_) {
  /* Legacy Implementation
  char frame[32+1];
  uint8_t pipe;
  useconds_t half=poll_timeus/4;
  stopped_=false;
  RF24_conf_t *cc=device._get_config();

  while(!want_stop) {
    device.radio_write_mutex.lock(); // avoid writes during read polling
    // Assure that listening
    device._listen();
    while(device._available(&pipe)) {
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
  */
  stopped_=false;
  std::set<uint8_t> pipes;
  //RF24AsyncType_t tmp_msg[5];
  int ret;
  bool more_pending, lock_or_continue=true;
  // Delays
  struct timespec micro_sleep,short_sleep,base_sleep;
  //struct timespec start_measure;
  UsToTimeSpec(650, &micro_sleep);
  UsToTimeSpec(poll_timeus/4,&short_sleep);
  UsToTimeSpec(poll_timeus, &base_sleep);

  // Init IRQ if needed
  //m_start(&start_measure);
  RF24Irq *irq=NULL;
  if(device._use_irq()) {
      irq=new RF24Irq(device._get_irq());
      if(!irq->begin(RF24Irq::DIR_INPUT,RF24Irq::EDGE_FALLING)) {
        delete irq;
        irq=NULL;
      }
  }
  device._cleanBuffers(NULL); // Sure to clear all buffers
  // Main loop
  while(!want_stop) {
    if(lock_or_continue) {
      device.radio_write_mutex.lock();
      device._listen();
    }

    ret=device._read_buffered(pipes,more_pending);
    if(ret>0) { // Reads ready
      //std::cout << "Ret:" << ret << " more pendig:"<< more_pending << std::endl;
      //std::cout << "R:" << ret << ", T:" << m_end(&start_measure,true) << "us, M:"<< more_pending << " / ";
      //m_start(&start_measure);
      size_t n_pipes=pipes.size();
      //RF24AsyncType *tmp_msg=new RF24AsyncType[n_pipes];
      int j=0;
      for(auto i=pipes.begin();i!=pipes.end();i++){
         tmp_msg[j].pipe=*i;
         tmp_msg[j].size=device._get_raw_buffer_size(*i);
         memcpy(&tmp_msg[j].buffer[0],device._get_raw_buffer(*i),tmp_msg[j].size);
         //tmp_msg[j].buffer[0]=(uint8_t) *i;
         //memcpy(&tmp_msg[j].buffer[1],device._get_raw_buffer(*i),tmp_msg[j].size);
         //progress_.Send(&tmp_msg[j].buffer[0],tmp_msg[j].size+1);
         j++;
      }

      progress_.Send(&tmp_msg[0],n_pipes); // Send it , this will create a copy of tmp_msg
      device._cleanBuffers(&pipes);
      pipes.clear();
      lock_or_continue=false;
      if(!more_pending) sleep_ts(&micro_sleep);
      continue;
    }else { // Error || nothing
      lock_or_continue=true;
      device.radio_write_mutex.unlock(); // writers can write
      if(ret<0) { // Error
        error_count++;
        if(error_count>RF24_MAX_ERROR_COUNT) break; // Extit loop
      }
      else {// Nothing more_pending
          error_count=0;
      }
    }
    if(want_stop) break;
    if(irq==NULL) {
      // If no IRQ is used we do pooling giving more time if want_towrite
      //m_start(&start_measure);
      if(want_towrite) sleep_ts(&base_sleep); else sleep_ts(&short_sleep);
      //std::cout<<"[P:"<< m_end(&start_measure) << "us]";
      //std::cout<< "P";
    } else { // Use IRQ pooling

          int irq_ret;
          //m_start(&start_measure);
          //do {
          irq_ret=irq->wait(false,RF24_IRQ_POLLTIME);
          //}while(!want_stop && irq_ret==0);
          if(irq_ret>0) continue;
          if(irq_ret<0) {
            error_count++;
            if(error_count>RF24_MAX_ERROR_COUNT) break; // Extit loop
          }
    }

  } // While loop

  if(!lock_or_continue) device.radio_write_mutex.unlock();
  if(irq) delete irq;

  stopped_=true;
}

void nRF24::ReaderWorker::HandleProgressCallback(const RF24AsyncType *data, size_t size) {
  Nan::HandleScope scope;
  /*Legacy Implementation
  int pipe=(int)data[0];
  v8::Local<v8::Value> argv[] = {
    //Nan::NewBuffer( ((char*)data)+1,size-1).ToLocalChecked(), // Buffer with the data
    Nan::CopyBuffer( ((char*)data)+1,size-1).ToLocalChecked(),
    Nan::New(pipe)
  }; */

  //if(stopped_) return;
  auto arr=Nan::New<v8::Array>(size);
  for(size_t j=0;j<size;j++) {
    //if(data[j].size>0) {
    //std::cout << "S:" << data[j].size << (void*)(&data[j].buffer[0]);
    auto o=Nan::New<v8::Object>();
    Nan::Set(o,Nan::New("pipe").ToLocalChecked(),Nan::New(data[j].pipe));
    Nan::Set(o,Nan::New("data").ToLocalChecked(),
               Nan::CopyBuffer(&data[j].buffer[0],data[j].size).ToLocalChecked());
    Nan::Set(arr,j,o);
  //} else std::cout<< "N";
  }

  v8::Local<v8::Value> argv[] ={
    arr,
    Nan::New(size)
  };

  progress->Call(2, argv,this->async_resource);
}

void nRF24::ReaderWorker::HandleOKCallback() {
    Nan::HandleScope scope;
    v8::Local<v8::Value> argv[] = {
      Nan::New(stopped_),
      Nan::New(want_stop),
      Nan::New(error_count) };
      callback->Call(3,argv,this->async_resource);
}

void nRF24::ReaderWorker::stop() {
  if(!stopped_) {
     want_stop=true;
     if(device._use_irq()) sleep_us(40000+RF24_IRQ_POLLTIME+(RF24_DEFAULT_POLLTIME/2));
     else sleep_us((poll_timeus*2)+40000);
  }
}

void nRF24::ReaderWorker::want_write(){
  std::lock_guard<std::mutex> guard(wantwrite_mutex);
  want_towrite++;
}

void nRF24::ReaderWorker::no_want_write()
{
    std::lock_guard<std::mutex> guard(wantwrite_mutex);
    if(want_towrite>0) want_towrite--;
}

/* Writter Class */
void nRF24::WriterWorker::Execute(){

  // Compute n_packets
  RF24_conf_t *cc=device._get_config();
  pck_size= cc->PayloadSize;
  size_t rem= buffer_size % pck_size;
  size_t n_packets= buffer_size / pck_size;
  tx_requested= n_packets + (( rem ==0 ) ? 0 : 1);
  // 1st Lock with the possibility of aborting pending
  //std::cout << "[BL]";
  {
  std::lock_guard<std::mutex> lock(device.write_abort_mutex);
  //std::cout << "[AL]";
  started_=true; // Transmission started
  if(!aborted_ && buffer!=NULL) {
    if(buffer_size <= pck_size)
      tx_ok=device._write(buffer,buffer_size,1,pck_size);
    else
      tx_ok=device._write(buffer,buffer_size,n_packets,pck_size);
  }
  finished_=true; // Transmission finished
  device.removeWriterWorker(); // Remove first in queue
  //device.write_abort_mutex.unlock();
  //std::cout << "[AR]";
  }
  if(buffer) { delete [] buffer, buffer=NULL; }

}

void nRF24::WriterWorker::HandleOKCallback(){
  Nan::HandleScope scope;
  bool success= (tx_requested == tx_ok);
  v8::Local<v8::Value> argv[] = {
    Nan::New(success),
    Nan::New(tx_ok),
    Nan::New(tx_ok*pck_size),
    Nan::New(tx_requested),
    Nan::New(pck_size),
    Nan::New(aborted_)
    };
    callback->Call(6,argv,this->async_resource);
}
