
#include "block.h"
#include "util.h"

#include <unordered_map>
#include <map>
#include <iostream>
#include <assert.h>

#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"

namespace xcoin 
{
    using namespace std;
    /* *************************
     * GLobal State
     * */
    unordered_map<string, BlockIndex*> g_block_index;
    multimap<string, Block*> g_orphan_blocks;
    multimap<string, Block*> g_orphan_prev_blocks;

    const string g_genesis_hash("e9d9579737f7e206617cd903ce42d986d5a2f");
    BlockIndex* g_genesis_indexblk = NULL;
    int g_best_height = -1;
    string g_best_chain_hash = string("DEADBEEF");
    BlockIndex* g_best_index = NULL;
    
    
    static void connect_orphans(string&);
    /* ************************ 
     * Block
     *  */
    Block::Block(BlockIndex* blk_idx, int nonce, string& name) : nonce_(nonce) 
    {
        index_ = blk_idx->get_height() + 1;
        prev_hash_ = blk_idx->hash_;
        nbit_ = blk_idx->nbit_;
        miner_ = name;
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

        auto itr = g_block_index.find(this->prev_hash_);
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

    bool Block::process_block() 
    {
        auto err = spdlog::get("stderr");
        debug_chains();
        string hash = this->get_hash();

        /*  Check for duplicates */
        if(g_block_index.count(hash)) {
            cerr << "process_block(): already have block " << hash <<"\n" << endl;     
            return false;
        }
        if(g_orphan_blocks.count(hash)) {
            cerr << "process_block(): orphan block \n"<< endl;
            return false;
        }

        /*  General Check on the block */
        if(!this->check_block()) {
            cerr << "process_block(): check block failed \n"<<endl;
            return false;
        }

        /*  If previous block not present => it's an orphan */
        if(!g_block_index.count(this->prev_hash_)) {
            err->info("[Main - process_block] Orphan Block : {}", hash);     
            g_orphan_blocks.insert(make_pair(hash, this));
            g_orphan_prev_blocks.insert(make_pair(this->prev_hash_, this));
            
            //FIXME: ask for the missing blocks?
            return true;
        }

        if(!this->accept_block()) {
            cerr << "process_block(): accept_block failed\n" << endl;
            return false;
        }
    
        /*  Recursively add orphans that depend on this block */
        connect_orphans(hash);
        return true;
    }


    static void
    connect_orphans(string& hash) 
    {
       vector<string> work_q;
       work_q.push_back(hash);

       for (int i = 0; i<work_q.size(); i++) {
            string prev_hash = work_q[i];

            for(auto itr = g_orphan_prev_blocks.lower_bound(prev_hash);
                    itr != g_orphan_prev_blocks.upper_bound(prev_hash);
                    itr++) {
                Block* orphan_blk = (*itr).second;

                /*  Orphan now finds a parent */
                if(orphan_blk->accept_block()) {
                    /*  Some other orphans might depend on it */
                    work_q.push_back(orphan_blk->get_hash());
                }

                /*  Delete the orphan hash even if it is not deleted */
                g_orphan_blocks.erase(orphan_blk->get_hash());

                //FIXME: delete orphan block? 
                //delete orphan_block;
            }

            g_orphan_prev_blocks.erase(prev_hash);
       }

    }

    /* ****************************
     * BlockIndex
     * */

    BlockIndex*
    BlockIndex::get_best_blkidx()
    {
        return g_best_index;
    }
    

    /*  ************************
     *  Utility methods
     * */

    int
    best_blk_height() 
    {
        return g_best_height;
    }

    void
    debug_chains()
    {
        // Print heigh
        cout << "------- Cur Height ------\n";
        cout << g_best_height << "\n";


        // Print best chain 
        assert(g_best_index != NULL);
        cout << "------- Best Chain ------\n";
        BlockIndex* tmp_blk = g_best_index;
        int cnt = 0;        // current row idx
        int row_cnt = 3;    // 5 blocks per row
        while(tmp_blk != NULL) {
            cnt++;
            cout << tmp_blk->to_short_string() << " ---> ";
            if(cnt == row_cnt) {
                cout << "\n";
                cnt = 0;
            }
            tmp_blk = tmp_blk->pprev_;
        }

        cout << "\n";
    }

    void debug_all_chains()
    {
    }


}
