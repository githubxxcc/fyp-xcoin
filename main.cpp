
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <fcntl.h>

#include <assert.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>


/* Required by event.h. */
#include <sys/time.h>
#include <event2/event.h>
#include <event2/event_struct.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>

/*  For multithreads */
//#include <boost/thread.hpp>
#include <boost/chrono.hpp>
#include <pthread.h>


#define MY_PORT 12345

#include "main.h"



using namespace std;
using namespace xcoin;

static struct event_base *g_evbase;
void buf_err_cb(struct bufferevent *bev, short what, void *arg);
static void check_result(const char*, int);

void 
buf_write_cb(int fd, short events, void * args )
{
    struct bufferevent *bev = (struct bufferevent*)args;
    char *msg = (char*)"Hello there!\n";
    cout << "connected to server" << endl;
    if(bufferevent_write(bev, msg, strlen(msg)) != 0) {
        cerr << "buf_write_cb(): failed" << endl;
    }
}

void buf_read_cb(struct bufferevent *bev, void *arg )
{
    Client* client = static_cast<Client*>(arg);
    char data[8192];
    size_t n;
    cout << "Reading..." <<endl;
    for(;;) {
        n = bufferevent_read(bev, data, sizeof(data));
        cout << "Read: " << n << endl;
        if(n <= 0) {
            break;
        }
        cout << data << endl;
    }
}

void buf_event_cb(struct bufferevent *bev, short events, void *ptr)
{
    if (events & BEV_EVENT_CONNECTED) {
        cout << "connected to server" <<endl;
    } 
    else if (events & BEV_EVENT_ERROR) {
        cerr << "bf_event_cb(): error" << endl;
        bufferevent_free(bev);
    }
}

int
start_client(XState* state, char* server_host_name)
{

    state->client.server_host_name_ = strndup(server_host_name, (size_t) 50);
    printf("server_host_name: %s\n", state->client.server_host_name_);
    /*  get address */ 
    struct addrinfo* server_info;
    int res = getaddrinfo(state->client.server_host_name_, NULL, NULL, &server_info);
    if(res == -1) {
        //error
        printf("err in address\n");
        exit(1);
    }
    state->client.server_ip_ = ((struct sockaddr_in*) (server_info->ai_addr))->sin_addr.s_addr;

    char str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(state->client.server_ip_), str, INET_ADDRSTRLEN);
    printf("server ip: %s\n", str);

    freeaddrinfo(server_info);

    state->client.sd_ = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if (state->client.sd_ == -1) {
        //error
        printf("start_client: unable to open socket\n");
        return -1;
    }

    /*  client socket address info */
    sockaddr_in listen_addr;
    memset(&listen_addr, 0, sizeof(listen_addr));

    listen_addr.sin_family = AF_INET;
    listen_addr.sin_addr.s_addr = state->client.server_ip_;
    listen_addr.sin_port=htons(MY_PORT);

    /*  Connect to the server */
    res = connect(state->client.sd_, (sockaddr *) &listen_addr, sizeof(listen_addr));
    if (res == -1 && errno != EINPROGRESS) {
        printf("start_client: failed to connect socket\n");
        return -1;
    }

    struct event *ev_read;
    struct event *ev_write;
    struct timeval tv = {5, 0};
    //ev_read = event_new(g_evbase, state->client.sd_, EV_READ | EV_PERSIST, on_read, &state->client);
    

    state->client.buf_ev_ = bufferevent_socket_new(g_evbase, state->client.sd_, BEV_OPT_CLOSE_ON_FREE);
    bufferevent_enable(state->client.buf_ev_,  EV_WRITE);
    //bufferevent_setcb(state->client.buf_ev_, NULL, buf_write_cb, buf_event_cb, NULL);
    //event_add(ev_read, NULL); /*  No echoing back yet */
    ev_write= event_new(g_evbase, state->client.sd_, EV_WRITE, buf_write_cb, state->client.buf_ev_); /* Write once */

    event_add(ev_write, &tv); 
    event_base_dispatch(g_evbase);

    return 0;
}

/* *
 *  * Set a socket to non-blocking mode.
 *   */
int
setnonblock(int fd)
{
    int flags;

    flags = fcntl(fd, F_GETFL);
    if (flags < 0)
        return flags;
    flags |= O_NONBLOCK;
    if (fcntl(fd, F_SETFL, flags) < 0)
        return -1;

    return 0;
}

/*  Called when error  */
void
buf_err_cb(struct bufferevent *bev, short what, void *arg)
{
    Client *client = (Client *)arg;

    if (what & BEV_EVENT_EOF) {
        /*  Client disconnected, remove the read event and the
         *   * free the client structure. */
        printf("Client disconnected.\n");
    }
    else {
        cerr <<"Client socket error, disconnecting" << endl;
    }

    /* TODO: Remove the client from the tailq. */
    
    bufferevent_free(client->buf_ev_);
    close(client->sd_);
    free(client);
}



void on_accept(int socket, short ev, void *arg)
{
    int client_fd;
    struct sockaddr_in client_addr;
    struct event_base * base = (struct event_base*) arg;
    Client* client;

    /*  Networking accepting new client  */
    socklen_t client_len = sizeof(client_addr);
    printf("Accepting conn \n");

    client_fd = accept(socket, (struct sockaddr *)&client_addr, &client_len);
    printf("Accepted \n");
    if (client_fd < 0) {
        printf("err: client_fd < 0\n");
        return;
    }

    if (setnonblock(client_fd) < 0) {
        cerr << "err: setnonblock failed" << endl;
    }
    
    /* Create Client Instance */
    client = (Client *) calloc(1, sizeof(Client));
    if (client == NULL) {
        printf("err: client calloc failed\n");
        exit(1);
    }
    client->sd_ = client_fd;

    client->buf_ev_ = bufferevent_socket_new(g_evbase, client_fd, BEV_OPT_CLOSE_ON_FREE);
    bufferevent_setcb(client->buf_ev_, buf_read_cb, NULL, buf_err_cb, client);
    bufferevent_enable(client->buf_ev_, EV_READ | EV_WRITE);

    /* TODO: add the client to the peers */
}

int 
start_server(XState * state) 
{
    /*  Create the socket  */
    state->server.sd_ = socket(AF_INET, (SOCK_STREAM | SOCK_NONBLOCK), 0);

    if(state->server.sd_ == -1) {
        //failed
        return -1;
    }

    struct sockaddr_in bind_address;
    bind_address.sin_family = AF_INET;
    bind_address.sin_addr.s_addr = INADDR_ANY;
    bind_address.sin_port = htons(MY_PORT);
    


    /*  bind the socket to ther server port */
    int res = bind(state->server.sd_, (struct sockaddr *) &bind_address, sizeof(bind_address));
    if (res == -1) {
        //failed
        return -1;
    }

    res = listen(state->server.sd_, 100);
    if (res == -1) {
        return -1;
    }

    int reuseaddr_on = 1;
    setsockopt(state->server.sd_, SOL_SOCKET, SO_REUSEADDR, &reuseaddr_on, 
                sizeof(reuseaddr_on));

    struct event *ev_accept; 

    ev_accept = event_new(g_evbase, state->server.sd_, EV_READ | EV_PERSIST, on_accept, NULL);
    event_add(ev_accept, NULL);
    
    printf("Waiting for connection \n");
    event_base_dispatch(g_evbase);

    return 0;
}



void *
mine(void* interval) 
{
    long n= (long)interval;
    while(true) {
        sleep(n);
        cout << "wake up liao" << endl;
    }

    pthread_exit(NULL);
}


XState*
init(int argc, char* argv[])
{
    /*  FIXME: check input */
    int res; 

    XState *state = static_cast<XState*>(calloc(1, sizeof(XState)));
    assert(state);
    

   if (argc == 2) {
       printf("Starting Client\n");
       res = start_client(state, argv[1]);
   } else {
       printf("Starting Server\n");
       res = start_server(state);
   }
   
    //res = start_server(state);

    check_result("init(): init server failes\n",res);
    
    /*  TODO: check failure */
    return state;
}


int main(int argc, char*argv[])
{
    printf("Starting....\n");
    //boost::thread t1 { init, argc, argv };
    pthread_t t1;
    pthread_t t2;
    long time = 100;
    void *status;
    g_evbase = event_base_new();

    pthread_create(&t1, NULL, mine, (void *) time);

    init(argc, argv);


    pthread_join(t1, &status);
    pthread_exit(NULL);
}


static void 
check_result(const char* msg, int res) 
{
    if(res) {
        cout << string(msg) << endl;
        exit(1);
    }
}
