#pragma once
// IWYU pragma: always_keep
#include <boost/predef/os.h>


/*
HMM: Not sure how I feel about this being in util/ but, since
pretty much everything depends on util/, it's fine for now.
*/

#if BOOST_OS_LINUX
#define JOSH3D_OS_LINUX
#elif BOOST_OS_WINDOWS
#define JOSH3D_OS_WINDOWS
#else
#define JOSH3D_OS_UNKNOWN
#endif
