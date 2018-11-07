#!/usr/bin/env python

import run_multinode as run_util
import argparse
import time


def run_node_dis_cluster_exp(args):
    out_file = './benchmark/dis-cluster.csv'
    config= {
            "nodenum" : 300,
            "peercnt" : [2, 3,  4, 5, 6, 7, 8, 9],
            "msgsize" : 1,
            }
    with open(out_file, "w+") as f:
        f.write("node_num,peer_cnt,msg_size,avg_dis,avg_latency,avg_hop\n")
        f.close()


    for peercnt in config['peercnt']:
        args.peercnt = peercnt
        args.nodenum = config['nodenum']
        args.msgsize = config['msgsize']
        for i in range(0, 10):
            run_util.do_experiment(args, out_file)

def run_node_dis_single_exp(args):
    out_file = './benchmark/dis-single.csv'
    config= {
            "nodenum" : 200,
            "peercnt" : [4],
            "msgsize" : 1,
            }
    with open(out_file, "w+") as f:
        f.write("node_num,peer_cnt,msg_size,avg_dis,avg_latency,avg_hop\n")
        f.close()

    for peercnt in config['peercnt']:
        args.peercnt = peercnt
        args.nodenum = config['nodenum']
        args.msgsize = config['msgsize']
        for i in range(0, 100):
            run_util.do_experiment(args, out_file)


def run_latency_experiments(args):
    # avg_dis = 3.40 - 3.42
    exp_cnt = 4
    output_file = './benchmark/latency.csv'
    with open(output_file, "w+") as f:
        f.write("node_num,peer_cnt,msg_size,avg_dis,avg_latency,avg_hop\n")
        f.close()
    config= {
            "nodenum" : [20, 50, 220, 1500],
            "peercnt" : [2, 3,  5, 10],
            "msgsize" : [1, 4, 8, 16,32 ,64, 128, 256, 512, 1024],
            }

    for idx in range(0, exp_cnt):
        args.nodenum = config['nodenum'][idx]
        args.peercnt = config['peercnt'][idx]

        for msgsize in config['msgsize']:
            args.msgsize = msgsize

            # Perform 5 samples
            for i in range(0, 5):
                run_util.do_experiment(args, output_file)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Script for generating shadow config xml and running shadow experiments.' )
    parser.add_argument("--nodenum", type=int, help="Number of bitcoin nodes for experiment.")
    parser.add_argument("--msgsize", type=int, help="Size of msg.")
    parser.add_argument("--log", type=int, help="Log level [0 - 4] = [trace,debug,ifno,warn,critical].")
    parser.add_argument("--peercnt", type=int, help="Number of peers.")
    parser.add_argument("--exp", type=str, help="Experiments to run. [latency, dis, all]")
    parser.add_argument("--workernum", type=int, help="Number of shadow workers for the simulation. Multiple worker can accelerate the speed of the simulation.")

    args = parser.parse_args()
    args.nodenum = None
    args.workernum = 1
    args.msgsize = None
    args.peercnt = None
    if args.log == None:
        args.log = 3

    if args.exp == 'all':
        run_node_dis_single_exp(args)
        run_node_dis_cluster_exp(args)
        run_latency_experiments(args)
    if args.exp == "latency":
        run_latency_experiments(args)
    if args.exp == "dis-cluster":
        run_node_dis_cluster_exp(args)
    if args.exp == "dis-single":
        run_node_dis_single_exp(args)
    if args.exp == None:
        raise ValueError("no exp")

