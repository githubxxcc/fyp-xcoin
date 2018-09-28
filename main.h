#ifndef XCOIN_MAIN_H_
#define XCOIN_MAIN_H_

namespace xcoin {
    typedef struct _XState XState;
    typedef struct _Client Client;
    typedef struct _Server Server;


    struct _Server {
        int sd_;
    };

    struct _Client {
        int sd_;
        char* server_host_name_;
        in_addr_t server_ip_;
        struct bufferevent * buf_ev_;
    };

    struct _XState {
        Server server;
        Client client;
        struct event_base * evbase_;

        char* peer_name_;

        /*  Communication Sockets to Miner */
        struct bufferevent * r_bev_;
        struct bufferevent * w_bev_;


    };

    class MinerState {
        public:
            struct event_base * evbase_ ;
            struct bufferevent* r_bev_;
            struct bufferevent* w_bev_;
            struct evbuffer * w_out_;
            int time;
    };


}

#endif
