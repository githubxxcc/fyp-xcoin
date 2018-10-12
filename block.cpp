
#include "block.h"
#include "util.h"

#include <unordered_map>
#include <iostream>
#include <assert.h>


namespace xcoin 
{
    using namespace std;
    /* *************************
     * GLobal State
     * */
    unordered_map<string, BlockIndex*> g_block_index;
    const string g_genesis_hash("000000000019d6689c085ae165831e934ff763ae46a2a6c172b3f1b60a8ce26f");
    BlockIndex* g_genesis_indexblk = NULL;
    int g_best_height = -1;
    string g_best_chain_hash = string("DEADBEEF");
    BlockIndex* g_best_index = NULL;
    

    /* ************************ 
     * Block
     *  */
    Block::Block(BlockIndex* blk_idx, uint32_t nonce) : nonce_(nonce) 
    {
        index_ = blk_idx->n_height_ + 1;
        prev_hash_ = blk_idx->hash_;
        nbit_ = blk_idx->nbit_;
    }


    string
    Block::get_hash() const 
    {
        stringstream ss;
        ss << prev_hash_
            << index_
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

    bool Block::accept_block() 
    {
        string hash = this->get_hash();
        if(g_block_index.count(hash)) {
            cerr << "accept_block(): already in " << endl;
            return false;
        }

        auto itr = g_block_index.find(hash);
        assert(itr != g_block_index.end());

        BlockIndex* index_prev = (*itr).second;

        //Check pow and timestamp
        

        /* Add to the Block index */
        if(!this->add_to_chain()) {
            cerr << "accept_block(): add_to_chain failed" << endl;
            return false;
        }

        return true;
    }

    bool Block::check_block() const 
    {
        //FIXME : assuming the block is valid all the time
        return true;
    }

    bool Block::add_to_chain() 
    {
        string hash = this->get_hash();
        if(g_block_index.count(hash)) {
            cerr << "add_to_chain(): " << hash << "exists";
            return false;
        }
        
        //Create the new block index and add to the global map
        BlockIndex * index_new = new BlockIndex(*this);
        auto itr = g_block_index.insert(make_pair(hash, index_new)).first;
        index_new->phash_ = &((*itr).first);
        
        //Configure the new block index
        auto prev_itr = g_block_index.find(this->prev_hash_);
        if(prev_itr != g_block_index.end()) {
            index_new->pprev_ = (*prev_itr).second;
            index_new->n_height_ = index_new->pprev_->n_height_+1;
        }

        //If new head node 
        if(index_new->n_height_ > g_best_height) {
            //Case 1: genesis block
            if(g_genesis_indexblk == NULL && hash == g_genesis_hash){
                g_genesis_indexblk = index_new;
            } 
            //Case 2: adding to current best branch 
            else if (this->prev_hash_ == g_best_chain_hash) {
                index_new->pprev_->pnext_  = index_new;
            }
            //Case 3: new best branch
            else {
                //do nothing
            }


            g_best_chain_hash = hash;
            g_best_index = index_new;
            g_best_height = index_new->n_height_;
        }
        
        return true;
    }


    Block Block::genesis() 
    {
        Block genesis;
        return genesis;
    }

    /* ****************************
     * BlockIndex
     * */

    BlockIndex*
    BlockIndex::get_best_blkidx()
    {
        return g_best_index;
    }


}
