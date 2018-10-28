import os
import argparse
import lxml.etree as ET
import numpy as np
from subprocess import check_output
from random import randint
from scipy.sparse import *
from scipy import *



# Generate graph
def generate_graph(node_num, peer_num, const_in_deg=False):
    out_peer_cnt =peer_num

    edges = []
    rows = []
    cols = []
    peer_cnt = np.zeros((node_num, ), dtype=int)

    for node in range(node_num):
        # Exclude yourself
        node_set = range(0, node_num)
        node_set.remove(node)
        for x in range(out_peer_cnt):
            # FIXME: this is for keeping the in_degree const
            # Find a peer other than self and existing peers
           #  while True:
           #      peer = random.choice(node_set)
           #      if not const_in_deg or peer_cnt[peer] < out_peer_cnt:
           #          break
           #      node_set.remove(peer)

            to_peer = random.choice(node_set)
            peer_cnt[to_peer]+=1
            rows.append(node)
            cols.append(to_peer)
            edges.append(1)
            node_set.remove(to_peer)

    graph = csr_matrix((edges, (rows, cols)), shape=(node_num, node_num))
    # print(graph)
    return graph

def connect_graph(node_num, peer_cnt, const_out_deg=True):
    graph = generate_graph(node_num, peer_cnt)
    num_cmpnt, cmpnts  = csgraph.connected_components(graph, True, 'strong')
    # print(graph)
    temp_graph = graph.tolil(False)
    while num_cmpnt != 1:
        # Union set of nods in set 0
        base_set = np.where(cmpnts == 0)[0]
        for c_id in range(1, num_cmpnt):
            # ndoes = component with c_id
            nodes = np.where(cmpnts == c_id)[0]
            for node in nodes:
                # Set one of node's peer to 0 => maintain the out degree
                if const_out_deg:
                    node_peers = temp_graph.getrow(node).nonzero()[1]
                    peer = random.choice(node_peers)
                    temp_graph[node, peer] = 0

                # one of the node in 0
                v = random.choice(base_set)
                edges = temp_graph.getrow(v)
                us = edges.nonzero()[1]
                u = random.choice(us)

                # Add node into the 0 set
                temp_graph[v,u] = 0
                temp_graph[v,node] = 1
                temp_graph[node, u] = 1

        num_cmpnt, cmpnts  = csgraph.connected_components(temp_graph, True, 'strong')
    #print(temp_graph.todense())
    return temp_graph


def parse_log(node_id, latency, hop):
    log_file = "./shadow.data/hosts/peer%d/stdout-peer%d.xcoin.1000.log" % (node_id, node_id)
    data_lines = [line.rstrip('\n')[line.find("****"):] for line in open(log_file) if line.find("****") != -1 ]
    data_str = [line.strip('****') for line in data_lines][0]
    data_arr = data_str.split(',')
    latency.append(int(data_arr[0].strip()) / 1000)
    hop.append(int(data_arr[1].strip()))



def report_stat(graph, node_num, args):
    dis, y = csgraph.dijkstra(graph, return_predecessors=True, unweighted=True)
    avg_dis = dis.sum() / (node_num * (node_num-1))
    num_edges = graph.count_nonzero()

    # Avg_dis, Max_dis
    print("%f, %d, %d"
            % ( avg_dis,
                dis.max(),
                num_edges
                ))


    # Walk through all the log files
    latency=[]
    hop=[]
    for node_id in range(1, node_num):
        parse_log(node_id, latency, hop)
    print("Avg Latency: %f" % np.mean(latency))
    print("Avg Hop : %f" % np.mean(hop))

    avg_latency = np.mean(latency)
    avg_hop = np.mean(hop)
    # Write Data to Benchmark file
    out_file = "./benchmark/latency.csv"
    with open(out_file, "a+") as f :
        # msgsize, avg_dis, avg_latency, avg_hop,
        f.write("%d,%d,%d,%f,%f,%f,\n"
                % (args.nodenum, args.peercnt, args.msgsize, avg_dis,avg_latency, avg_hop))
        print("%d,%d, %d, %f, %f, %f,"
                % (args.nodenum, args.peercnt, args.msgsize, avg_dis,avg_latency, avg_hop))
        f.close()


def setup_multiple_node_xml(node_num):
    baseline_xml = "./example.xml"
    new_xml = "./example_multiple_generated.xml"

    parser = ET.XMLParser(remove_blank_text=True, strip_cdata=False)

    tree = ET.parse(baseline_xml, parser)

    shadow = tree.getroot()

    for node in shadow.findall('host'):
        shadow.remove(node)

    #  Is Clean XML
    for i in range(node_num):
        node_id = "peer%d" % (i)
        node = ET.SubElement(shadow, "host", id=node_id)
        ET.SubElement(node, "process", plugin="xcoin", starttime="1", arguments="--input-dir=./data/peer%d.toml" % i )

    tree.write(new_xml, pretty_print=True)

# Setup up TOML
def setup_multiple_node_data(node_num, graph, args):
    os.system("rm -rf ./data/*")
    BASE_PORT = 50000
    for i in range(node_num):
        peers = graph[i, :].nonzero()[1]
        file = open("./data/peer%d.toml" % i, "w")
        file.write('my-addr = "peer%d"\n' % i)
        file.write('my-port = %d\n' % (BASE_PORT+i))
        file.write("msg-size = %d\n" % args.msgsize)
        file.write('log-level = %d\n' % args.log)
        file.write('miner-timeout = %d\n\n'% (30))

        # Write peers information
        for peer in peers:
            file.write('[[peers]]\n')
            file.write('peer-host = "peer%d"\n' % peer)
            file.write('peer-port = %d\n' % (BASE_PORT + peer))
        file.close()

def do_experiment(args):
    graph = connect_graph(args.nodenum, args.peercnt, const_out_deg=True)
    setup_multiple_node_xml(args.nodenum)
    setup_multiple_node_data(args.nodenum, graph, args)
    run_shadow_bitcoin_multiple_node(args.nodenum, args.workernum)
    print(graph)
    report_stat(graph, args.nodenum, args)


def run_shadow_bitcoin_multiple_node(node_num, worker_num):
    os.system("shadow -l critical --interface-buffer=2097152 --socket-recv-buffer=2097152 --socket-send-buffer=2097152 %s" % ("./example_multiple_generated.xml"))

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Script for generating shadow config xml and running shadow experiments.' )
    parser.add_argument("--nodenum", type=int, help="Number of bitcoin nodes for experiment.")
    parser.add_argument("--msgsize", type=int, help="Size of msg.")
    parser.add_argument("--log", type=int, help="Log level [0 - 4] = [trace,debug,ifno,warn,critical].")
    parser.add_argument("--peercnt", type=int, help="Number of peers.")
    parser.add_argument("--workernum", type=int, help="Number of shadow workers for the simulation. Multiple worker can accelerate the speed of the simulation.")

    args = parser.parse_args()
    if args.nodenum == None:
        args.nodenum = 4
    if args.workernum == None:
        args.workernum = 1
    if args.msgsize == None:
        args.msgsize = 4
    if args.peercnt == None:
        args.peercnt = 1
    if args.log == None:
        args.log = 1

    # Prep result file
    os.system("rm -rf ./benchmark/*")
    with open("./benchmark/latency.csv", "w+") as f:
        f.write("node_num,peer_cnt,msg_size,avg_dis,avg_latency,avg_hop\n")
        f.close()

        do_experiment(args)





