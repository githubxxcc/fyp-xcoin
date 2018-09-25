
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>

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

void on_write(evutil_socket_t socket, short flags, void * args )
{
    char *msg = (char*)"Hello there!";
    int ret = send(socket, msg, strlen(msg), 0);
    printf("connected, write to server : %d\n", ret);
}

void on_read(evutil_socket_t socket, short flags, void *args )
{
    char buf[256];
    Client *client = (Client *)args;

    int ret = recv(socket, buf, 256, 0);
    printf("read: %d bytes\n", ret);

    if (ret == 0) {
        printf("client disconnected \n");
        close(socket);
        event_base_loopexit(client->base_, NULL);
        return ;
    } else if (ret < 0) {
        printf("socket error %s", strerror(errno));
        close(socket);
        event_base_loopexit(client->base_, NULL);
        return;
    }

    printf("read: %s", buf);
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
    struct event_base *base = event_base_new();
    struct timeval tv = {5, 0};
    ev_read = event_new(base, state->client.sd_, EV_READ | EV_PERSIST, on_read, &state->client);
    ev_write= event_new(base, state->client.sd_, EV_WRITE, on_write, NULL); /* Write once */

    //event_add(ev_read, NULL); /*  No echoing back yet */
    event_add(ev_write, &tv); 
    event_base_dispatch(base);

    return 0;
}

void on_accept(int socket, short ev, void *arg)
{
    int client_fd;
    struct sockaddr_in client_addr;
    struct event_base * base = (struct event_base*) arg;
    Client* client;
    socklen_t client_len = sizeof(client_addr);
    printf("Accepting conn \n");

    client_fd = accept(socket, (struct sockaddr *)&client_addr, &client_len);
    printf("Accepted \n");
    if (client_fd < 0) {
        printf("err: client_fd < 0\n");
        return;
    }

    client = (Client *) calloc(1, sizeof(Client));
    if (client == NULL) {
        printf("err: client calloc failed\n");
        exit(1);
    }

    //printf("Accepted conn from %s\n", inet_ntoa(client_addr.sin_addr));
    client->base_ = base; 
    struct event * ev_read = event_new(base, client_fd, EV_READ | EV_PERSIST, on_read, client);
    event_add(ev_read, NULL);
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

    struct event *ev_accept; 
    struct event_base *base = event_base_new();

    ev_accept = event_new(base, state->server.sd_, EV_READ | EV_PERSIST, on_accept, base);
    event_add(ev_accept, NULL);
    
    printf("Waiting for connection \n");
    event_base_dispatch(base);

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

    pthread_create(&t1, NULL, mine, (void *) time);
    pthread_create(&t2, NULL, mine, (void *) time);

    pthread_join(t1, &status);
    pthread_join(t2, &status);

    pthread_exit(NULL);
}
