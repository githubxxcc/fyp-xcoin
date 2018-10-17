#ifndef XCOIN_BLOCK_H_
#define XCOIN_BLOCK_H_


#include <string>
#include <sstream>
#include <ostream>
#include <vector>

#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>


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
            
            Block() {
                set_null();
            }

            Block(uint32_t index, string& prev_hash, uint32_t nbit):
                index_(index),
                prev_hash_(prev_hash),
                nbit_(nbit) {
                    this->nonce_ = 0;
                }

            Block(Block& par, uint32_t nonce) :nonce_(nonce) {
                /* Copy info from the parent */
                index_ = par.index_+1;
                prev_hash_ = par.get_hash();
                nbit_ = par.nbit_;
            }

            Block(BlockIndex* blk, uint32_t);

            void set_null() {
                index_ = 0;
                prev_hash_= string("NULL");
                nbit_ = 5;
                nonce_ = 0;
            }

            string serialize() {
                stringstream ss;
                boost::archive::text_oarchive oa{ss};
                oa  << *this;
                return ss.str();
            }

            static Block deserialize(stringstream &ss) {
                Block block;
                boost::archive::text_iarchive ia{ss};
                ia >> block;
                return block;
            }

            string get_hash() const;
            bool check_block() const;
            bool accept_block();
            bool add_to_chain();
            static Block genesis();

            friend ostream& operator<<(ostream &strm, const Block&);

            friend class boost::serialization::access;
            template<class Archive>
            void serialize(Archive & ar, const unsigned int version) 
            {
                ar  & index_
                    & prev_hash_
                    & nbit_
                    & nonce_;
            }

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

            string to_string() const {
                stringstream ss;
                ss  << "n_height : " << n_height_ << "| " 
                    <<  "hash : " << hash_ << " | ";
                return ss.str();
            }

            static BlockIndex* get_best_blkidx() ;
    };


    void debug_chains();

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
