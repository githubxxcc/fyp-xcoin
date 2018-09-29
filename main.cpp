
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
#include <err.h>


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
#include <future>
#include <chrono>

#include <time.h>
#include <stack>
#include <signal.h>


#define MY_PORT 12345
#define READ_END 0
#define WRITE_END 0

#include "main.h"



using namespace std;
using namespace xcoin;

//static struct event_base *g_evbase;
void buf_err_cb(struct bufferevent *bev, short what, void *arg);
int create_miner(XState*);

static void check_result(const char*, int);
static stack<int> MINE_BLOCKS_Q;
static int N_NEXT_BLOCK = -1;
static int N_INIT_BLOCK = 0;
static int N_CLIENT_NEXT_BLOCK = 3;

static pthread_cond_t COND = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t MUTEX = PTHREAD_MUTEX_INITIALIZER;

void 
buf_write_cb(int fd, short events, void * args )
{
    struct bufferevent *bev = (struct bufferevent*)args;
    stringstream ss;
    string data;
    cout << "buf_write_cb(): called" << endl;
    ss << N_CLIENT_NEXT_BLOCK;
    N_CLIENT_NEXT_BLOCK++;
    
    data = ss.str();
    if(bufferevent_write(bev, data.c_str(), data.size()) != 0) {
        cerr << "buf_write_cb(): failed" << endl;
    }
}

void buf_read_cb(struct bufferevent *bev, void *arg )
{
    cout << "Reading..." <<endl;
    Client* client = static_cast<Client*>(arg);
    char data[8192];
    size_t n;
    stringstream ss;

    for(;;) {
        n = bufferevent_read(bev, data, sizeof(data));
        cout << "Read: " << n << endl;
        if(n <= 0) {
            break;
        }
        cout << data << endl;
        
        /*  Append to the stack */
        ss << data;
        
        /* FIXME: checking on number */
        int received_block;
        ss >> received_block;
        
        /*  FIXME: checking errors */
        pthread_mutex_lock(&MUTEX);
        MINE_BLOCKS_Q.push(received_block);
        pthread_cond_signal(&COND);
        pthread_mutex_unlock(&MUTEX);
    }
}

void error_cb(struct bufferevent *bev, short events, void *aux)
{
    if (events & BEV_EVENT_CONNECTED) {
        cout << "connected to miner/main" <<endl;
    } 
    else if (events & BEV_EVENT_ERROR) {
        cerr << "error_cb(): error" << endl;
        bufferevent_free(bev);
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
    

    state->client.buf_ev_ = bufferevent_socket_new(state->evbase_, state->client.sd_, 0);
    bufferevent_enable(state->client.buf_ev_,  EV_WRITE);
    //bufferevent_setcb(state->client.buf_ev_, NULL, buf_write_cb, buf_event_cb, NULL);
    //event_add(ev_read, NULL); /*  No echoing back yet */
    ev_write= event_new(state->evbase_, state->client.sd_, EV_WRITE, buf_write_cb, state->client.buf_ev_); /* Write once */

    event_add(ev_write, &tv); 
    event_base_loop(state->evbase_, EVLOOP_NONBLOCK);
    while(1){}

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
    cout<< "buf_err_cb" << endl;
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
    XState * state = static_cast<XState*>(arg);
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
    client->buf_ev_ = bufferevent_socket_new(state->evbase_, client_fd, 0);
    bufferevent_setcb(client->buf_ev_, buf_read_cb, NULL, buf_err_cb, client);
    bufferevent_enable(client->buf_ev_, EV_READ | EV_WRITE);

    /* TODO: add the client to the peers */
}

int 
start_server(XState * state) 
{
    /*  Record Peer */
    //state->peer_name_ = server_host_name;

    /*  Create the socket  */
    state->server.sd_ = socket(AF_INET, (SOCK_STREAM | SOCK_NONBLOCK), 0);
    if(state->server.sd_ == -1) {
        cerr << "start_server(): socket create failed" << endl;
        return -1;
    }

    struct sockaddr_in bind_address;
    bind_address.sin_family = AF_INET;
    bind_address.sin_addr.s_addr = INADDR_ANY;
    bind_address.sin_port = htons(MY_PORT);

    /*  bind the socket to ther server port */
    int res = bind(state->server.sd_, (struct sockaddr *) &bind_address, sizeof(bind_address));
    if (res == -1) {
        cerr << "start_server(): bind failed" << endl;
        return -1;
    }
    
    /* Listen  */
    res = listen(state->server.sd_, 100);
    if (res == -1) {
        //error
        return -1;
    }
    
    /*  Set reusable  */
    int reuseaddr_on = 1;
    setsockopt(state->server.sd_, SOL_SOCKET, SO_REUSEADDR, &reuseaddr_on, 
                sizeof(reuseaddr_on));


    struct event *ev_accept; 
    ev_accept = event_new(state->evbase_, state->server.sd_, EV_READ | EV_PERSIST, on_accept, state);
    event_add(ev_accept, NULL);
    
    /*  Running  */
    printf("Waiting for connection \n");

    /*  Set up mining thread */
    create_miner(state);
    
    //event_base_loop(state->evbase_, EVLOOP_NONBLOCK);
    return 0;
}

void
miner_on_read(struct bufferevent *bev, void *arg)
{
    char data[8192];
    size_t n;
    MinerState* s = static_cast<MinerState*>(arg);

    for(;;) {
        n = bufferevent_read(bev, data, sizeof(data));
        if(n<=0)
            break;
        
        cout << "Miner Read From Main" 
            << n  << " Bytes\n"
            << data 
            << endl;
    }
    
    /*  Cancel the mining */

    /*  Update the state */
}

void 
main_on_read(struct bufferevent *bev, void *arg) 
{
    char data[8192];
    size_t n;
    cout << "Reading from main\n";
    for(;;) {
        n = bufferevent_read(bev, data, sizeof(data));
        if(n<=0)
            break;
        
        cout << "Main Read From Miner" 
            << n  << " Bytes\n"
            << data 
            << endl;
    }
}


void 
on_mine(int fd, short events, void* aux) 
{
    MinerState *my_state = static_cast<MinerState*>(aux);
    cout << "Yes, new block:" << my_state->cur_block <<  endl;

    stringstream ss;
    string data;
    cout << "buf_write_cb(): called" << endl;
    ss << N_CLIENT_NEXT_BLOCK;
    N_CLIENT_NEXT_BLOCK++;
    
    data = ss.str();
    if(bufferevent_write(my_state->w_bev_, data.c_str(), data.size()) != 0) {
        cerr << "mine(): failed send to main\n";
    }
    
    //FIXME
    /* Reset the timer */
}

void *
mine(void* aux) 
{
    MinerState* my_state = static_cast<MinerState*>(aux);

    struct timespec ts;
    struct timeval tp;
   // int res;

   // /*  Acquire the lock  */
   // pthread_mutex_lock(&MUTEX);
   // int to_mine = N_INIT_BLOCK;
   //
    //while(1) {
    //    res = gettimeofday(&tp, NULL);
    //    check_result("miner gettimeofday: failed\n", res);
    //    ts.tv_sec = tp.tv_sec;
    //    ts.tv_nsec = tp.tv_usec * 1000;
    //    ts.tv_sec += my_state->time;
    //    
    //    res = pthread_cond_timedwait(&COND, &MUTEX, &ts);

    //    if(res == ETIMEDOUT) {
    //        /*  Found a block without interrupt */
    //        if(MINE_BLOCKS_Q.empty()) {
    //            cout << "Found new block\n";
    //            /*  Informing the main */
    //            N_NEXT_BLOCK = to_mine;

    //            /*  Mine the next one */
    //            to_mine++;
    //        } else {
    //            /*  Another new block comes when we are done */
    //            cerr << "new block but also timeout\n";
    //            to_mine = MINE_BLOCKS_Q.top();
    //            MINE_BLOCKS_Q.pop();
    //        }
    //    } else {
    //        cout << "Main interrupted me\n";
    //        if(!MINE_BLOCKS_Q.empty()) {
    //            cout << "New block arrives\n";
    //            to_mine = MINE_BLOCKS_Q.top();
    //            MINE_BLOCKS_Q.pop();
    //        } else {
    //            cerr << "spurious wakeup. How to deal with this\n";
    //        }
    //    }
    //}

    //pthread_mutex_unlock(&MUTEX);
    

    
    /*  Setup MIning Event */
    struct timeval tv = {5, 0};
    struct event *mine_ev = event_new(my_state->evbase_, -1, EV_PERSIST , on_mine, my_state);
        //evtimer_new(miner_state->evbase_, on_mine, miner_state);  
    evtimer_add(mine_ev, &tv);
    
    event_base_dispatch(my_state->evbase_);

    //while(true) {
    //    future<bool> fut = async(launch::async, [](MinerState* miner_state){
    //                int res = event_base_dispatch(miner_state->evbase_);
    //                if(res == 0) {
    //                    cout << "normal" <<endl;
    //                } else if (res == 1) {
    //                    cout << "no more events" <<endl;
    //                } else if( res == -1) {
    //                    cout << "errored "<<endl;
    //                } else {
    //                    cout << "whut" <<endl;
    //                }
    //                return true;
    //            }, my_state);

    //    chrono::milliseconds span (5000);
    //    if(fut.wait_for(span) == future_status::timeout) {
    //        cout << "YAY found a block\n";

    //        /*  Inform the main thread */

    //        char *msg = (char*)"Hello main. I am your miner!\n";
    //       // if(bufferevent_write(my_state->w_bev_, msg, strlen(msg)) != 0) {
    //       //     cerr << "mine(): failed send to main\n";
    //       // }
    //        
    //        evbuffer_add_printf(my_state->w_out_, "hwwlooo~");
    //        cout << "Sent to main\n";
    //        event_base_loopbreak(my_state->evbase_);
    //    } else {
    //        cout << "GOT interrupted by main\n";
    //        bool res = fut.get();
    //    }
    //}

    pthread_exit(NULL);
}


XState*
init(int argc, char* argv[])
{
    /*  FIXME: check input */
    int res; 

    XState *state = static_cast<XState*>(calloc(1, sizeof(XState)));
    assert(state);
    state->evbase_ = event_base_new();

   if (argc == 2) {
       printf("Starting Client\n");
       res = start_client(state, argv[1]);
       check_result("init(): init server failes\n",res);
   } else {
       printf("Starting Server\n");
       res = start_server(state);
       check_result("init(): init server failes\n",res);
       cout << "Server Started....Looping\n";
       event_base_loop(state->evbase_, 0);

      // while(true) {
      //     /* Loop to pull events */
      //      if(N_NEXT_BLOCK != -1) {
      //          cout << "Mined block :" << N_NEXT_BLOCK <<endl;
      //          
      //          //FIXME: send to peers
      //          sent_block = N_NEXT_BLOCK;
      //          N_NEXT_BLOCK = -1;
      //          cout << "Sending block :" << sent_block << endl;
      //          cout << "Sent block :" << endl;
      //      }
      // }
   }

   
    
    /*  TODO: check failure */
    return state;
}


int main(int argc, char*argv[])
{
    printf("Starting....\n");
    void *status;

    init(argc, argv);
    
    pthread_cond_destroy(&COND);
    pthread_mutex_destroy(&MUTEX);
    pthread_exit(NULL);
}


void 
on_kill(int fd, short events, void* aux)
{
    XState* state = static_cast<XState*>(aux);
    
    struct timeval tv { 1,0};
    cout << "Shutting down miner" << endl;
    event_base_loopbreak(state->miner_base_);
    event_base_loopexit(state->evbase_, &tv);
}

int
create_miner(XState *state) 
{
    pthread_t miner;
    
    /*  Comm bev, named by Read side */
    struct bufferevent * main_bev_pair[2];  
    struct bufferevent * miner_bev_pair[2];

    /* Miner state init */
    MinerState *miner_state = static_cast<MinerState*>(calloc(1, sizeof(MinerState)));
    miner_state->evbase_ = event_base_new();
    miner_state->time = 5;
    state->miner_base_ = miner_state->evbase_;

    bufferevent_pair_new(miner_state->evbase_, 0, main_bev_pair);
    bufferevent_pair_new(state->evbase_, 0, miner_bev_pair);

    miner_state->w_bev_ = main_bev_pair[0];
    miner_state->r_bev_ = miner_bev_pair[0];

    state->r_bev_ = main_bev_pair[1];
    state->w_bev_ = miner_bev_pair[1];

    bufferevent_enable(state->r_bev_, EV_READ | EV_PERSIST);
    bufferevent_enable(miner_state->w_bev_, EV_WRITE);
    bufferevent_enable(miner_state->r_bev_, EV_READ | EV_PERSIST);
    bufferevent_enable(state->w_bev_, EV_WRITE);

    bufferevent_setcb(state->r_bev_, main_on_read, NULL, error_cb, state);
    bufferevent_setcb(miner_state->r_bev_, miner_on_read, NULL, error_cb, miner_state);

    /*  Register SigINT */
    struct event* sig_ev = evsignal_new(state->evbase_, SIGINT, on_kill, state);
    event_add(sig_ev, NULL);


    //if(evutil_socketpair(AF_UNIX, SOCK_STREAM, 0, up) != 0) {
    //    //error
    //    return -1; 
    //}

    //if(evutil_socketpair(AF_UNIX, SOCK_STREAM, 0, down) != 0) {
    //    //error 
    //    return -1;
    //}
   // struct bufferevent* miner_r_bev = bufferevent_socket_new(miner_state->evbase_, down[READ_END], 0);
   // struct bufferevent* miner_w_bev = bufferevent_socket_new(miner_state->evbase_, up[WRITE_END], 0);

   // struct bufferevent* main_r_bev = bufferevent_socket_new(state->evbase_, up[READ_END], 0);
   // struct bufferevent* main_w_bev = bufferevent_socket_new(state->evbase_, down[WRITE_END], 0);

   // bufferevent_enable(miner_r_bev, EV_READ | EV_PERSIST);
   // bufferevent_enable(miner_w_bev, EV_WRITE);
   // bufferevent_enable(main_r_bev, EV_READ | EV_PERSIST);
   // bufferevent_enable(main_w_bev, EV_WRITE);

   // bufferevent_setcb(miner_r_bev, miner_on_read, NULL, error_cb, miner_state);
   // bufferevent_setcb(main_r_bev, main_on_read, NULL, error_cb, NULL);

   // state->r_bev_ = main_r_bev;
   // state->w_bev_ = main_w_bev;

   // miner_state->r_bev_ = miner_r_bev;
   // miner_state->w_bev_ = miner_w_bev;
   // miner_state->w_out_ = bufferevent_get_output(main_r_bev);
   
    pthread_create(&miner, NULL, mine, (void*) miner_state);

    return 0;
}


static void 
check_result(const char* msg, int res) 
{
    if(res) {
        cout << string(msg) << endl;
        exit(1);
    }
}
