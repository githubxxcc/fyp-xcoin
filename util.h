#ifndef XCOIN_UTIL_H_
#define XCOIN_UTIL_H_

#include <sstream>

namespace xcoin
{
    int util_get_block(char*, size_t);
    void util_serialize_block(int, std::stringstream &);
}


#endif
