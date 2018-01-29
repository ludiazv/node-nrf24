#ifndef _NRF24_TRYABORT_H
#define _NRF24_TRYABORT_H

#include<functional>

// Prototype
static std::function<void ()> NullFunction;
void try_and_catch_abort (std::function<void ()>);
void set_debug_abort(const char *txt);
void on_abort(std::function<void ()> func=NullFunction);

#endif
