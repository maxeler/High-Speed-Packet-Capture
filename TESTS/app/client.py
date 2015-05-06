"""
Tests for APP client.
"""

import unittest
from subprocess import check_call, call, Popen, CalledProcessError
import time
from scapy.all import IP, ICMP, sendp
from hashlib import md5
import os

from utils import list_extend, check_output, make_sim_env, DEV_NULL, \
    interface_from_addr, PacketRecord, try_safe_exits
from app.pcap_reader import rdpcap
from app.settings import *


class TestArgs(unittest.TestCase):
    """
    Sanity checks application arguments.
    """

    def setUp(self):
        self.env = make_sim_env(SIM_NAME)

    def _run(self, args=None):
        if args is None:
            args = []
        return check_output(list_extend([CLIENT_APP], args), env=self.env)

    def _runFailTest(self, args):
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
        self._runFailTest([])
        self._runFailTest([DFE_IP])
        self._runFailTest([DFE_IP, DFE_NETMASK])

    def testTooMany(self):
        self._runFailTest([DFE_IP, DFE_NETMASK, '-l', CAPTURE_FILE, "extra-arg"])
        self._runFailTest([DFE_IP, DFE_NETMASK, '-r', SERVER_IP, "extra-arg"])


class TestCapture(unittest.TestCase):
    """
    Sends a series of test packets, ensures they appear in the capture file, and checks that
    nothing fatal happened along the way.
    """

    def setUp(self):
        os.remove(CAPTURE_FILE)
        self.processes = []

        self.env = make_sim_env(SIM_NAME)
        check_call(list_extend(SIM_ARGS, ['restart']), stdout=DEV_NULL, stderr=DEV_NULL, env=self.env)

        # give some time for setup
        time.sleep(1)

        self.iface = interface_from_addr(DFE_CAPTURE_PORT_IP)

    def tearDown(self):
        try_safe_exits(self.processes)

        for process in self.processes:
            process.wait()

        call(list_extend(SIM_ARGS, ['stop']), stdout=DEV_NULL, stderr=DEV_NULL, env=self.env)

    def testLocal(self):
        # note: ignore_zero_padding necessary until '0' padding simulation bug is resolved
        record = PacketRecord(ignore_zero_padding=True)

        # start capture
        process = Popen([CLIENT_APP, DFE_IP, DFE_NETMASK, '-l', CAPTURE_FILE], env=self.env, \
            stdout=DEV_NULL, stderr=DEV_NULL)
        self.processes.append(process)

        # send packets
        for i in range(PACKET_COUNT):
            packet = IP(dst='127.0.0.2')/ICMP()
            sendp(packet, iface=self.iface, verbose=False)
            record.add_sent(packet)

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
        for packet in rdpcap(CAPTURE_FILE):
            record.add_received(packet)
        self.assertTrue(record.verify())


def make_suite():
    ts = unittest.TestSuite()
    ts.addTest(unittest.makeSuite(TestArgs))
    # note: ignoring TestCapture until simulation bug is resolved
    #ts.addTest(unittest.makeSuite(TestCapture))
    return ts

suite = make_suite()

if __name__ == '__main__':
    unittest.main()
