#ifndef XCOIN_MAIN_H_
#define XCOIN_MAIN_H_

#include <string>
#include <unordered_map>
#include <arpa/inet.h>
#include "block.h"
#include "util.h"
#include <boost/program_options.hpp>

namespace xcoin {
    /*  Constants  */ 
    constexpr int MINE_VAR_INTERVAL = 5; /*  Seconds  */

    typedef struct _Server Server;
    class MinerState;
    class Client;


    struct _Server {
        int sd_;
    };

    class Client {
    public:
        int sd_;
        string server_host_name_;
        in_addr_t server_ip_;
        struct bufferevent * buf_ev_;
    };


    class XState {
        public:
        Server server;

        int my_port_;
        string my_addr_;

        /* Outgoing Clients */
        unordered_map<string, Client*> out_clients_;
        
        /*  For incoming client instance. For callback */
        struct event_base * evbase_;
        unordered_map<string, Client*> in_clients_;

        /*  Miner state */
        pthread_t miner_;
        struct event_base * miner_base_;
        MinerState * miner_state_;

        /*  Communication Sockets to Miner */
        struct bufferevent * r_bev_;
        struct bufferevent * w_bev_;

        Client* connect_peer(PeerAddr &);
        void broadcast_block(string);
        void add_client_in(Client*);
        void add_client_out(Client*);
    };

    class MinerState {
        public:
            struct event_base * evbase_ ;
            struct event* mine_ev_;
            struct bufferevent* r_bev_;
            struct bufferevent* w_bev_;
            struct evbuffer * w_out_;
            int time_;
            int cur_block_;
            string name_;
            exponential_distribution<double> dis_;
            default_random_engine engine_;

            void reset_mining();
            bool has_new_block() const;
            Block* parse_block();
            bool find_hash(Block*, uint32_t) const;
    };



}

#endif
