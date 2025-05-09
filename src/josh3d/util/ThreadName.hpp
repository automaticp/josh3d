#pragma once
#include <cstring>
#ifdef __gnu_linux__
#include <pthread.h>
#endif


namespace josh {


/*
TODO: Really dirty implementation for now. Linux only.
*/
inline void set_current_thread_name(const char* name_hint) {
#ifdef __gnu_linux__
    char name[16]{}; // NOTE: Limited to 16 characters.
    std::strncpy(name, name_hint, 15);
    pthread_setname_np(pthread_self(), name);
#endif
}


} // namespace josh
