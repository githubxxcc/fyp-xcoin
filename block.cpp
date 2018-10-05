

#include "block.h"
#include <sstream>

namespace xcoin 
{
    using namespace std;
    /*  Serialize a block header without the nonce */
    string Block::get_hash() const 
    {
        stringstream ss;

        ss << this->prev_hash_ 
            << this-> index_
            << this->n_bits_;

        return ss.str();
    }
}
