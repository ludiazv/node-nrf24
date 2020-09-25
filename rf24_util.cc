#include<stdlib.h>
#include "rf24_util.hpp"

bool ObjHas(v8::Local<v8::Object> & obj,const std::string & name) {
  auto t=Nan::Has(obj,Nan::New(name).ToLocalChecked());
  return (t.IsJust()) ? t.FromJust() : false;
}

v8::Local<v8::Value> ObjGet(v8::Local<v8::Object> & obj,const std::string & name) {
  return Nan::Get(obj,Nan::New(name).ToLocalChecked()).ToLocalChecked();
}

uint32_t ObjGetUInt(v8::Local<v8::Object> & obj,const std::string & name) {
    auto v=ObjGet(obj,name);
    return (v->IsUint32()) ? v->Uint32Value(Nan::GetCurrentContext()).FromJust() : 0;
}

int32_t ObjGetInt(v8::Local<v8::Object> & obj,const std::string & name) {
    auto v=ObjGet(obj,name);
    return (v->IsInt32()) ? v->Int32Value(Nan::GetCurrentContext()).FromJust() : 0;
}

bool ObjGetBool(v8::Local<v8::Object> & obj,const std::string & name) {
  auto v=ObjGet(obj,name);
  return (v->IsBoolean()) ? Nan::To<bool>(v).FromJust() : true;
}

bool ConvertHexAddress(v8::Local<v8::String> val,uint8_t *converted,uint8_t size) {
  bool res=false;
  //v8::String::Utf8Value value(v8::Isolate::GetCurrent(),val); 
  Nan::Utf8String value(val);
  int l=value.length(),i;
  char tmp[3];
  tmp[2]='\0';

  if(*value) {
    char *org=*value;
    //std::cout << "value->" << org << " size:" << (int)size*2 << " l:" << l-2 << std::endl;
    //Check if well formated 0xAABBCCDD
    if(org[0]=='0' && (org[1]=='x' || org[1]=='X') && (size*2)==(l-2)) {
      for(i=l-1;i>2;i-=2) {
          tmp[0]=org[i-1];tmp[1]=org[i];
          //std::cout << " /converting->" << tmp;
          *converted=(uint8_t)strtol(tmp,NULL,16);
          converted++;
      }
      res=true;
    }
  }
  return res;
}

/* Time functions */
/* Time functions */
void  UsToTimeSpec(useconds_t us,struct timespec *tv){

   tv->tv_sec = us/1000000;
   tv->tv_nsec = (us*1000) % 1000000000;

}

void  sleep_us(useconds_t us){
    struct timespec res;
    UsToTimeSpec(us,&res);
    clock_nanosleep(CLOCK_MONOTONIC_RAW, 0, &res, NULL);
}

useconds_t m_end(struct timespec *start,bool millis) {
  struct timespec fin;
  clock_gettime(CLOCK_MONOTONIC_RAW, &fin);
  useconds_t us=(fin.tv_nsec - start->tv_nsec) / 1000 + (fin.tv_sec  - start->tv_sec)* 1000000;
  return (millis) ? us/1000 : us;
}
