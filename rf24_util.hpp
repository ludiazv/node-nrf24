#ifndef _NRF24_NODE_UTIL_HPP_
#define _NRF24_NODE_UTIL_HPP_
#include<nan.h>
// For debug
#ifdef NRF24_DEBUG
#include<iostream>
#endif

bool ObjHas(v8::Local<v8::Object> & obj,const std::string & name);
v8::Local<v8::Value> ObjGet(v8::Local<v8::Object> & obj,const std::string & name);
uint32_t ObjGetUInt(v8::Local<v8::Object> & obj,const std::string & name);
bool ObjGetBool(v8::Local<v8::Object> & obj,const std::string & name);

bool ConvertHexAddress(v8::Local<v8::String> val,uint8_t *converted,uint8_t size);

#ifdef NRF24_DEBUG
#define NRF24DBG(x) (x)
#else
#define NRF24DBG(x)
#endif


#endif
