#include "file_singleton.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/file.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include <iostream>
#include <string>

namespace avs_util
{
//
int singleton(const std::string& lock_file)
{
    int fd = ::open(lock_file.c_str(), O_WRONLY|O_CREAT, S_IRWXU);
    if ( -1 == fd  )
    {   
        std::cout << "open lock file: [ " << lock_file << "  ] fail! err msg: "
            << strerror(errno) << std::endl;
        return -1;
    }

    int rv = flock(fd, LOCK_EX|LOCK_NB);
    if (0 != rv)
    {
        std::cout << "the process is running!" << std::endl;
        ::close(fd);
        return -1;
    }
    return 0;
}

//
}
