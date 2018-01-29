
#include "rf24.hpp"
#include "rf24mesh.hpp"

NAN_MODULE_INIT(InitAll) {
  nRF24::Init(target);
  nRF24Mesh::Init(target);
}

NODE_MODULE(NODE_GYP_MODULE_NAME, InitAll)
