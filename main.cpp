
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sstream>
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
#include <boost/asio.hpp>

#include <pthread.h>
#include <future>
#include <chrono>

#include <time.h>
#include <stack>
#include <signal.h>

#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"

#define MY_PORT 0 
#define READ_END 0
#define WRITE_END 0

#include "main.h"
#include "util.h"
#include "block.h"


using namespace std;
using namespace xcoin;

//static struct event_base *g_evbase;
void buf_err_cb(struct bufferevent *bev, short what, void *arg);
int create_miner(XState*, char*);

static void check_result(bool, int, const char*);
static stack<int> MINE_BLOCKS_Q;
static int N_NEXT_BLOCK = -1;
static int N_INIT_BLOCK = 0;
static int N_CLIENT_NEXT_BLOCK = 10;
static bool G_DO_MINE = true;

static pthread_cond_t COND = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t MUTEX = PTHREAD_MUTEX_INITIALIZER;


//extern BlockIndex* g_best_index;




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
    ss.clear();
    if(bufferevent_write(bev, data.c_str(), data.size()) != 0) {
        cerr << "buf_write_cb(): failed" << endl;
    }
}

void buf_read_cb(struct bufferevent *bev, void *arg )
{
    cout << "Reading...\n" <<endl;
    XState* state = static_cast<XState*>(arg);
    size_t n;
    stringstream ss;

    for(;;) {
        char data[8192] = {0};
        n = bufferevent_read(bev, data, sizeof(data));
        if(n <= 0) {
            break;
        }
        cout << "[MAIN] --- [NETWORK] Read: "  << data << "\n"  << endl;
        
        /*  Append to the stack */
        ss << data;
        string blk_str = ss.str();
        ss.clear();

        /*  Pass next miner block, call miner_on_read */
        if(bufferevent_write(state->w_bev_, blk_str.c_str(), blk_str.size()) != 0) {
            cerr << "buf_read_cb() : failed writing" <<endl;
        }
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
    XState *state = (XState *)arg;
    Client *client = state->in_client_;
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

    /* Create Client Instance */
    client = (Client *) calloc(1, sizeof(Client));
    if (client == NULL) {
        printf("err: client calloc failed\n");
        exit(1);
    }

    /*  Networking accepting new client  */
    socklen_t client_len = sizeof(client_addr);

    client_fd = accept(socket, (struct sockaddr *)&client_addr, &client_len);
    client->server_ip_ = client_addr.sin_addr.s_addr;

    char addr_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(client_addr.sin_addr), addr_ip, INET_ADDRSTRLEN);
    printf("Accepted : %s\n", addr_ip);
    if (client_fd < 0) {
        printf("err: client_fd < 0\n");
        return;
    }

    if (setnonblock(client_fd) < 0) {
        cerr << "err: setnonblock failed" << endl;
    }
    
    client->sd_ = client_fd;
    client->buf_ev_ = bufferevent_socket_new(state->evbase_, client_fd, 0);

    bufferevent_setcb(client->buf_ev_, buf_read_cb, NULL, buf_err_cb, state);
    bufferevent_enable(client->buf_ev_, EV_READ | EV_WRITE);

    state->in_client_ = client;
    /* TODO: add the client to the peers */
}

int 
start_server(XState * state, char* time_str)
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
    bind_address.sin_port = htons(state->my_port_);

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

    
    return 0;
}

void
miner_on_read(struct bufferevent *bev, void *arg)
{
    size_t n;
    stringstream ss;
    MinerState* s = static_cast<MinerState*>(arg);

    for(;;) {
        char data[128] = {0};
        n = bufferevent_read(bev, data, sizeof(data));
        if(n<=0)
            break;
        

        ss << data;
        auto block = Block::deserialize(ss);
        ss.clear();

        cout << "[MINER]---[MINER-MAIN] Miner Read From Main: " 
            << block
            << "\n"
            << endl;

        if(block.process_block()) {
            /*  Reset the mining */
            cout << "miner_on_read(): prcess_blocked ok\n";
            //s->reset_mining();
        } else {
            /*  FIXME: What to do  */
            cout << "miner_on_read(): process_block fails\n";
        }

        /*  Cancel the mining */
        //s->reset_mining(new_block + 1);
    }
    
}


/*  
 *  When received a block from the miner
 *  */
void 
main_on_read(struct bufferevent *bev, void *arg) 
{
    size_t n;
    int block;
    XState * state = static_cast<XState*>(arg);

    for(;;) {
        char data[8192] = {0};
        stringstream ss;
        ss.clear();
        n = bufferevent_read(bev, data, sizeof(data));
        if(n<=0)
            break;
        
        cout << "[MINER-MAIN] Main Read From Miner :" 
            << data 
            <<"\n"
            << endl;
    
            
        /*  Broadcast to peers */
        ss << data;
        state->broadcast_block(ss.str());
    }
}


void 
on_mine(int fd, short events, void* aux) 
{
    //XState* state = static_cast<XState*>(aux);
    MinerState *my_state = static_cast<MinerState*>(aux);
    string data;
    
    //FIXME: synchronize and report nonce
    /*  Create new block */
    auto blk_idx = BlockIndex::get_best_blkidx();
    assert(blk_idx != NULL);
    int nonce = rand() % 100;
    cout << "on_mine() : new nonce " << nonce << "\n";
    auto new_block = new Block(blk_idx,nonce);
    
    /*  Add new block */
    new_block->accept_block();
    //debug_chains();
    cout << "[MINER] [on_mine]:" << new_block << "\n" << endl;
    
    /*  Serialize the block for sending */
    data = new_block->serialize();
    cout << "SENDING: " << data << "\n";

    if(bufferevent_write(my_state->w_bev_, data.c_str(), data.size()) != 0) {
        cerr << "mine(): failed send to main\n";
    }
}

void *
mine(void* aux) 
{
    XState* state = static_cast<XState*>(aux);
    MinerState* my_state = state->miner_state_;

    printf("Miner running\n");    
    struct timespec ts;
    struct timeval tp;
    
    /*  Setup MIning Event */
    struct timeval tv { 5, 0};

    struct event *mine_ev = event_new(my_state->evbase_, -1, EV_PERSIST  , on_mine, my_state);
    evtimer_add(mine_ev, &tv);
    my_state->mine_ev_ = mine_ev;

    /* Loop until the main thread breaks the evloop */
    event_base_loop(my_state->evbase_, 0);
    
    cout << " miner exiting\n" ;
    pthread_exit(NULL);
}


XState*
init(int argc, char* argv[])
{
    /*  FIXME: check input */
    int res; 
    srand(time(NULL));
    
    /* Init State relevant info */
    XState* state = static_cast<XState*>(calloc(1, sizeof(XState)));
    assert(state);
    state->evbase_ = event_base_new();
    state->my_port_ = stoi(string(argv[3]));
    
    
    /*  Init Server Instance */
    printf("Starting Server\n");
    res = start_server(state, argv[4]);
    check_result(res == 0, 0, "init(): init server failes\n");
    cout << "Server Started....Looping\n";

    /* Connect to unknown peers */
   auto client = state->connect_peer(argv[1], argv[2]);
   check_result(client != NULL, 1, "init(): failed to connect to client"); 
   printf("Connected peer\n");

    /*  Set up mining thread */
    create_miner(state, argv[4]);

    /* Set up the chain genesis block */
    Block genesis = Block::genesis();
    genesis.add_to_chain();
    cout << "Genesis Block Hash: " << genesis.get_hash() << "\n";

    debug_chains();

    /*  TODO: check failure */
    return state;
}


int main(int argc, char*argv[])
{
    /*  Settup Logger */
    auto console = spdlog::stdout_color_mt("console");
    auto err_log = spdlog::stderr_color_mt("stderr");
    
    console->info("Starting");
    /*  Init randome seed */
    auto state = init(argc, argv);

    event_base_dispatch(state->evbase_);

    pthread_join(state->miner_, NULL);
    pthread_cond_destroy(&COND);
    pthread_mutex_destroy(&MUTEX);
    //pthread_exit(NULL);
    return 0;
}


void 
on_kill(int fd, short events, void* aux)
{
    XState* state = static_cast<XState*>(aux);
    
    //struct timeval tv { 1,0};
    cout << "Shutting down miner" << endl;
    //event_base_loopbreak(state->miner_base_);
    G_DO_MINE = false;
    event_base_loopbreak(state->evbase_);
}

int
create_miner(XState *state, char* time_str) 
{
    
    /*  Comm bev, named by Read side */
    struct bufferevent * main_bev_pair[2];  
    struct bufferevent * miner_bev_pair[2];

    /* Miner state init */
    MinerState *miner_state = static_cast<MinerState*>(calloc(1, sizeof(MinerState)));
    miner_state->evbase_ = event_base_new();
    miner_state->time_ = stoi(string(time_str));
    state->miner_base_ = miner_state->evbase_;
    state->miner_state_ = miner_state;

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
    
    /*  Set up callbacks for the mining */
    bufferevent_setcb(state->r_bev_, main_on_read, NULL, error_cb, state);
    bufferevent_setcb(miner_state->r_bev_, miner_on_read, NULL, error_cb, miner_state);

    /*  Register SigINT */
    struct event* sig_ev = evsignal_new(state->evbase_, SIGINT, on_kill, state);
    event_add(sig_ev, NULL);

    pthread_create(&state->miner_, NULL, mine, (void*) state);
    printf("Created miner\n");
    return 0;
}


static void 
check_result(bool ok, int fatal, const char* msg) 
{
    if(!ok) {
        cout << string(msg) << endl;
        if(fatal) {
            exit(1);
        }
    }
}


void MinerState::reset_mining(int new_block)
{
    /* Cancel timer */
    assert(mine_ev_ != NULL);
    assert(evtimer_pending(mine_ev_, NULL));
    cout << "reset_mining():cancelling event " << new_block << "\n" <<endl;
    if (event_del(mine_ev_) != 0) {
        cerr << "reset_mining(): event_del faield\n";
    }
    
    /* Restart next on next block*/
    cur_block_ = new_block;

    /* Random sleep time */
    struct timeval tv { rand() % 50 + this->time_, 0};
    mine_ev_ = event_new(evbase_, -1, EV_PERSIST , on_mine, this);
    event_add(mine_ev_, &tv);
}

void XState::broadcast_block(string data) 
{
    Client* clt = this->get_client();

    if (clt == NULL) {
        cout << "broadcast_block(): no client conencted" <<endl;
        return;
    }

    if(bufferevent_write(clt->buf_ev_, data.c_str(), data.size()) != 0) {
        cerr << "broadcast_block(): failed" << endl;
    } else {
        cout << "broadcast_block() : sent, " << data << endl;
    }

}


Client*
XState::get_client() const
{
    return this->out_client_;
}

Client* 
XState::connect_peer(char* server_host_name, char* peer_port)
{
    Client * client = static_cast<Client*>(calloc(1, sizeof(Client)));
    check_result(client != NULL,0,"connect_peer(): calloc client failed");

    client->server_host_name_ = strndup(server_host_name, (size_t) 50);
    printf("server_host_name: %s\n", client->server_host_name_);
    /*  get address */ 
    struct addrinfo* server_info;
    int res = getaddrinfo(client->server_host_name_, NULL, NULL, &server_info);
    if(res == -1) {
        //error
        printf("err in address\n");
        exit(1);
    }
    client->server_ip_ = ((struct sockaddr_in*) (server_info->ai_addr))->sin_addr.s_addr;

    char str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(client->server_ip_), str, INET_ADDRSTRLEN);
    printf("server ip: %s\n", str);

    freeaddrinfo(server_info);

    client->sd_ = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if (client->sd_ == -1) {
        //error
        printf("start_client: unable to open socket\n");
        return NULL;
    }

    /*  client socket address info */
    sockaddr_in listen_addr;
    memset(&listen_addr, 0, sizeof(listen_addr));

    listen_addr.sin_family = AF_INET;
    listen_addr.sin_addr.s_addr = client->server_ip_;
    string port(peer_port);
    listen_addr.sin_port=htons(stoi(port));

    /*  Connect to the server */
    res = connect(client->sd_, (sockaddr *) &listen_addr, sizeof(listen_addr));
    if (res == -1 && errno != EINPROGRESS) {
        printf("start_client: failed to connect socket\n");
        return NULL;
    }


    /*  Create bufferevents */
    client->buf_ev_ = bufferevent_socket_new(this->evbase_, client->sd_, 0);
    res = bufferevent_enable(client->buf_ev_, EV_WRITE|EV_READ);
    check_result(res == 0, 0, "connect_peer(): bufferevent_enable failed");

    this->out_client_ = client;
    return client;
}
