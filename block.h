#ifndef XCOIN_BLOCK_H_
#define XCOIN_BLOCK_H_


#include <string>
#include <ostream>
#include <vector>

using namespace std;
namespace xcoin
{
    class BlockIndex;
    class Block;



    class Block {
        public:
            /*  Header  */
            uint32_t index_;
            string prev_hash_;
            uint32_t nbit_;
            uint32_t nonce_;

            //string data_;

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

            Block(BlockIndex* blk, uint32_t);

            string get_hash() const;
            bool check_block() const;
            bool accept_block();
            bool add_to_chain();

            friend ostream& operator<<(ostream &strm, const Block&);
    };

    class BlockIndex {
        public: 
            const string* phash_;
            BlockIndex * pprev_;
            BlockIndex * pnext_;
            unsigned int block_pos_;
            int n_height_;

            /*  Block Header Data  */ //Why needed?
            uint32_t nbit_;
            uint32_t nonce_;
            string  hash_;

            //BlockIndex() 
            //{
            //    phash_ = NULL;
            //    pprev_ = NULL;
            //    pnext_ = NULL;
            //    block_pos_ = 0;
            //    n_height_ = 0;

            //    nbit_ = 0;
            //    nonce_ = 0;
            //}

            BlockIndex(Block block) 
            {
                phash_ = NULL;
                pprev_ = NULL;
                pnext_ = NULL;
                block_pos_ = 0;
                n_height_ = 0;

                nbit_ = block.nbit_;
                nonce_ = block.nonce_;
                hash_ = block.get_hash();
            }

            static BlockIndex* get_best_blkidx() ;
    };

   // class Chain {
   //     public:
   //         vector<Block> blocks_;
   //         
   //         Chain() : blocks_() {
   //             add_genesis();
   //         }

   //         const Block& get_top_block() const;
   //         void add_genesis();
   //         
   //         /*  Merge another chain into it */
   //         void merge_chain(Chain&);               


   // };
}


#endif
