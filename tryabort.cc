#include <thread>
#include <csetjmp>
#include <csignal>
#include <cstdlib>
#include <stdio.h>
#include <string.h>
#include "tryabort.hpp"

static jmp_buf env;
static char point[512];
static std::function<void ()> on_abort_func=NullFunction;

#ifdef NRF24_DEBUG
void set_debug_abort(const char *txt){
  strcpy(point,txt);
}
#endif

void on_abort(std::function<void ()> func){
  on_abort_func=func;
}

void _on_sigabrt (int signum)
{
  std::function<void ()> f=on_abort_func;
  on_abort(); // Reset on abort
  //printf("Aborted on:%s thread %p\n",point, std::this_thread::get_id());
  if(f) f();
  longjmp (env, 1);
}
void try_and_catch_abort (std::function<void ()> func)
{
  if (setjmp (env) == 0) {
    signal(SIGABRT, &_on_sigabrt);
    point[0]='\0';
    func();
  }
}
