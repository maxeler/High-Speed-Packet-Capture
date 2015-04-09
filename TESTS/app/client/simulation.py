import unittest
import subprocess
import os
from subprocess import check_call, Popen, PIPE, CalledProcessError
from pytun import TunTapDevice, IFF_TAP
import time
from scapy.all import IP, ICMP, sendp, rdpcap
from hashlib import md5

SIM_NAME = 'SimplePacketCa'
SIM_ARGS = ['maxcompilersim', '-n', SIM_NAME, '-c', 'ISCA', '-k', '-d', '1', '-e', 'QSFP_TOP_10G_PORT1:172.0.0.1:255.255.255.0', '-e', 'QSFP_TOP_10G_PORT2:5.5.5.1:255.255.255.0']
MAKE_DIR = '../APP/client/CPUCode'
APP = '../APP/client/RunRules/SIMULATION/binaries/PacketCaptureClient'
DFE_IP = '5.5.5.1'
DFE_NETMASK = '255.255.255.0'
CAPTURE_FILE = 'capture.pcap'
SERVER_IP = '5.5.5.2'

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

def make_sim_env():
    env = os.environ.copy()

    maxcompilerDir = env.get('MAXCOMPILERDIR', '')
    if not os.path.exists(maxcompilerDir):
        raise EnvironmentError('MAXCOMPILERDIR not exist: %s' % maxcompilerDir)

    maxelerosSimDir = os.path.join(maxcompilerDir, 'lib', 'maxeleros-sim')
    simLib = os.path.join(maxelerosSimDir, 'lib', 'libmaxeleros.so')
    if not os.path.exists(simLib):
        raise EnvironmentError('Maxeleros Simulation Library does not exist: %s' % simLib)

    env['MAXELEROSDIR'] = maxelerosSimDir
    env['LD_PRELOAD'] = '%s:%s' % (simLib, env.get('LD_PRELOAD', ''))
    env['SLIC_CONF'] = 'use_simulation=%s;%s' % (SIM_NAME, env.get('SLIC_CONF', ''))

    return env


class TestArgs(unittest.TestCase):

    def setUp(self):
        self.env = make_sim_env()

    def _run(self, args=[]):
        return check_output(list_extend([APP], args), env=self.env)

    def _runArgsTest(self, args):
        try:
            self._run(args)
            self.fail()
        except CalledProcessError as e:
            self.assertTrue('returned non-zero exit status' in str(e))
            self.assertTrue('Usage: ' in e.stderr or 'PacketCaptureClient: ' in e.stderr)

    def testHelp(self):
        out, err = self._run(['--help'])
        self.assertTrue('Usage: ' in out)

    def testTooFew(self):
        self._runArgsTest([])
        self._runArgsTest([DFE_IP])
        self._runArgsTest([DFE_IP, DFE_NETMASK])

    def testTooMany(self):
        self._runArgsTest([DFE_IP, DFE_NETMASK, '-l', CAPTURE_FILE, "extra-arg"])
        self._runArgsTest([DFE_IP, DFE_NETMASK, '-r', SERVER_IP, "extra-arg"])


def make_suite():
    ts = unittest.TestSuite()
    ts.addTest(unittest.makeSuite(TestArgs))
    return ts

suite = make_suite()
        
if __name__ == '__main__':
    unittest.main()