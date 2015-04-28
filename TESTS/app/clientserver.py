from subprocess import check_call, call, Popen, PIPE, CalledProcessError
import unittest
import time
from scapy.all import Ether, IP, ICMP, sendp
from hashlib import md5
import os
import signal

from utils import list_extend, check_output, make_sim_env, StreamReaderNB, DEV_NULL, lookup_interface_from_addr
from pcap_reader import rdpcap

SIM_NAME = 'SimplePacketCa'
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
CLIENT_APP = '../APP/client/RunRules/SIMULATION/binaries/capture_client'
SERVER_APP = '../APP/server/capture_server'
SERVER_IPS = {
    'A' : ['5.5.5.3', '5.5.5.4'],
    'B' : ['5.5.5.5', '5.5.5.6'],
}
SERVER_NETMASK = '255.255.255.0'
CAPTURE_FILES = {}
for key in SERVER_IPS.keys():
    CAPTURE_FILES[key] = []
    for ip in SERVER_IPS[key]:
        CAPTURE_FILES[key].append('capture_%s.pcap' % ip)
DFE_IP = '5.5.5.2'
DFE_NETMASK = '255.255.255.0'


class TestCapture(unittest.TestCase):

    # note: cannot hash until '0' padding simulation bug is resolved
    should_hash = False

    def _makeData(clazz, packet):
        raw = packet.build()
        if clazz.should_hash:
            data = md5(raw).digest()
        else:
            data = raw
        return data

    def _checkData(clazz, sent_raw, received):
        # hash received
        received_raw = []
        for packet in received:
            raw = packet.build()
            if clazz.should_hash:
                raw = md5(raw).digest()
            received_raw.append(raw)
        # sort
        received_raw = sorted(received_raw)
        sent_raw = sorted(sent_raw)
        # check
        i = 0
        for s in sent_raw:
            found = False
            while i < len(received_raw):
                r = received_raw[i]
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
        self.processes = []

        # setup env
        self.env = make_sim_env(SIM_NAME)
        check_call(list_extend(SIM_ARGS, ['restart']), env=self.env, stdout=DEV_NULL, stderr=DEV_NULL)

        # give some time for setup
        time.sleep(1)

        # add server ips to sim tap
        self.server_iface = lookup_interface_from_addr(DFE_SERVER_PORT_IP)
        for group in SERVER_IPS.keys():
            for i in range(len(SERVER_IPS[group])):
                ip = SERVER_IPS[group][i]
                check_call(['ip', 'addr', 'add', ip, 'broadcast', SERVER_NETMASK, 'dev', self.server_iface])

        self.capture_iface = lookup_interface_from_addr(DFE_CAPTURE_PORT_IP)

    def tearDown(self):
        def kill(process):
            try:
                process.terminate()
            except OSError as e:
                if e.errno == 3: # no such process
                    pass
                else:
                    raise e

        for process in self.processes:
            kill(process)
        for process in self.processes:
            process.wait()

        call(list_extend(SIM_ARGS, ['stop']), env=self.env, stdout=DEV_NULL, stderr=DEV_NULL)
        
    def testRemote(self):
        should_hash = False

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
        send_group_data = {}
        # broadcast
        send_data = []
        for i in range(0, 255):
            packet = Ether(dst='FF:FF:FF:FF:FF:FF')/IP(dst='127.0.0.2')/ICMP()
            sendp(packet, iface=self.capture_iface, verbose=False)
            send_data.append(self._makeData(packet))
        send_group_data['A'] = send_data
        # non-broadcast
        send_data = []
        for i in range(0, 255):
            packet = Ether(dst='AA:AA:AA:AA:AA:AA')/IP(dst='127.0.0.2')/ICMP()
            sendp(packet, iface=self.capture_iface, verbose=False)
            send_data.append(self._makeData(packet))
        send_group_data['B'] = send_data

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
        receive_group_data = {}
        for group in SERVER_IPS:
            for i in range(len(SERVER_IPS[group])):
                cfile = CAPTURE_FILES[group][i]
                # append packets
                data = receive_group_data.get(group, [])
                data.extend(rdpcap(cfile))
                receive_group_data[group] = data

        for group in SERVER_IPS:
            send_data = send_group_data[group]
            receive_data = receive_group_data[group]
            self.assertTrue(self._checkData(send_data, receive_data))



def make_suite():
    ts = unittest.TestSuite()
    ts.addTest(unittest.makeSuite(TestCapture))

    return ts

suite = make_suite()
        
if __name__ == '__main__':
    unittest.main()

