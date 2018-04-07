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
    return (v->IsUint32()) ? v->Uint32Value() : 0;
}
bool ObjGetBool(v8::Local<v8::Object> & obj,const std::string & name) {
  auto v=ObjGet(obj,name);
  return (v->IsBoolean()) ? v->BooleanValue() : true;
}

bool ConvertHexAddress(v8::Local<v8::String> val,uint8_t *converted,uint8_t size) {
  bool res=false;
  v8::String::Utf8Value value(val);
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
