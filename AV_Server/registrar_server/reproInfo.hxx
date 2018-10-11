#ifdef WIN32
#define REPRO_BUILD_HOST "localhost" 
#define REPRO_RELEASE_VERSION "1.10.2" 
#else
#ifdef HAVE_CONFIG_H
#include "config.h"
#else
#define REPRO_BUILD_HOST "localhost" 
#define REPRO_RELEASE_VERSION "1.10.2" 
#endif
#endif