
#include "block.h"
#include "util.h"

namespace xcoin 
{
    string
    Block::get_hash() const 
    {
        stringstream ss;
        ss << index_ 
            << prev_hash_
            << nbit_
            << nonce_;
        auto temp = ss.str();
        return sha1(temp);
    }

    ostream& operator<<(ostream& strm, const Block &b) 
    {
        return strm << "index: " << b.index_ << "\t"
            << "prev_hash: " << b.prev_hash_ << "\t" 
            << "nbit: " << b.nbit_ <<  "\t"
            << "nonce : " << b.nonce_ << endl;
    }


    
    const Block&
    Chain::get_top_block() const
    {
        return this->blocks_.back();
    }

    void
    Chain::add_genesis() 
    {
        Block genesis(0, 5);
        this->blocks_.push_back(genesis);
    }
}
