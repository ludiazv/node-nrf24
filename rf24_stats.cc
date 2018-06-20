#include "rf24.hpp"
#include "tryabort.hpp"

NAN_METHOD(nRF24::getStats) {
  auto THIS=MTHIS(nRF24);
  std::lock_guard<std::mutex> guard(THIS->radio_mutex);

  if(info.Length()>=1 && info[0]->IsUint32()){
      auto pipe=info[0]->Uint32Value();
      if(pipe==0){
        auto o=Nan::New<v8::Object>();
        Nan::Set(o,Nan::New("TotalTx_Ok").ToLocalChecked(),Nan::New(THIS->stats_[0].snd));
        Nan::Set(o,Nan::New("TotalTx_Err").ToLocalChecked(),Nan::New(THIS->stats_[0].sndError));
        MRET(o);
        return;
      } else if(pipe >0 && pipe <=5)
          {
            MRET(THIS->stats_[pipe].rcv); // Return readed
            return;
          }
  }
  // Return all pipes
  auto o=Nan::New<v8::Object>();
  auto a=Nan::New<v8::Array>(5);
  Nan::Set(a,0,Nan::New(0));
  int rx=0;
  for(int i=1;i<6;i++) {
      Nan::Set(a,i,Nan::New(THIS->stats_[i].rcv));
      rx+=THIS->stats_[i].rcv;
  }
  Nan::Set(o,Nan::New("TotalRx").ToLocalChecked(),Nan::New(rx));
  Nan::Set(o,Nan::New("TotalTx_Ok").ToLocalChecked(),Nan::New(THIS->stats_[0].snd));
  Nan::Set(o,Nan::New("TotalTx_Err").ToLocalChecked(),Nan::New(THIS->stats_[0].sndError));
  Nan::Set(o,Nan::New("PipesRx").ToLocalChecked(),a);
  MRET(o);

}

NAN_METHOD(nRF24::resetStats) {
  auto THIS=MTHIS(nRF24);
  std::lock_guard<std::mutex> guard(THIS->radio_mutex);
  if(info.Length()>=1 && info[0]->IsUint32()) {
    auto pipe=info[0]->Uint32Value();
    if(pipe>=0 && pipe<=5) memset(&THIS->stats_[pipe],0,sizeof(RF24_stats_t));
  } else memset(&THIS->stats_[0],0,sizeof(RF24_stats_t)*6); // Clear all

}
