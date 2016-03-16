# -*- coding: utf-8 -*-

import argparse
import logging
import signal
import sys
import time
from protocol import Protocol


if __name__ == "__main__":
    # Initialize argument parser + get settings
    parser = argparse.ArgumentParser(description='Robot management')
    parser.add_argument('-d,--device', required=True, type=str, dest='device', help='Serial device')
    parser.add_argument('-o,--log', required=False, type=str, dest='logfile', help='Logfile. Default is stdout.')
    args = parser.parse_args()

    # Open logfile if necessary
    log_out = sys.stdout
    if args.logfile:
        log_out = open(args.logfile, 'ab')

    # Set up the global log
    log_format = '[%(asctime)s] %(message)s'
    log_datefmt = '%d.%m.%Y %I:%M:%S'
    logging.basicConfig(stream=log_out,
                        level=logging.INFO,
                        format=log_format,
                        datefmt=log_datefmt)
    log = logging.getLogger(__name__)

    # Our own stuff
    log.info("Starting up.")
    p = Protocol(args.device)
    
    time.sleep(2)

    p.setMotor(0, 2, 0)
    p.setMotor(1, 2, 0)
    p.setServo(0, 450)
    p.setServo(1, 450)
    p.getCS()
    p.getEN()

    # Run while not ctrl-c'd
    run = True
    while run:
        try:
            p.handle()
            k = p.read()
            if k != None:
                print k
        except KeyboardInterrupt:
            run = False
            print "Caught CTRL+C, stopping ..."

    # All done, clean up.
    p.close()
    log.info("All done.")
    if args.logfile:
        log_out.close()
    exit(0)
