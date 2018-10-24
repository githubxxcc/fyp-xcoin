import os
import argparse
import lxml.etree as ET
from subprocess import check_output

def setup_multiple_node_xml(node_num):
    baseline_xml = "./example.xml"
    new_xml = "./example_multiple_generated.xml"

    parser = ET.XMLParser(remove_blank_text=True, strip_cdata=False)

    tree = ET.parse(baseline_xml, parser)

    shadow = tree.getroot()

    for node in shadow.findall('host'):
        shadow.remove(node)

    for i in range(node_num):
        node_id = "peer%d" % (i)
        node = ET.SubElement(shadow, "host", id=node_id)
        ET.SubElement(node, "process", plugin="xcoin", starttime="1", arguments="--input-dir=./data/peer%d.toml" % i )

    tree.write(new_xml, pretty_print=True)

# Setup up TOML
def setup_multiple_node_data(node_num):
    os.system("rm -rf ./data/*")
    BASE_PORT = 50000
    for i in range(node_num):
        file = open("./data/peer%d.toml" % i, "w")
        file.write('my-addr = "peer%d"\n' % i)
        file.write('my-port = %d\n' % (BASE_PORT+i))
        file.write('miner-timeout = %d\n\n'% (30))

        # Write peers information
        file.write('[[peers]]\n')
        file.write('peer-host = "peer%d"\n' % ((i+1) % node_num))
        file.write('peer-port = %d\n' % (BASE_PORT + (i+1)%node_num))

        file.write('[[peers]]\n')
        file.write('peer-host = "peer%d"\n' % ((i+2) % node_num))
        file.write('peer-port = %d\n' % (BASE_PORT + (i+2)%node_num))

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

    setup_multiple_node_xml(args.nodenum)
    setup_multiple_node_data(args.nodenum)
    run_shadow_bitcoin_multiple_node(args.nodenum, args.workernum)


