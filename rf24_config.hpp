#ifndef _RF24_NODE_CONFIG_HPP_
#define _RF24_NODE_CONFIG_HPP_

#include <nan.h>
#include <nan-check.h>
#include <RF24.h>
#include <RF24Network.h>
#include <RF24Mesh.h>
#include <RF24Gateway.h>


// config
#define RF24_DEFAULT_POLLTIME  50*1000   // 50ms default poll wait for reading
#define RF24_MAX_ERROR_COUNT   100

// MACRO UTILS
#define MTHIS(T)       (Nan::ObjectWrap::Unwrap<T>(info.Holder()))
#define MRET(x)        (info.GetReturnValue().Set(x))
#define NANCONST(x,v)  (Nan::Set(target,Nan::New(x).ToLocalChecked(),Nan::New(v).ToLocalChecked()))
#define NANCONSTI(x,v) (Nan::Set(target,Nan::New(x).ToLocalChecked(),Nan::New(v)))


#endif
