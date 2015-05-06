"""
Tests for ORIG
"""

from subprocess import Popen, CalledProcessError
import unittest
from pytun import TunTapDevice, IFF_TAP
import time
from scapy.all import IP, ICMP, sendp, rdpcap
from hashlib import md5

from utils import check_output, DEV_NULL, PacketRecord

APP = '../ORIG/capture'
TAP_NAME = 'tap_captest'
CAPTURE_FILE = 'capture.pcap'
PACKET_COUNT = 255


class TestArgs(unittest.TestCase):
    """
    Sanity checks application arguments.
    """

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
    """
    Sends a series of test packets, ensures they appear in the capture file, and checks that
    nothing fatal happened along the way.
    """

    def setUp(self):
        tap = TunTapDevice(name=TAP_NAME, flags=IFF_TAP)
        tap.up()
        self.tap = tap

    def tearDown(self):
        self.tap.down()
        self.tap.close()

    def testBasic(self):
        iface = self.tap.name
        record = PacketRecord()

        # start capture
        process = Popen([APP, iface, CAPTURE_FILE], stdout=DEV_NULL, stderr=DEV_NULL)

        # send packets
        for i in range(PACKET_COUNT):
            packet = IP(dst="www.google.com")/ICMP()
            sendp(packet, iface=iface, verbose=False)
            record.add_sent(packet)

        # wait for stragglers
        time.sleep(1)

        # stop capture
        process.terminate()
        # hack: send one more packet to make sure capture closes immediately
        sendp(IP(), iface=iface, verbose=False)
        process.poll()

        # verify capture file
        for packet in rdpcap(CAPTURE_FILE):
            record.add_received(packet)
        self.assertTrue(record.verify())


def make_suite():
    ts = unittest.TestSuite()
    ts.addTest(unittest.makeSuite(TestArgs))
    ts.addTest(unittest.makeSuite(TestCapture))

    return ts

suite = make_suite()

if __name__ == '__main__':
    unittest.main()
