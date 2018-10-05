#ifndef XCOIN_BLOCK_H_
#define XCOIN_BLOCK_H_

#include <string>
#include <cstdint>

namespace xcoin 
{
    using namespace std;
    class Block;

    class Block 
    {
        public: 
            string hash_;
            string data_ ;

            /*  Header section  */
            string prev_hash_;
            int index_;
            uint32_t nonce_;
            uint32_t n_bits_;

            string get_hash() const;
    };

}

#endif
