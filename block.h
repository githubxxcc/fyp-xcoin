#ifndef XCOIN_BLOCK_H_
#define XCOIN_BLOCK_H_


#include <string>
#include <ostream>
#include <vector>

using namespace std;
namespace xcoin
{
    class Block;
    class Chain;



    class Block {
        public:
            /*  Header  */
            uint32_t index_;
            string prev_hash_;
            uint32_t nbit_;
            uint32_t nonce_;

            string data_;

            Block(uint32_t index, string& prev_hash, uint32_t nbit):
                index_(index),
                prev_hash_(prev_hash),
                nbit_(nbit) {
                    this->nonce_ = 0;
                }

            Block(uint32_t index, uint32_t nbit):
                index_(index),
                nbit_(nbit) {
                    this->nonce_ = 0;
                    this->prev_hash_ = "000000000019d6689c085ae165831e934ff763ae46a2a6c172b3f1b60a8ce26f";
                }

            Block(Block& par, uint32_t nonce) :nonce_(nonce) {
                /* Copy info from the parent */
                index_ = par.index_+1;
                prev_hash_ = par.get_hash();
                nbit_ = par.nbit_;
            }

            string get_hash() const;

            friend ostream& operator<<(ostream &strm, const Block&);
    };


    class Chain {
        public:
            vector<Block> blocks_;
            
            Chain() : blocks_() {
                add_genesis();
            }

            const Block& get_top_block() const;
            void add_genesis();

    };
}


#endif
