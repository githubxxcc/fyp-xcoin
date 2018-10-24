#ifndef XCOIN_NET_H_
#define XCOIN_NET_H_

#include <sstream>
#include <string>

namespace xcoin {


    class MsgHeader {
        public:
            int type;
            int size;
        
        MsgHeader(string & s) {
            stringstream ss(s);
            ss >> type >> size;
        }
        
        string to_string() {
            stingstream ss;
            ss << type << " " << size;
            return ss.str();
        }

        
    };

}


#endif
