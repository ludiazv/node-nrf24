#ifndef _RF24_NODE_IRQ_HPP_

#include "rf24_config.hpp"
#include <poll.h>

class RF24Irq {
  public:
  RF24Irq(uint32_t pin);
  ~RF24Irq();

  bool    begin(const char* mode,const char* edge);
  bool    configure(const char* mode,const char *edge);
  void    clear();
  int     wait(bool clear=true,uint32_t timeout=200);
  void    stop();
  inline  uint8_t get() { return val; }

  static void un_export(uint32_t p);
  static const char *DIR_INPUT;
  static const char *DIR_OUTPUT;
  static const char *EDGE_BOTH;
  static const char *EDGE_RISING;
  static const char *EDGE_FALLING;
  static const char *EDGE_NONE;

  private:
    uint32_t pin;
    int fd , efd;
    uint8_t val;
    struct pollfd pfd[2];
    void unexport();

};


#endif
