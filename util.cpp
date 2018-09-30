
#include "util.h"

using namespace std;

namespace xcoin {
    int util_get_block(char* buf, size_t size) 
    {
        stringstream ss;
        int new_block;

        ss << buf;
        ss >> new_block;

        return new_block;
    }


    void util_serialize_block(int b, stringstream &ss) 
    {
        ss << b;
    }
}


