#ifndef _NRF24_TRYABORT_H
#define _NRF24_TRYABORT_H

#include<functional>

// Prototype
static std::function<void ()> NullFunction;
void try_and_catch_abort (std::function<void ()>);
void on_abort(std::function<void ()> func=NullFunction);
#ifdef NRF24_DEBUG
void set_debug_abort(const char *txt);
#endif

#endif
