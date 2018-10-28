import run_multinode as run_util
import argparse


def run_experiments(args):
    exp_cnt = 4
    config= {
            "nodenum" : [2, 10, 50, 200],
            "peercnt" : [1, 2,  3, 5],
            "msgsize" : [1, 4, 8, 16,32 ,64, 128, 256, 512, 1024, 2048],
            }

    for idx in range(0, exp_cnt):
        args.nodenum = config['nodenum'][idx]
        args.peercnt = config['peercnt'][idx]

        for msgsize in config['msgsize']:
            args.msgsize = msgsize

            # Perform 5 samples
            for i in range(0, 3):
                run_util.do_experiment(args)


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

    run_experiments(args)
