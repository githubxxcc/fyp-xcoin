#ifndef XCOIN_UTIL_H_
#define XCOIN_UTIL_H_

#include <sstream>
#include <string>

#include "cpptoml.h"

using namespace std;
namespace xcoin
{
    class Config;
    class PeerAddr;


    int util_get_block(char*, size_t);
    void util_serialize_block(int, stringstream &);


    /*  Crypto  */
    string sha1(string &s);

    Config parse_config(string&);

    class PeerAddr {
        public:
            string host;
            int port;
    };

    class Config {
        public:
            vector<PeerAddr> peers;
            int my_port;
            int miner_timeout;

        void print() {
            printf("----------System Config-------\n");
            printf(" [Peers]:\n");
            for(const auto& peer : peers) {
                printf("Host : %s  - Port %d \n", peer.host.c_str(), peer.port);
            }

            printf("My Port :       %d\n", my_port);
            printf("Miner Timeout : %d\n", miner_timeout);
        }
    };



}


#endif
