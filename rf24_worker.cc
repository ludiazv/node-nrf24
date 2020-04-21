#include "rf24.hpp"
#include "tryabort.hpp"


/* Reader Class */
/* ============ */
std::mutex nRF24::ReaderWorker::one_reader_mutex; // Static mutex
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
  // Main control variables
  //std::lock_guard<std::mutex>  one_reader_guard(nRF24::ReaderWorker::one_reader_mutex); // Mutex to control accidental lauch of multiple readers
  stopped_=false;             // Stopped flag
  std::set<uint8_t> pipes;    // Pipes set
  int ret;                    // Number of packets received
  bool more_pending=false;    // Reader reported more items pending
  bool continue_polling=true; // Control flag to keep polling reads 
  // Delays 
  struct timespec micro_sleep;  // Small sleep
  struct timespec short_sleep;  // short wait polling
  struct timespec base_sleep;   // full wait polling
  
  UsToTimeSpec(500, &micro_sleep);   // Computte the timespecs
  UsToTimeSpec(poll_timeus/4,&short_sleep);
  UsToTimeSpec(poll_timeus, &base_sleep);
  //-----

  // Init IRQ if needed
  //struct timespec start_measure;
  //m_start(&start_measure);
  device._cleanBuffers(NULL); // Sure to clear all buffers
  // Main loop Exit condtions:
  //   case A) user want_stop flaging want_stop variable
  //   case B) error count > MAX_ERROR (break call inside the loop)
  while(!want_stop) {
    
    { // Preparantion and exclusion
    std::lock_guard<std::mutex> writeguard(device.radio_write_mutex);
    //device.radio_write_mutex.lock(); // Adquire the mutex for write (exclude writers or wait for finish)
    device._listen(); // Set the radio in listen mode.
    continue_polling=true; // Try allways to read.
    // Polling loop Exit conditions:
    // A) no available transmissions continue_polling will be false
    //  Continue pollin will be on if there are pending writes. The loop tries twice if something is received
    while(continue_polling) {
      ret=device._read_buffered(pipes,more_pending);
      //std::cout << "{R:" << ret << ", T:" << m_end(&start_measure,true) << "ms, M:"<< more_pending << "}";
      if(ret>0) { // Reads ready
        //std::cout << "Ret:" << ret << " more pendig:"<< more_pending << std::endl;
        //std::cout << "R:" << ret << ", T:" << m_end(&start_measure,true) << "ms, M:"<< more_pending << " / ";
        //m_start(&start_measure);
        size_t n_pipes=pipes.size(); // number of pipes
        int j=0;
        for(auto i=pipes.begin();i!=pipes.end();i++) { // Copy the buffer locally
            tmp_msg[j].pipe=*i;
            tmp_msg[j].size=device._get_raw_buffer_size(*i);
            memcpy(&tmp_msg[j].buffer[0],device._get_raw_buffer(*i),tmp_msg[j].size);
            j++;
        }
        progress_.Send(&tmp_msg[0],n_pipes); // Send it
        device._cleanBuffers(&pipes);       // Clear the buffer
        pipes.clear();         // Clear the pipes set
        if(!more_pending) sleep_ts(&micro_sleep); // optimization: give some minimal sleep to check if more is comming
      } else {
          continue_polling=false; // Nothing to read stop -> stop polling
          error_count = (ret<0) ? error_count+1 : 0;  // if error increase the error count / if no erro clear error conunt
      }     
    } // Pooling loop

    //device.radio_write_mutex.unlock(); // writers can write if they want
    
    }// Mutex sextion
    if(error_count>RF24_MAX_ERROR_COUNT) break; // Extit loop if error cuont exedd limit
    // Wait section
    if(device._use_irq()) {
        //m_start(&start_measure);
        int irq_ret;
        //std::cout << "[IB]";
        //do {
        irq_ret=device._waitIrq(RF24_IRQ_POLLTIME /*-1*/);
        //} while(want_towrite>0 && irq_ret>0);
        //std::cout <<"[IE]";
        //std::cout<<"[P:"<< m_end(&start_measure,true) << "ms] [irq ret:" << irq_ret << "]";
        //m_start(&start_measure);
        if(irq_ret<0) error_count++; // if negative increase error
    } else { // Use poolling
        // If no IRQ is used we do pooling giving more time if want_towrite
        //m_start(&start_measure);
        if(want_towrite>0) sleep_ts(&base_sleep); else sleep_ts(&short_sleep); // If want to write give more time
        //std::cout<<"[P:"<< m_end(&start_measure) << "us]";
        //std::cout<< "P"; 
    } // Wait section
  } // Main loop
  //std::cout <<"[reader Stop]";
  stopped_=true; // stopped mark
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
    Nan::New((uint32_t)size)
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

void nRF24::ReaderWorker::no_want_write(){
    std::lock_guard<std::mutex> guard(wantwrite_mutex);
    if(want_towrite>0) want_towrite--;
}

/* Writter Class */
/* ============= */
void nRF24::WriterWorker::Execute(){

  // Compute n_packets
  RF24_conf_t *cc=device._get_config();
  pck_size= cc->PayloadSize;
  size_t rem= buffer_size % pck_size;
  size_t n_packets= buffer_size / pck_size;
  tx_requested= n_packets + (( rem ==0 ) ? 0 : 1);
  //std::cout << "Size:" << buffer_size << " pkt size:" << (int)pck_size << 
  //             "full pck:" << n_packets << " requested:" << tx_requested << std::endl;
  // 1st Lock with the possibility of aborting pending
  //std::cout << "[BL]";
  {
  std::lock_guard<std::mutex> abort_guard(device.write_abort_mutex);
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
  //std::cout << "TXOK:" << tx_ok;
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
