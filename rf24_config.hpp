#ifndef _RF24_NODE_CONFIG_HPP_
#define _RF24_NODE_CONFIG_HPP_

#include <set>
#include <vector>
#include <nan.h>
#include <nan-check.h>
#include <RF24.h>
/*#include <RF24Network.h>
#include <RF24Mesh.h>
#include <RF24Gateway.h>
*/

// config
#define RF24_DEFAULT_POLLTIME   35*1000   // 35ms default poll wait for reading in us
#define RF24_MIN_POLLTIME       4*1000    // 4ms as minimum poll time
#define RF24_MAX_ERROR_COUNT    100       // Repetitive error MAX.
#define RF24_IRQ_POLLTIME       1500      // 1500ms for irq polling
#define RF24_MAX_MERGEFRAMES    128      // ABSOLUTE MAX of merge frames
#define RF24_MAX_BUFFREAD_ERR   1500     // Max error in pipes read buffered



// MACRO UTILS
#define MTHIS(T)       (Nan::ObjectWrap::Unwrap<T>(info.Holder()))
#define MRET(x)        (info.GetReturnValue().Set(x))
#define NANCONST(x,v)  (Nan::Set(target,Nan::New(x).ToLocalChecked(),Nan::New(v).ToLocalChecked()))
#define NANCONSTI(x,v) (Nan::Set(target,Nan::New(x).ToLocalChecked(),Nan::New(v)))

// Old Nan version 2.7.0
//typedef Nan::AsyncProgressWorker RF24AsyncWorker;
// For Nan version > 2.8.0
//typedef Nan::AsyncProgressQueueWorker<char> RF24AsyncWorker;



struct RF24AsyncType_t {
  uint32_t pipe;
  size_t   size;
  char     buffer[(32*RF24_MAX_MERGEFRAMES)+1];
};

//typedef char RF24AsyncType;
typedef RF24AsyncType_t RF24AsyncType;
typedef char              RF24MeshAsyncType;
typedef Nan::AsyncProgressQueueWorker<RF24AsyncType> RF24AsyncWorker;
typedef Nan::AsyncProgressQueueWorker<RF24MeshAsyncType> RF24MeshAsyncWorker;
typedef Nan::AsyncWorker RF24AsyncWriterWorker;

#endif
