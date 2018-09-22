#ifndef XCOIN_MAIN_H_
#define XCOIN_MAIN_H_
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
    struct event_base* base_;
    struct event* ev_read_;
};

struct _XState {
    Server server;
    Client client;
};

#endif
