"""
Tests for APP server.
"""

from subprocess import check_call, call, Popen, PIPE, CalledProcessError
import unittest
from pytun import TunTapDevice, IFF_TAP
import time
from scapy.all import Ether, IP, ICMP, sendp
from hashlib import md5

from utils import list_extend, check_output, make_sim_env, StreamReaderNB, DEV_NULL, \
    interface_from_addr, PacketRecord, try_safe_exits
from pcap_reader import rdpcap
from app.settings import *


class TestArgs(unittest.TestCase):
    """
    Sanity checks application arguments.
    """

    def _runArgsTest(self, args):
        try:
            check_output(list_extend([SERVER_APP], args))
            self.fail()
        except CalledProcessError as e:
            self.assertTrue('returned non-zero exit status' in str(e))
            self.assertTrue('Usage: ' in e.stderr)

    def testHelp(self):
        out, err = check_output([SERVER_APP, '--help'])
        self.assertTrue('Usage: ' in out)

    def testTooFew(self):
        self._runArgsTest([])

    def testTooMany(self):
        self._runArgsTest([SERVER_IP, CAPTURE_FILE, 'extra-arg'])


class TestCapture(unittest.TestCase):
    """
    Sends a series of test packets, ensures they appear in the capture file, and checks that
    nothing fatal happened along the way.
    """

    def setUp(self):
        self.processes = []

        # setup env
        self.env = make_sim_env(SIM_NAME)
        check_call(list_extend(SIM_ARGS, ['restart']), env=self.env, stdout=DEV_NULL, stderr=DEV_NULL)

        # give some time for setup
        time.sleep(1)

        # add server ips to sim tap
        self.server_iface = interface_from_addr(DFE_SERVER_PORT_IP)
        for group in SERVER_IPS.keys():
            for i in range(len(SERVER_IPS[group])):
                ip = SERVER_IPS[group][i]
                check_call(['ip', 'addr', 'add', ip, 'broadcast', SERVER_NETMASK, 'dev', self.server_iface])

        self.capture_iface = interface_from_addr(DFE_CAPTURE_PORT_IP)

    def tearDown(self):
        try_safe_exits(self.processes)
        for process in self.processes:
            process.wait()

        call(list_extend(SIM_ARGS, ['stop']), env=self.env, stdout=DEV_NULL, stderr=DEV_NULL)
        
    def testRemote(self):
        group_record = {}
        for group in SERVER_IPS:
            # note: ignore_zero_padding necessary until '0' padding simulation bug is resolved
            group_record[group] = PacketRecord(ignore_zero_padding=True)

        # start servers
        servers = []
        for group in SERVER_IPS:
            for i in range(len(SERVER_IPS[group])):
                ip = SERVER_IPS[group][i]
                cfile = CAPTURE_FILES[group][i]
                server = Popen([SERVER_APP, ip, cfile], stdout=DEV_NULL, stderr=DEV_NULL)
                servers.append(server)
                self.processes.append(server)

        # start client
        args = [CLIENT_APP, DFE_IP, DFE_NETMASK]
        for group in SERVER_IPS:
            for i in range(len(SERVER_IPS[group])):
                ip = SERVER_IPS[group][i]
                args.extend(['-r%s' % group, ip])
        client = Popen(args, env=self.env, stdout=DEV_NULL, stderr=DEV_NULL)
        self.processes.append(client)

        # send packets
        # eth broadcast
        for i in range(PACKET_COUNT):
            packet = Ether(dst='FF:FF:FF:FF:FF:FF')/IP(dst='127.0.0.2')/ICMP()
            sendp(packet, iface=self.capture_iface, verbose=False)
            group_record['A'].add_sent(packet)
        # eth non-broadcast
        send_data = []
        for i in range(PACKET_COUNT):
            packet = Ether(dst='AA:AA:AA:AA:AA:AA')/IP(dst='127.0.0.2')/ICMP()
            sendp(packet, iface=self.capture_iface, verbose=False)
            group_record['B'].add_sent(packet)

        # wait for stragglers
        time.sleep(1)

        # make sure still running
        client.poll()
        self.assertTrue(client.returncode == None)
        for server in servers:
            server.poll()
            self.assertTrue(server.returncode == None)

        # stop capture
        client.terminate()
        for server in servers:
            server.terminate()

        client.wait()
        for server in servers:
            server.wait()

        # verify capture
        for group in SERVER_IPS:
            for i in range(len(SERVER_IPS[group])):
                cfile = CAPTURE_FILES[group][i]
                for packet in rdpcap(cfile):
                    group_record[group].add_received(packet)

        for group in SERVER_IPS:
            self.assertTrue(group_record[group].verify())

def make_suite():
    ts = unittest.TestSuite()
    ts.addTest(unittest.makeSuite(TestArgs))
    # note: ignoring TestCapture until simulation bug is resolved
    #ts.addTest(unittest.makeSuite(TestCapture))

    return ts

suite = make_suite()
        
if __name__ == '__main__':
    unittest.main()
