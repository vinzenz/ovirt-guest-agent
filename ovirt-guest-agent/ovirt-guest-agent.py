#!/usr/bin/python
#
# Copyright 2010 Red Hat, Inc. and/or its affiliates.
#
# Licensed to you under the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.  See the files README and
# LICENSE_GPL_v2 which accompany this distribution.
#

import logging, logging.config, os, signal, sys
import getopt
import ConfigParser
from GuestAgentLinux2 import LinuxVdsAgent

AGENT_CONFIG = '/etc/ovirt-guest-agent.conf'
AGENT_PIDFILE = '/run/ovirt-guest-agent.pid'

class OVirtAgentDaemon:

    def __init__(self):
        logging.config.fileConfig(AGENT_CONFIG)

    def run(self, daemon, pidfile):
        logging.info("Starting oVirt guest agent")

        config = ConfigParser.ConfigParser()
        config.read(AGENT_CONFIG)

        self.agent = LinuxVdsAgent(config)

        if daemon:
            self._daemonize()

        f = file(pidfile, "w")
        f.write("%s\n" % (os.getpid()))
        f.close()
        os.chmod(pidfile, 0x1b4) # rw-rw-r-- (664)
        
        self.register_signal_handler()
        self.agent.run()

        logging.info("oVirt guest agent is down.")

    def _daemonize(self):
        if os.getppid() == 1:
            raise RuntimeError, "already a daemon"
        pid = os.fork()
        if pid == 0:
            os.umask(0)
            os.setsid()
            os.chdir("/")
            self._reopen_file_as_null(sys.stdin)
            self._reopen_file_as_null(sys.stdout)
            self._reopen_file_as_null(sys.stderr)
        else:
            os._exit(0)

    def _reopen_file_as_null(self, oldfile):
        nullfile = file("/dev/null", "rw")
        os.dup2(nullfile.fileno(), oldfile.fileno())
        nullfile.close()

    def register_signal_handler(self):
        
        def sigterm_handler(signum, frame):
            logging.debug("Handling signal %d" % (signum))
            if signum == signal.SIGTERM:
                logging.info("Stopping oVirt guest agent")
                self.agent.stop()
 
        signal.signal(signal.SIGTERM, sigterm_handler)

def usage():
    print "Usage: %s [OPTION]..." % (sys.argv[0])
    print ""
    print "  -p, --pidfile\t\tset pid file name (default: %s)" % (AGENT_PIDFILE)
    print "  -d\t\t\trun program as a daemon."
    print "  -h, --help\t\tdisplay this help and exit."
    print ""

if __name__ == '__main__':
    try:
        try:
            opts, args = getopt.getopt(sys.argv[1:], "?hp:d", ["help", "pidfile="])
            pidfile = AGENT_PIDFILE
            daemon = False
            for opt, value in opts:
                if opt in ("-h", "-?", "--help"):
                    usage()
                    os._exit(2)
                elif opt in ("-p", "--pidfile"):
                    pidfile = value
                elif opt in ("-d"):
                    daemon = True
            agent = OVirtAgentDaemon()
            agent.run(daemon, pidfile)
        except getopt.GetoptError, err:
            print str(err)
            print "Try `%s --help' for more information." % (sys.argv[0])
            os._exit(2)
        except:
            logging.exception("Unhandled exception in oVirt guest agent!")
            sys.exit(1)
    finally:
        try:
            os.unlink(AGENT_PIDFILE)
        except:
            pass
