#ifndef _NRF24_NODE_UTIL_HPP_
#define _NRF24_NODE_UTIL_HPP_

#include<nan.h>
#include<time.h>

// For debug
#ifdef NRF24_DEBUG
#include<iostream>
#endif

bool ObjHas(v8::Local<v8::Object> & obj,const std::string & name);
v8::Local<v8::Value> ObjGet(v8::Local<v8::Object> & obj,const std::string & name);
uint32_t ObjGetUInt(v8::Local<v8::Object> & obj,const std::string & name);
int32_t ObjGetInt(v8::Local<v8::Object> & obj,const std::string & name);
bool ObjGetBool(v8::Local<v8::Object> & obj,const std::string & name);

bool ConvertHexAddress(v8::Local<v8::String> val,uint8_t *converted,uint8_t size);

/* Time functions */
void  UsToTimeSpec(useconds_t us,struct timespec *tv);
void  sleep_us(useconds_t us);
inline void  sleep_ts(struct timespec *tv) { clock_nanosleep(CLOCK_MONOTONIC, 0, tv, NULL); }

inline void m_start(struct timespec *tv) { clock_gettime(CLOCK_MONOTONIC_RAW, tv); }
useconds_t m_end(struct timespec *start,bool millis=false);

#ifdef NRF24_DEBUG
#define NRF24DBG(x) (x)
#else
#define NRF24DBG(x)
#endif


#endif
