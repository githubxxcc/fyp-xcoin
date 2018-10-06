#ifndef XCOIN_UTIL_H_
#define XCOIN_UTIL_H_

#include <sstream>
#include <string>

using namespace std;
namespace xcoin
{
    int util_get_block(char*, size_t);
    void util_serialize_block(int, stringstream &);


    /*  Crypto  */
    string sha1(string &s);
}


#endif
