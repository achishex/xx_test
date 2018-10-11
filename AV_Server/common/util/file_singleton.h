#ifndef _FILE_SINGLETON_H_
#define _FILE_SINGLETON_H_

#include <string>

namespace avs_util
{

int singleton(const std::string& lock_file);

}

#endif
