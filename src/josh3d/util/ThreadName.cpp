#include "BuildConfig.hpp"
#include <cstring>
#ifdef JOSH3D_OS_LINUX
#include <pthread.h>
#endif


namespace josh {


void set_current_thread_name(const char* name_hint)
{
#ifdef JOSH3D_OS_LINUX
    char name[16] = {}; // NOTE: Limited to 16 characters.
    std::strncpy(name, name_hint, 15);
    pthread_setname_np(pthread_self(), name);
#endif
}


} // namespace josh
