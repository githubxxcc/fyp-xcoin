
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sstream>
#include <arpa/inet.h>
#include <fcntl.h>

//for exp distribution
#include <random>

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
#include "net.h"


using namespace std;
using namespace xcoin;

namespace b_po = boost::program_options;

/*  ***************
 *  Static Methods 
 *  ****************/

//static struct event_base *g_evbase;
static void parse_cmd(int, char**);
void buf_err_cb(struct bufferevent *bev, short what, void *arg);
int create_miner();
void init_ping();
static void check_result(bool, int, const char*);
static stack<int> MINE_BLOCKS_Q;


boost::program_options::variables_map INPUT_VAR_MAP;
Config SYS_CONFIG;
XState* SYS_STATE;
static int N_NEXT_BLOCK = -1;
static int N_INIT_BLOCK = 0;
static int N_CLIENT_NEXT_BLOCK = 10;
static bool G_RECEIVED = false;
//static bool G_DO_MINE = true;

static pthread_cond_t COND = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t MUTEX = PTHREAD_MUTEX_INITIALIZER;


//extern BlockIndex* g_best_index;




//void 
//buf_write_cb(int fd, short events, void * args )
//{
//    struct bufferevent *bev = (struct bufferevent*)args;
//    stringstream ss;
//    string data;
//    cout << "buf_write_cb(): called" << endl;
//    ss << N_CLIENT_NEXT_BLOCK;
//    N_CLIENT_NEXT_BLOCK++;
//    
//    data = ss.str();
//    ss.clear();
//    if(bufferevent_write(bev, data.c_str(), data.size()) != 0) {
//        cerr << "buf_write_cb(): failed" << endl;
//    }
//}

void init_ping() 
{
    auto console = spdlog::get("console");
    auto nwk = spdlog::get("nwk");
    /*  Create a message */
    int64_t start_us = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    
    console->info("[ init_ping ] Start: {}, Hop: {}", start_us, 1);

    SYS_STATE->broadcast_ping(start_us, 1);
    G_RECEIVED = true;
    
    /*  Broadcast to outgoing peers */
   // for(auto itr = out_clients_.begin(); itr != out_clients_.end(); ++itr) {
   //     auto clt = itr->second;

   //     if(bufferevent_write(clt->buf_ev_, data.c_str(), data.size()) !=0) {
   //         spdlog->warn("[Main - init_ping] Failed");
   //     } else {
   //         nwk->info("[Main - init_ping] Sent to {}", itr->first); 
   //     }
   // }
}



void buf_read_cb(struct bufferevent *bev, void *arg )
{
    XState* state = SYS_STATE;
    size_t n;
    stringstream ss;
    auto console = spdlog::get("console");
    auto err = spdlog::get("stderr");

    for(;;) {
        char data[8192] = {0};
        n = bufferevent_read(bev, data, sizeof(data));
        if(n <= 0) break;

        int64_t end_us = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        console->debug("[Main - buf_read_cb] Deserialziing Data: {}", data);
        ss << data;
        while(ss.rdbuf()->in_avail()) {
            int64_t start_us;
            int hop;

            //ss >> start_us >> hop;

            PingMsg msg = PingMsg::deserialize(ss);

            //if(!state->ping_state_->received_) {
            if(!G_RECEIVED){
                //Send to peers
                console->info("[Main - buf_read_cb] Received {}, Hop {}", end_us - msg.start_us, msg.hop+1);
                state->broadcast_ping(msg.start_us, msg.hop+1);
            } else {
                console->debug("[Main - buf_read_cb] Repeated {}, Hope {}", end_us - msg.start_us, msg.hop);
            }
            //state->ping_state_->received_ = true;
            G_RECEIVED = true;
        }
    }
   // for(;;) {
   //     char data[8192] = {0};
   //     n = bufferevent_read(bev, data, sizeof(data));
   //     if(n <= 0) {
   //         break;
   //     }
   //     spdlog::get("network")->info("[Main - read_network] Read: {}", data);

   //     /*  Append to the stack */
   //     ss << data;

   //     /*  Process the block here */
   //     while() {
   //         int data_size;
   //         ss >> data_size;
   //         vector<char> res(data_size);
   //         ss.read(&res[0], data_size);
   //         string str = string(res.begin(), res.begin()+data_size);
   //         auto block = Block::deserialize(stringstream(str));
   //         console->debug("[Main - buf_read_cb] Block: {}" , block.to_string());

   //         if(block.process_block()) {
   //             /*  Reset the mining */
   //             console->info("[Main - buf_read_cb] PROCESS BLOCK OK : {}", block.to_string());
   //             state->miner_state_->reset_mining();

   //             /*  Relay the block to others if needed*/
   //             state->broadcast_block(ss.str());
   //         } else {
   //             err->warn("[Miner - buf_read_cb] Process blocked failed");
   //         }
   //     }


   //     ss.clear();

   //     /*  Pass next miner block, call miner_on_read */
   //     //string blk_str = ss.str();
   //     //ss.clear();
   //     //if(bufferevent_write(state->w_bev_, blk_str.c_str(), blk_str.size()) != 0) {
   //     //    spdlog::get("stderr")->warn("[Main - pthread_cond_destroy] failed writing to miner");
   //     //}
   // }
}

void error_cb(struct bufferevent *bev, short events, void *aux)
{
    if (events & BEV_EVENT_CONNECTED) {
        spdlog::get("network")->debug("[Main - err_cb] connected to miner/main");
    } 
    else if (events & BEV_EVENT_ERROR) {
        spdlog::get("stderr")->warn("[Main - err_cb]  error");
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
    Client *client = static_cast<Client*>(arg);
    if (what & BEV_EVENT_EOF) {
        /*  Client disconnected, remove the read event and the
         *   * free the client structure. */
        spdlog::get("network") -> debug("[Main - buf_err_cb] Client disconnected.");
    }
    else {
        spdlog::get("stderr") -> warn("Client socket error, disconnecting");
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
    XState * state = SYS_STATE;
    Client* client;
    auto err = spdlog::get("stderr");
    auto console = spdlog::get("console");
    auto nwk_log = spdlog::get("network");

    /* Create Client Instance */
    client = (Client *) calloc(1, sizeof(Client));
    if (client == NULL) {
        err->warn("err: client calloc failed\n");
        exit(1);
    }

    /*  Networking accepting new client  */
    socklen_t client_len = sizeof(client_addr);

    client_fd = accept(socket, (struct sockaddr *)&client_addr, &client_len);
    client->server_ip_ = client_addr.sin_addr.s_addr;

    char addr_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(client_addr.sin_addr), addr_ip, INET_ADDRSTRLEN);
    nwk_log->info("[Main - on_accept] Accepted : {}", addr_ip);

    if (client_fd < 0) {
        err->warn("err: client_fd < 0\n");
        return;
    }

    if (setnonblock(client_fd) < 0) {
         err->warn("err: setnonblock failed");
    }
    
    client->sd_ = client_fd;
    client->buf_ev_ = bufferevent_socket_new(state->evbase_, client_fd, 0);

    bufferevent_setcb(client->buf_ev_, buf_read_cb, NULL, buf_err_cb, client);
    bufferevent_enable(client->buf_ev_, EV_READ | EV_WRITE);

    state->add_client_in(client);
}

int 
start_server()
{
    /*  Record Peer */
    //state->peer_name_ = server_host_name;
    auto state = SYS_STATE;
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
    spdlog::get("network") -> info("[MAIN - start_server] Waiting for connection");
    return 0;
}

void
miner_on_read(struct bufferevent *bev, void *arg)
{
    size_t n;
    stringstream ss;
    MinerState* s = static_cast<MinerState*>(arg);
    auto console = spdlog::get("console");
    auto err = spdlog::get("stderr");

    for(;;) {
        char data[128] = {0};
        n = bufferevent_read(bev, data, sizeof(data));
        if(n<=0)
            break;
        

        ss << data;
        auto block = Block::deserialize(ss);
        ss.clear();

        console->debug("[MINER - miner_on_read] Block: {}" , block.to_string());

        if(block.process_block()) {
            /*  Reset the mining */
            console->info("[Miner - miner_on_read] PROCESS BLOCK OK : {}", block.to_string());
            s->reset_mining();
        } else {
            /*  FIXME: What to do  */
            err->warn("[Miner - miner_on_read] Process blocked failed");
        }
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
        
        spdlog::get("network")->info("[Main - on_read] Read data: {}", data); 
            
        /*  Broadcast to peers */
        ss << data;
        state->broadcast_block(ss.str());
        state->miner_state_->reset_mining();
    }
}


void 
on_mine(int fd, short events, void* aux) 
{
    auto console = spdlog::get("console");
    //XState* state = static_cast<XState*>(aux);
    MinerState *my_state = static_cast<MinerState*>(aux);
    string data;
    
    //FIXME: synchronize and report nonce
    /*  Create new block */
    auto blk_idx = BlockIndex::get_best_blkidx();
    assert(blk_idx != NULL);
    int nonce = rand() % 100;
    console->debug("[Miner - on_mine] USING NONCE : {}", nonce);
    auto new_block = new Block(blk_idx,nonce, my_state->name_);
    
    /*  Add new block */
    new_block->accept_block();
    //debug_chains();
    console->info("[Miner - on_mine] NEW BLOCK: {}" , new_block->to_string());
    
    /*  Serialize the block for sending */
    data = new_block->serialize();

    if(bufferevent_write(my_state->w_bev_, data.c_str(), data.size()) != 0) {
        spdlog::get("stderr")->warn("[MIner - on_mine]  failed send to main");
    }
}

void *
mine(void* aux) 
{
    auto console = spdlog::get("console");
    XState* state = SYS_STATE;
    MinerState* my_state = state->miner_state_;

    console->info("[Miner - mine] Miner running\n"); 
    struct timespec ts;
    struct timeval tp;
    
    /*  Setup MIning Event */
    //struct timeval tv { my_state->time_, 0};
    double num = my_state->dis_(my_state->engine_);
    console->debug("[Miner - mine] Miner next value {}", num);
    struct timeval tv { (int)num, 0};
    struct event *mine_ev = event_new(my_state->evbase_, -1, EV_PERSIST  , on_mine, my_state);
    evtimer_add(mine_ev, &tv);
    my_state->mine_ev_ = mine_ev;

    /* Loop until the main thread breaks the evloop */
    event_base_loop(my_state->evbase_, 0);
    
    console->info("[Miner - mine] Miner exiting\n"); 
    pthread_exit(NULL);
}


XState*
init()
{
    auto console = spdlog::get("console");
    auto nt_log = spdlog::get("network");

    /*  FIXME: check input */
    int res; 
    
    /* Init State relevant info */
    XState* state = new XState(); 
    assert(state);
    SYS_STATE = state;
    state->evbase_ = event_base_new();
    /*  Setup network info */
    state->my_port_ = SYS_CONFIG.my_port;
    state->my_addr_ = SYS_CONFIG.my_addr;
    state->msg_size_ = SYS_CONFIG.msg_size;

    
    
    /*  Init Server Instance */
    nt_log->info("[Main - init] Starting Server\n");
    res = start_server();
    check_result(res == 0, 0, "init(): init server failes\n");
    nt_log->info("[Main - init] Server Started, Looping ...\n");

    /* Connect to unknown peers */
    /* TODO : CONNECT TO ALL */
    for(auto peer_addr : SYS_CONFIG.peers) {
        auto client = state->connect_peer(peer_addr);
        check_result(client != NULL, 1, "init(): failed to connect to client"); 
        nt_log->info("[Main - init] Connected peer {}\n", client->server_host_name_);
    }

    ///*  Set up mining thread */
    //create_miner();
    
    ///* Set up the chain genesis block */
    //Block genesis = Block::genesis();
    //genesis.add_to_chain();
    //console->info("[Main - init] Genesis Block Hash: {}", genesis.get_hash());
    //debug_chains();

    /*  Set up ping */
    if(state->my_addr_ == "peer0") {
        init_ping();
    }


    /*  TODO: check failure */
    return state;
}


int main(int argc, char*argv[])
{
    /*  Settup Logger */
    auto console = spdlog::stdout_color_mt("console");
    auto err_log = spdlog::stderr_color_mt("stderr");
    auto network = spdlog::stdout_color_mt("network");
    
    console->info("Starting");
    /*  Read in arguments  */
    parse_cmd(argc, argv);



    /*  Init randome seed */
    init();
    event_base_dispatch(SYS_STATE->evbase_);

    pthread_join(SYS_STATE->miner_, NULL);
    //pthread_cond_destroy(&COND);
    //pthread_mutex_destroy(&MUTEX);
    //pthread_exit(NULL);
    return 0;
}


void 
on_kill(int fd, short events, void* aux)
{
    //XState* state = static_cast<XState*>(aux);
    XState* state = SYS_STATE;
    
    //struct timeval tv { 1,0};
    spdlog::get("console")->info("Shutting down miner");
    //G_DO_MINE = false;
    event_base_loopbreak(state->evbase_);
}

int
create_miner() 
{
    XState* state = SYS_STATE; 

    /*  Comm bev, named by Read side */
    struct bufferevent * main_bev_pair[2];  
    struct bufferevent * miner_bev_pair[2];

    /* Miner state init */
    MinerState * miner_state = new MinerState();
    miner_state->evbase_ = event_base_new();
    miner_state->time_ = SYS_CONFIG.miner_timeout; 
    miner_state->name_ = state->my_addr_;


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

    /*  Setup distribution */
    auto seed = hash<string>{}(state->my_addr_);
    miner_state->engine_ = default_random_engine(seed);
    miner_state->dis_ = exponential_distribution<double>(1/10.0);

    /*  Create the miner thread  */
    pthread_create(&state->miner_, NULL, mine, (void*) state);
    spdlog::get("console")->info("Created miner\n");
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


/*  Reset the mining sleep
 *  */
void MinerState::reset_mining()
{
    spdlog::get("console")->info("[MINER - reset_mining] RESET TIMER");

    /* Cancel timer */
    assert(mine_ev_ != NULL);
    //assert(evtimer_pending(mine_ev_, NULL));
    if (event_del(mine_ev_) != 0) {
        spdlog::get("stderr")->warn("[MINER - reset_mining] event_del faield");
    }
    
    /* Random sleep time */
    double num = dis_(engine_);
    struct timeval tv {(int)num, 0};
    spdlog::get("console")->debug("[Miner - mine] Miner next value {}", num);

    mine_ev_ = event_new(evbase_, -1, EV_PERSIST , on_mine, this);
    event_add(mine_ev_, &tv);
}

void XState::broadcast_block(string data) 
{
    auto err = spdlog::get("stderr");
   
    /*  Pad the data, so that multiple blocks in buffer can be read */
    data.append("*");
    for(auto itr = out_clients_.begin() ; itr != out_clients_.end(); ++itr) {
        auto clt = itr->second; 

        if (clt == NULL) {
            err->warn("[Main - broadcast_block] no client conencted");
            return;
        }

        if(bufferevent_write(clt->buf_ev_, data.c_str(), data.size()) != 0) {
            err->warn("[Main - broadcast_block] buffer write failed");
        } else {
            spdlog::get("network")->info("[Main - broadcast_block]: Sent to {}", itr->first);
        }
    }

}

void XState::broadcast_ping(int64_t start, int hop) 
{
    auto nwk = spdlog::get("network");
    
    /*  Prepare Ping message  */
    PingMsg msg(start, hop, this->msg_size_);
    string data = msg.save();
    nwk->info("[Main - broadcast_ping] Msg string: {}", msg.to_string());
    nwk->info("[Main - broadcast_ping] Sending Data: {}", data);
    /*  Broadcast to outgoing peers */
    for(auto itr = out_clients_.begin(); itr != out_clients_.end(); ++itr) {
        auto clt = itr->second;

        if(bufferevent_write(clt->buf_ev_, data.c_str(), data.size()) !=0) {
            nwk->warn("[Main - broadcast_ping] Failed");
        } else {
            nwk->info("[Main - broadcast_ping] Sent to {}, Hop: {}", itr->first, hop); 
        }
    }
}

Client* 
XState::connect_peer(PeerAddr& peer)
{
    Client * client = static_cast<Client*>(calloc(1, sizeof(Client)));
    check_result(client != NULL,0,"connect_peer(): calloc client failed");

    client->server_host_name_ = peer.host;
    spdlog::get("network")->info("[MAIN - connect_peer] Connected: {}", client->server_host_name_);
    /*  get address */ 
    struct addrinfo* server_info;
    int res = getaddrinfo(client->server_host_name_.c_str(), NULL, NULL, &server_info);
    if(res == -1) {
        //error
        printf("err in address\n");
        exit(1);
    }
    client->server_ip_ = ((struct sockaddr_in*) (server_info->ai_addr))->sin_addr.s_addr;

    char str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(client->server_ip_), str, INET_ADDRSTRLEN);
    spdlog::get("network")->debug("[MAIN - connect_peer] server ip: {}", str);

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
    listen_addr.sin_port=htons(peer.port);

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
    
    //TODO:BUG:
    this->add_client_out(client);
    //this->out_client_ = client;
    return client;
}

void XState::add_client_out(Client * c) {
    auto nw_log = spdlog::get("network");
    
    if(this->out_clients_.count(c->server_host_name_)) {
        nw_log->info("[Main - add_client_out] Duplicate Client {}", c->server_host_name_); 
    }
    
    /*  Overwrite with the new one */
    this->out_clients_.insert(make_pair(c->server_host_name_, c));
}

void XState::add_client_in(Client * c) {
    auto nw_log = spdlog::get("network");
    
    if(this->in_clients_.count(c->server_host_name_)) {
        nw_log->info("[Main - add_client_in] Duplicate Client {}", c->server_host_name_); 
    }
    
    /*  Overwrite with the new one */
    this->in_clients_.insert(make_pair(c->server_host_name_, c));
}

static void
parse_cmd(int argc, char* argv[]) 
{
    auto console = spdlog::get("console");
    b_po::options_description desc("Allwed Options");
    desc.add_options()
        ("input-dir", b_po::value<string>(), "input data")
        ;
    
    b_po::store(b_po::parse_command_line(argc, argv, desc), INPUT_VAR_MAP);
    b_po::notify(INPUT_VAR_MAP);

    /*  FIXME: Verify input variables */
    assert(INPUT_VAR_MAP.count("input-dir"));
    console->info("[MAIN - parse_cmd] Program Options: input-dir : {}",
            INPUT_VAR_MAP["input-dir"].as<string>());
    string input_data_dir = INPUT_VAR_MAP["input-dir"].as<string>();

    SYS_CONFIG = parse_config(input_data_dir);
    //SYS_CONFIG.print();
}



