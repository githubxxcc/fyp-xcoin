#ifndef XCOIN_MAIN_H_
#define XCOIN_MAIN_H_

#include <arpa/inet.h>

namespace xcoin {
    typedef struct _Server Server;
    class MinerState;
    class Block;
    class Client;


    struct _Server {
        int sd_;
    };

    class Client {
    public:
        int sd_;
        char* server_host_name_;
        in_addr_t server_ip_;
        struct bufferevent * buf_ev_;
    };


    class XState {
        public:
        Server server;

        int my_port_;

        /* Outgoing Clients */
        Client* out_client_;
        
        /*  For incoming client instance. For callback */
        Client * in_client_;
        struct event_base * evbase_;

        /*  Miner state */
        struct event_base * miner_base_;
        MinerState * miner_state_;

        char* peer_name_;

        /*  Communication Sockets to Miner */
        struct bufferevent * r_bev_;
        struct bufferevent * w_bev_;

        Client* connect_peer(char* name, char* port);
        /* Get the incoming client */ //FIXME: how about out clients
        Client* get_client() const;
        void broadcast_block(int);
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

            void reset_mining(int);
    };


    class Block {
        public: 
            int val_;
    };

    


}

#endif
