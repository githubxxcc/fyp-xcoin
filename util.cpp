
#include "util.h"

#include <openssl/sha.h>
#include <sstream>
#include <iomanip>
#include <iostream>
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

    string sha1(string &s) 
    {
        stringstream ss;
        unsigned char buf[SHA_DIGEST_LENGTH] = {0};
        SHA1(reinterpret_cast<const unsigned char*>(s.c_str()), 
                s.length(), buf);

        //Convert to HEX
        for(int i = 0 ; i < SHA_DIGEST_LENGTH ; i++ ) {
            ss << hex << static_cast<int>(buf[i]);
        }

        return ss.str();
    }

    //log_error(const char* format, ...) 
    //{
    //    char buffer[LOG_BUFFER_SIZE];
    //    va_list arg_ptr;
    //    va_start(arg_ptr, format);
    //    int ret = vsnprintf(buffer, LOG_BUFFER_SIZE, format, arg_ptr);
    //    va_end(arg_ptr);

    //    if(ret < 0 || ret >= LOG_BUFFER_SIZE) {
    //        buffer[limit-1] = 0;
    //    }

    //    printf("ERROR: %s\n", buffer);
    //    return false;
    //}
}


