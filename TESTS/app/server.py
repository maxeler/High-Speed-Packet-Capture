from subprocess import check_call, Popen, PIPE, CalledProcessError
import unittest
from pytun import TunTapDevice, IFF_TAP
import time
from scapy.all import IP, ICMP, sendp, rdpcap
from hashlib import md5
import os

APP = '../APP/server/capture_server'
SERVER_IP = '5.5.5.2'
CAPTURE_FILE = 'capture.pcap'

def list_extend(a, b):
    c = list(a)
    c.extend(b)
    return c

def check_output(run_args, *args, **kwargs):
    kwargs['stdout'] = PIPE
    kwargs['stderr'] = PIPE

    process = Popen(run_args, *args, **kwargs)
    stdout, stderr = process.communicate()

    retcode = process.poll()
    if retcode is not 0:
        exception = CalledProcessError(retcode, run_args[0])
        exception.stdout = stdout
        exception.stderr = stderr
        raise exception

    return stdout, stderr


class TestArgs(unittest.TestCase):

    def _runArgsTest(self, args):
        try:
            check_output(list_extend([APP], args))
            self.fail()
        except CalledProcessError as e:
            self.assertTrue('returned non-zero exit status' in str(e))
            self.assertTrue('Usage: ' in e.stderr)

    def testHelp(self):
        out, err = check_output([APP, '--help'])
        self.assertTrue('Usage: ' in out)

    def testTooFew(self):
        self._runArgsTest([])

    def testTooMany(self):
        self._runArgsTest([SERVER_IP, CAPTURE_FILE, 'extra-arg'])


def make_suite():
    ts = unittest.TestSuite()
    ts.addTest(unittest.makeSuite(TestArgs))

    return ts

suite = make_suite()
        
if __name__ == '__main__':
    unittest.main()
