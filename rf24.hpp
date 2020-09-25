#ifndef _NRF24_NODE_HPP_
#define _NRF24_NODE_HPP_

#include <unistd.h>
#include <mutex>
#include <vector>
#include <queue>
#include "rf24_config.hpp"
#include "irq.hpp"
#include "rf24_util.hpp"

// Config structures
typedef struct RF24_conf {
  uint8_t PALevel;
  uint8_t EnableLna;
  uint8_t DataRate;
  uint8_t Channel;
  uint8_t CRCLength;
  uint8_t retriesDelay;
  uint8_t retriesCount;
  uint8_t PayloadSize;
  uint8_t AddressWidth;
  bool    AutoAck;
  // Extended options
  uint32_t   TxDelay;
  useconds_t PollBaseTime;
  int16_t    irq;
  bool       AutoFailureRecovery;

} RF24_conf_t;

typedef struct RF24_stats {
  uint32_t rcv;
  uint32_t snd;
  uint32_t sndError;
} RF24_stats_t;

// Operator
std::ostream& operator<<(std::ostream& out, RF24_conf_t &h); // Prototype


class nRF24 : public Nan::ObjectWrap {
 public:
   // Reader nested class
   class ReaderWorker : public RF24AsyncWorker {
    public:
     ReaderWorker(
         const char *name,
         Nan::Callback *progress
       , Nan::Callback *callback
       , nRF24& dev,useconds_t poll_base=RF24_DEFAULT_POLLTIME)
       : RF24AsyncWorker(callback,name), progress(progress),device(dev)
       , want_stop(false),stopped_(true), want_towrite(0), error_count(0)
       , poll_timeus(poll_base) {
          dev.worker_=this;
        }
     ~ReaderWorker() { device.worker_=NULL; if(progress) delete progress; }

     // Main loop for pooling the reading
     void Execute(const RF24AsyncWorker::ExecutionProgress& progress);
     void HandleProgressCallback(const RF24AsyncType *data, size_t size);
     void HandleOKCallback();

     void stop();
     void  want_write();
     void  no_want_write();
     bool inline  stopped() { return stopped_; }

   private:
    std::mutex wantwrite_mutex;
    static std::mutex one_reader_mutex; 
    Nan::Callback *progress;
    nRF24 &device;
    volatile bool  want_stop,stopped_;
    volatile int   want_towrite;
    int            error_count;
    useconds_t     poll_timeus;
    RF24AsyncType_t tmp_msg[5];   // Temp buffers for callbacks

   };
   // Writer class
   class WriterWorker : public RF24AsyncWriterWorker {
    public:
      WriterWorker(const char *name, Nan::Callback *callback_, nRF24 &dev,
                   const char *buff, size_t buff_size) :
                   RF24AsyncWriterWorker(callback_,name),device(dev),
                   buffer_size(buff_size),
                   tx_requested(0),tx_ok(0),tx_bytes(0),pck_size(0),
                   aborted_(false), started_(false), finished_(false)
      {
          buffer=new char[buff_size];
          if(buffer!=NULL) memcpy(buffer,buff,buff_size);
      }

      ~WriterWorker() { if(buffer) delete [] buffer;}

      void Execute();
      void HandleOKCallback();

      inline void abort() { aborted_=true; }
      inline bool started() {return started_; }
      inline bool finished() { return finished_; }
    private:
      nRF24 &device;
      char  *buffer;
      size_t buffer_size;
      uint32_t tx_requested;
      uint32_t tx_ok;
      uint32_t tx_bytes;
      uint8_t  pck_size;
      volatile bool aborted_,started_,finished_;
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
      Nan::SetPrototypeMethod(tpl, "stopRead",stop_read);
      Nan::SetPrototypeMethod(tpl, "write",write);
      Nan::SetPrototypeMethod(tpl, "stream",stream);
      Nan::SetPrototypeMethod(tpl, "stopWrite",stop_write);
      Nan::SetPrototypeMethod(tpl, "useWritePipe",useWritePipe);
      Nan::SetPrototypeMethod(tpl, "changeWritePipe",changeWritePipe);
      Nan::SetPrototypeMethod(tpl, "addReadPipe",addReadPipe);
      Nan::SetPrototypeMethod(tpl, "changeReadPipe",changeReadPipe);
      Nan::SetPrototypeMethod(tpl, "removeReadPipe",removeReadPipe);
      Nan::SetPrototypeMethod(tpl, "hasFailure",hasFailure);
      Nan::SetPrototypeMethod(tpl, "restart",restart);
      Nan::SetPrototypeMethod(tpl, "present",present);
      Nan::SetPrototypeMethod(tpl, "isP",isP);
      Nan::SetPrototypeMethod(tpl, "powerUp",powerUp);
      Nan::SetPrototypeMethod(tpl, "powerDown",powerDown);
      Nan::SetPrototypeMethod(tpl, "destroy",destroy_object);
      Nan::SetPrototypeMethod(tpl, "getStats",getStats);
      Nan::SetPrototypeMethod(tpl, "resetStats",getStats);
      Nan::SetPrototypeMethod(tpl, "SleepUs",doSleepUs);

      // Set up class
      constructor().Reset(Nan::GetFunction(tpl).ToLocalChecked());
      Nan::Set(target, Nan::New("nRF24").ToLocalChecked(),
      Nan::GetFunction(tpl).ToLocalChecked());
      // Configure constants
      NANCONSTI("RF24_1MBPS",RF24_1MBPS);
      NANCONSTI("RF24_2MBPS", RF24_2MBPS);
      NANCONSTI("RF24_250KBPS",RF24_250KBPS);
      NANCONSTI("RF24_PA_MIN",RF24_PA_MIN);
      NANCONSTI("RF24_PA_LOW",RF24_PA_LOW);
      NANCONSTI("RF24_PA_HIGH",RF24_PA_HIGH);
      NANCONSTI("RF24_PA_MAX",RF24_PA_MAX);
      NANCONSTI("RF24_PA_ULTRA",RF24_PA_MAX+1);
      NANCONSTI("RF24_CRC_DISABLED",RF24_CRC_DISABLED);
      NANCONSTI("RF24_CRC_8",RF24_CRC_8);
      NANCONSTI("RF24_CRC_16",RF24_CRC_16);
      NANCONSTI("RF24_MAX_MERGE",RF24_MAX_MERGEFRAMES);
      NANCONSTI("RF24_MIN_POLLTIME",RF24_MIN_POLLTIME);
      NANCONSTI("RF24_FAILURE_STAT",7);
  }

  bool _begin(bool print_details=false); // Start the radio
  void _config(bool print_details=false); // Configure the readio
  RF24_conf_t *_get_config();      // Init or get current config
  bool _available(uint8_t *pipe=NULL); // Check if data is avaialble
  bool _read(void *data,size_t r_length); // read data.
  void _stop_read();  // stop read
  int  _read_buffered(std::set<uint8_t> &pending,bool &more_available);
  int  _write(void *data,size_t r_length,size_t n_packets, size_t p_size);    // write data & stream
  void _stop_write();
  bool _useWritePipe(uint8_t *pipe_name,bool auto_ack=true);
  bool _changeWritePipe(bool auto_ack,uint16_t mm);
  int32_t _addReadPipe(uint8_t *pipe_name,bool auto_ack=true);
  void    _removeReadPipe(int32_t number);
  bool _hasFailure();
  void _restart();
  bool _changeReadPipe(int32_t number,bool auto_ack,uint16_t maxmerge);
  bool _powerUp();
  bool _powerDown();
  bool _listen();
  bool _transmit();
  bool _present();
  bool _isP();
  void _resetState();
  void _cleanBuffers(const std::set<uint8_t> *pipes=NULL);
  void _copyBuffers(const std::set<uint8_t>*pipes,std::vector<uint8_t> *to);
  void _resetStats(uint8_t pipe=7);
  int  _waitIrq(int32_t timeout_ms,bool clear=false);


  // uitlity one-liners
  inline const char* _get_raw_buffer(int i) { return (const char *)(&(read_buffer_[i][0]));}
  inline size_t      _get_raw_buffer_size(int i) { return read_buffer_[i].size(); }
  inline bool        _has_config() {return current_config!=NULL ; }
  inline RF24*       _get_radio() {return radio_; }
  inline void        _get_cecs(int& ce,int& cs) { ce=ce_; cs=cs_; }
  inline int         _get_irq_num() { return  _get_config()->irq; }
  inline useconds_t  _get_polltime() { return _get_config()->PollBaseTime; }
  inline void        _enable(bool e=true) { is_enabled_ = e; } // enable disable.
  inline bool        _use_irq() { return irq_!=NULL; }
  inline bool        _is_listening() { return is_listening_; }



 private:
  explicit nRF24(int ce,int cs,int spi_speed=RF24_SPI_SPEED);
  // Destructor freeResources
  ~nRF24();
  // Instance Creation
  static NAN_METHOD(New);

  // Constructor helper;
  static inline Nan::Persistent<v8::Function> & constructor() {
    static Nan::Persistent<v8::Function> my_constructor;
    return my_constructor;
  }

  // inteface methods
  static NAN_METHOD(begin);
  static NAN_METHOD(destroy_object);
  static NAN_METHOD(config);
  static NAN_METHOD(read);
  static NAN_METHOD(write);
  static NAN_METHOD(stream);
  static NAN_METHOD(useWritePipe);
  static NAN_METHOD(changeWritePipe);
  static NAN_METHOD(addReadPipe);
  static NAN_METHOD(removeReadPipe);
  static NAN_METHOD(changeReadPipe);
  static NAN_METHOD(hasFailure);
  static NAN_METHOD(restart);
  static NAN_METHOD(getStats);
  static NAN_METHOD(resetStats);

  // One liners
  static NAN_METHOD(stop_read) { MTHIS(nRF24)->_stop_read(); }
  static NAN_METHOD(stop_write) { MTHIS(nRF24)->_stop_write(); }
  static NAN_METHOD(present) { MRET(MTHIS(nRF24)->_present()); }
  static NAN_METHOD(isP) { MRET(MTHIS(nRF24)->_isP()); }
  static NAN_METHOD(powerDown) { MRET(MTHIS(nRF24)->_powerDown()); }
  static NAN_METHOD(powerUp) { MRET(MTHIS(nRF24)->_powerUp()); }
  static NAN_METHOD(doSleepUs) {
    useconds_t us;
    if(Nan::Check(info).ArgumentsCount(1).Argument(0).Bind(us)) {
      std::cout << "SLEEP:"<< us << std::endl;
      sleep_us(us);
    }
  }

  struct RF24_pipe_configuration_t {
     bool       in_use;
     uint8_t    addr[5];
     bool       ackmode;
     uint16_t   stream_info;     // maxtream for write pipe , max merge for reading pipe
  };

  // Class attributtes
  int ce_,cs_,spi_speed_;   // CE, CS
  RF24Irq *irq_;          ///<Irq object
  std::mutex radio_mutex,radio_write_mutex,write_abort_mutex,write_queue_mutex; // Coordination mutex
  RF24 *radio_;   // RADIO
  nRF24::ReaderWorker *worker_;  // READER WORKER
  RF24_conf_t *current_config;  // Curent config
  volatile bool is_powered_up_; // Control flags
  volatile bool is_listening_;
  volatile bool is_enabled_;
  RF24_pipe_configuration_t pipe_conf_[6];  // status of pipes configuration 
  RF24_stats_t  stats_[6];           // Stats for pipes
  uint32_t      failure_stat_;        // Failure counter
  std::vector<uint8_t> read_buffer_[6]; // Reading buffer for every pipe
  std::queue<nRF24::WriterWorker *>  write_queue_; // Writing queue workers

  static uint32_t  _sequencer;

  bool addWriterWorker(Nan::Callback *cb,const char *buff,size_t size);
  void removeWriterWorker();

  friend class nRF24::ReaderWorker;
  friend class nRF24::WriterWorker;
};

#endif

