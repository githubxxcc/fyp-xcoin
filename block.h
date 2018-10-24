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
            int nonce_;

            /*  Data  */
            string miner_;

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

            Block(Block& par, int nonce) :nonce_(nonce) {
                /* Copy info from the parent */
                index_ = par.index_+1;
                prev_hash_ = par.get_hash();
                nbit_ = par.nbit_;
            }

            Block(BlockIndex*, int, string&);


            void set_null() {
                index_ = 0;
                prev_hash_= string();
                nbit_ = 5;
                nonce_ = 0;

                miner_ = string();
            }

            string serialize() {
                assert(nonce_ < 100);
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
            bool process_block();
            static Block genesis();

            string to_string() {
                stringstream ss;
                ss  << "Index : " << index_  << " | " 
                    << "Nonce : " << nonce_  << " | " 
                    << "Hash : " << prev_hash_ << " | "
                    << "Miner : " << miner_ ;

                return ss.str();
            }


            friend ostream& operator<<(ostream &strm, const Block&);

            friend class boost::serialization::access;
            template<class Archive>
            void serialize(Archive & ar, const unsigned int version) 
            {
                ar  & index_;
                ar  & prev_hash_;
                ar  & nbit_;
                ar  & nonce_;
                ar  & miner_;
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
            int nonce_;
            string  hash_;

            string miner_;

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
                miner_ = block.miner_;
            }

            string to_string() const {
                stringstream ss;
                ss  << "n_height : " << n_height_ << "| " 
                    <<  "hash : " << hash_ << " | " 
                    << " miner: " << miner_ ;
                return ss.str();
            }

            string to_short_string() const {
                char buf[512];
                sprintf(buf, "[h:%d - hash:%s - miner:%s]", 
                        n_height_,  hash_.substr(0, 6).c_str(), miner_.c_str());
                return string(buf);
            }

            int get_height() const { return n_height_;}

            static BlockIndex* get_best_blkidx() ;
    };


    void debug_chains() ;
    void debug_all_chains() ;
    int best_blk_height();

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
