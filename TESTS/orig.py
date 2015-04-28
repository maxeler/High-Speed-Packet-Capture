from subprocess import check_call, Popen, PIPE, CalledProcessError
import unittest
from pytun import TunTapDevice, IFF_TAP
import time
from scapy.all import IP, ICMP, sendp, rdpcap
from hashlib import md5
import os

from utils import list_extend, check_output, DEV_NULL

APP = '../ORIG/capture'
TAP_NAME = 'tap_captest'
TAP_BAD_NAME = 'BADTAPNAME' # this should not exist
CAPTURE_FILE = 'capture.pcap'


class TestArgs(unittest.TestCase):

    def _testArgsException(self, args):
        try:
            check_output(args)
            self.fail()
        except CalledProcessError as e:
            self.assertTrue('returned non-zero exit status' in str(e))
            self.assertTrue('Usage: ' in e.stderr)

    def testHelp(self):
        out, err = check_output([APP, "--help"])
        self.assertTrue("Usage: " in out)

    def testTooFew(self):
        self._testArgsException([APP])
        self._testArgsException([APP, TAP_NAME])

    def testTooMany(self):
        self._testArgsException([APP, TAP_NAME, CAPTURE_FILE, "extra-arg"])


class TestCapture(unittest.TestCase):

    def setUp(self):
        tap = TunTapDevice(name=TAP_NAME, flags=IFF_TAP)
        tap.up()
        self.tap = tap

    def tearDown(self):
        self.tap.down()
        self.tap.close()
        
    def testBasic(self):
        iface = self.tap.name

        # start capture
        process = Popen([APP, iface, CAPTURE_FILE], stdout=DEV_NULL, stderr=DEV_NULL)

        # send packets
        send_hashes = []
        for i in range(0, 255):
            packet = IP(dst="www.google.com")/ICMP()
            sendp(packet, iface=iface, verbose=False)
            hash = md5(packet.build()).digest()
            send_hashes.append(hash)

        # wait for stragglers
        time.sleep(1)

        # stop capture
        process.terminate()
        # hack: send one more packet to make sure capture closes immediately
        sendp(IP(), iface=iface, verbose=False)
        process.poll()

        # verify capture CAPTURE_FILE
        pcap = rdpcap(CAPTURE_FILE)
        recv_hashes = sorted([md5(p.build()).digest() for p in pcap])
        send_hashes = sorted(send_hashes)
        i = 0
        for hash in send_hashes:
            found = False
            while i<len(recv_hashes):
                if hash == recv_hashes[i]:
                    found = True
                    break
                i += 1
            self.assertTrue(found)


def make_suite():
    ts = unittest.TestSuite()
    ts.addTest(unittest.makeSuite(TestArgs))
    ts.addTest(unittest.makeSuite(TestCapture))

    return ts

suite = make_suite()
        
if __name__ == '__main__':
    unittest.main()

