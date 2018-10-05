
#include "util.h"

#include <openssl/sha.h>

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


    string sha1(const string& s, size_t n) 
    {
        unsigned char buf[SHA_DIGEST_LENGTH];

        SHA1(reinterpret_cast<const unsigned char*>(s.c_str()), s.length(), buf);

        return string(reinterpret_cast<char*>(buf));
    }
}


