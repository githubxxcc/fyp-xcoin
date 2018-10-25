import os
import argparse
import lxml.etree as ET
from subprocess import check_output
from random import randint
from scipy.sparse import *
from scipy import *


PEER_CNT = 2

def generate_graph(node_num, peer_cnt):
    out_peer_cnt = peer_cnt

    edges = []
    rows = []
    cols = []

    for node in range(node_num):
        # Include yourself
        peers = [node]
        for x in range(out_peer_cnt):
            rows.append(node)

            # Find a peer other than self and existing peers
            while True:
                peer = randint(0, node_num-1)
                if peer not in peers:
                    break

            cols.append(peer)
            edges.append(1)
            peers.append(peer)


    graph = csr_matrix((edges, (rows, cols)), shape=(node_num, node_num))
    return graph

def connect_graph(node_num, peer_cnt):
    x = -1
    while x != 1:
        graph = generate_graph(node_num, peer_cnt)
        x, y  = csgraph.connected_components(graph, True, 'strong')
    print(graph)
    return graph




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
def setup_multiple_node_data(node_num, graph):
    os.system("rm -rf ./data/*")
    BASE_PORT = 50000
    for i in range(node_num):
        peers = graph[i, :].nonzero()[1]
        file = open("./data/peer%d.toml" % i, "w")
        file.write('my-addr = "peer%d"\n' % i)
        file.write('my-port = %d\n' % (BASE_PORT+i))
        file.write('miner-timeout = %d\n\n'% (30))

        # Write peers information
        for peer in peers:
            file.write('[[peers]]\n')
            file.write('peer-host = "peer%d"\n' % peer)
            file.write('peer-port = %d\n' % (BASE_PORT + peer))
        file.close()

def run_shadow_bitcoin_multiple_node(node_num, worker_num):
    os.system("shadow -l warning %s" % ("./example_multiple_generated.xml"))

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Script for generating shadow config xml and running shadow experiments.' )
    parser.add_argument("--nodenum", type=int, help="Number of bitcoin nodes for experiment.")
    parser.add_argument("--workernum", type=int, help="Number of shadow workers for the simulation. Multiple worker can accelerate the speed of the simulation.")

    args = parser.parse_args()
    if args.nodenum == None:
        args.nodenum = 4
    if args.workernum == None:
        args.workernum = 1

    graph = connect_graph(args.nodenum, PEER_CNT)
    setup_multiple_node_xml(args.nodenum)
    setup_multiple_node_data(args.nodenum, graph)
    run_shadow_bitcoin_multiple_node(args.nodenum, args.workernum)

    print(graph)


