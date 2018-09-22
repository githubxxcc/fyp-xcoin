
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

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

#define MY_PORT 12345

#include "main.h"



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
    /*  get address */ 
    struct addrinfo* server_info;
    int res = getaddrinfo(state->client.server_host_name_, NULL, NULL, &server_info);
    if(res == -1) {
        //error
        return -1;
    }
    state->client.server_ip_ = ((struct sockaddr_in*) (server_info->ai_addr))->sin_addr.s_addr;
    freeaddrinfo(server_info);

    state->client.sd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (state->client.sd_ == -1) {
        //error
        return -1;
    }

    /*  client socket address info */
    struct sockaddr_in listen_addr;
    memset(&listen_addr, 0, sizeof(listen_addr));

    listen_addr.sin_family = AF_INET;
    listen_addr.sin_addr.s_addr = state->client.server_ip_;
    listen_addr.sin_port=htons(MY_PORT);

    /*  Connect to the server */
    res = connect(state->client.sd_, (struct socketaddr *) &listen_addr, sizeof(listen_addr));
    if (res == -1 && errno != EINPROGRESS) {
        return -1;
    }

    struct event *ev_read;
    struct event *ev_write;
    struct event_base *base = event_base_new();
    struct timeval tv = {5, 0};
    ev_read = event_new(base, state->client.sd_, EV_READ | EV_PERSIST, on_read, &state->client);
    ev_write= event_new(base, state->client.sd_, EV_WRITE, on_write, NULL); /* Write once */

    event_add(ev_read, NULL);
    event_add(ev_write, &tv); 
    event_base_dispatch(base);

    return 0;
}

void on_accept(int socket, short ev, void *arg)
{
    int client_fd;
    struct sockaddr_in client_addr;
    Client* client;
    socklen_t client_len = sizeof(client_addr);


    client_fd = accept(socket, (struct sockaddr *)&client_addr, &client_len);
    if (client_fd < 0) {
        printf("err: client_fd < 0\n");
        return;
    }

    client = calloc(1, sizeof(*client));
    if (client == NULL) {
        printf("err: client calloc failed\n");
        exit(1);
    }

    event_set(&client->ev_read_, client_fd, EV_READ | EV_PERSIST, on_read, client);
    event_add(&client->ev_read_, NULL);
    
    printf("Accepted conn from %s\n", inet_ntoa(client_addr.sin_addr));
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
    int res = bind(state->server.sd_, (struct socketaddr *) &bind_address, sizeof(bind_address));
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

    ev_accept = event_new(base, state->server.sd_, EV_READ | EV_PERSIST, on_accept, (char*) "ACCEPTING CONN");
    event_add(ev_accept, NULL);

    event_base_dispatch(base);

    return 0;
}






XState*
init(int argc, char* argv[])
{
    /*  FIXME: check input */
    int res; 

    XState *state = calloc(1, sizeof(XState));
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
    XState *state = init(argc, argv);

    return 0;
}
