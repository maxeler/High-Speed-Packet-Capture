import unittest
import subprocess
import os
import signal
from subprocess import check_call, call, Popen, PIPE, CalledProcessError
from pytun import TunTapDevice, IFF_TAP
import time
from scapy.all import IP, ICMP, sendp
from hashlib import md5

from utils import list_extend, check_output, make_sim_env, DEV_NULL, lookup_interface_from_addr
from pcap_reader import rdpcap

APP = '../APP/client/RunRules/SIMULATION/binaries/capture_client'
DFE_IP = '5.5.5.1'
DFE_NETMASK = '255.255.255.0'
CAPTURE_FILE = 'capture.pcap'
SERVER_IP = '5.5.5.2'
SIM_NAME = 'captest'
DFE_CAPTURE_PORT_IP = '172.0.0.1'
DFE_CAPTURE_PORT_NETMASK = '255.255.255.0'
DFE_SERVER_PORT_IP = '5.5.5.1'
DFE_SERVER_PORT_NETMASK = '255.255.255.0'
SIM_ARGS = ['maxcompilersim',
            '-n', SIM_NAME,
            '-c', 'ISCA',
            '-k',
            '-d', '1',
            '-e', 'QSFP_TOP_10G_PORT1:%s:%s' % (DFE_CAPTURE_PORT_IP, DFE_CAPTURE_PORT_NETMASK),
            '-e', 'QSFP_TOP_10G_PORT2:%s:%s' % (DFE_SERVER_PORT_IP, DFE_SERVER_PORT_NETMASK)]


class TestArgs(unittest.TestCase):

    def setUp(self):
        self.env = make_sim_env(SIM_NAME)

    def _run(self, args=[]):
        return check_output(list_extend([APP], args), env=self.env)

    def _runArgsTest(self, args):
        try:
            self._run(args)
            self.fail()
        except CalledProcessError as e:
            self.assertTrue('returned non-zero exit status' in str(e))
            self.assertTrue('Usage: ' in e.stderr or 'capture_client: ' in e.stderr)

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


class TestCapture(unittest.TestCase):

    # note: cannot hash until '0' padding bug is resolved
    should_hash = False

    def _makeData(clazz, packet):
        raw = packet.build()
        if clazz.should_hash:
            data = md5(raw).digest()
        else:
            data = raw
        return data

    def _checkData(clazz, sent, received):
        received = [r.build() for r in received]
        # hash
        if clazz.should_hash:
            received = [md5(r).digest() for r in received]
        # sort
        received = sorted(received)
        sent = sorted(sent)
        # check
        i = 0
        for s in sent:
            found = False
            while i < len(received):
                r = received[i]
                # note: maxethsimd is unfortuantely padding some data with zeros
                if s == r[:len(s)]:
                    # ensure remaining data is all zeros
                    j = len(s)
                    while j<len(r) and 0 == ord(r[j]):
                        j += 1
                    if j == len(r):
                        found = True
                        break
                i += 1
            if not found:
                return False
        return True

    def setUp(self):
        os.remove(CAPTURE_FILE)
        self.processes = []

        env = make_sim_env(SIM_NAME)
        check_call(list_extend(SIM_ARGS, ['restart']), stdout=DEV_NULL, stderr=DEV_NULL, env=env)

        # give some time for setup
        time.sleep(1)

        self.env = env
        self.iface = lookup_interface_from_addr(DFE_CAPTURE_PORT_IP)

    def tearDown(self):
        # terminate
        for process in self.processes:
            try:
                process.terminate()
            except OSError as e:
                if e.errno == 3: # no such process
                    pass
                else:
                    raise e

        # wait
        time.sleep(1)

        # kill
        for process in self.processes:
            try:
                process.kill()
            except OSError as e:
                if e.errno == 3: # no such process
                    pass
                else:
                    raise e

        for process in self.processes:
            process.wait()

        call(list_extend(SIM_ARGS, ['stop']), stdout=DEV_NULL, stderr=DEV_NULL, env=self.env)

    def testLocal(self):
        should_hash = False

        # start capture
        process = Popen([APP, DFE_IP, DFE_NETMASK, '-l', CAPTURE_FILE], env=self.env, stdout=DEV_NULL, stderr=DEV_NULL)
        self.processes.append(process)

        # send packets
        send_data = []
        for i in range(0, 255):
            packet = IP(dst="www.google.com")/ICMP()
            sendp(packet, iface=self.iface, verbose=False)
            send_data.append(self._makeData(packet))

        # wait for stragglers
        time.sleep(1)

        # make sure still running
        process.poll()
        self.assertTrue(process.returncode == None)

        # stop capture
        process.terminate()
        # hack: send one more packet to make sure capture closes immediately
        sendp(IP(), iface=self.iface, verbose=False)
        process.wait()

        # verify capture CAPTURE_FILE
        # receive_data = rdpcap(CAPTURE_FILE)
        # self.assertTrue(self._checkData(send_data, receive_data))
        pcap = rdpcap(CAPTURE_FILE)
        recv_data = sorted([self._makeData(p) for p in pcap])
        send_data = sorted(send_data)

        i = 0
        for datum in send_data:
            # import base64
            #print "%d: %s" % (i, base64.b64encode(datum))
            # import binascii
            # print "%d: %s" % (i, binascii.hexlify(datum))
            found = False
            while i<len(recv_data):
                # note: maxethsimd is unfortuantely padding some data with zeros
                # check for datum
                recv_datum = recv_data[i]
                if recv_datum[:len(datum)] == datum:
                    # check for '0' padding
                    j = len(datum)
                    while j<len(recv_datum) and recv_datum[j] == chr(0):
                        j += 1
                    if j == len(recv_datum):
                        found = True
                        break
                i += 1


def make_suite():
    ts = unittest.TestSuite()
    ts.addTest(unittest.makeSuite(TestArgs))
    ts.addTest(unittest.makeSuite(TestCapture))
    return ts

suite = make_suite()
        
if __name__ == '__main__':
    unittest.main()