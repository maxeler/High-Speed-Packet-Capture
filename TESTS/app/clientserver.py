from subprocess import check_call, call, Popen, PIPE, CalledProcessError
import unittest
import time
from scapy.all import IP, ICMP, sendp
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
        self.processes = []

        # setup env
        env = make_sim_env(SIM_NAME)
        check_call(list_extend(SIM_ARGS, ['restart']), env=env, stdout=DEV_NULL, stderr=DEV_NULL)

        # give some time for setup
        time.sleep(1)

        # add server ips to sim tap
        iface = lookup_interface_from_addr(DFE_SERVER_PORT_IP)
        for group in SERVER_IPS.keys():
            for i in range(len(SERVER_IPS[group])):
                ip = SERVER_IPS[group][i]
                check_call(['ip', 'addr', 'add', ip, 'broadcast', SERVER_NETMASK, 'dev', iface])

        self.env = env
        self.server_iface = iface

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
                server = Popen([SERVER_APP, ip, cfile])#, stdout=DEV_NULL, stderr=DEV_NULL)
                servers.append(server)
                self.processes.append(server)

        # start client
        args = [CLIENT_APP, DFE_IP, DFE_NETMASK]
        for group in SERVER_IPS:
            for i in range(len(SERVER_IPS[group])):
                ip = SERVER_IPS[group][i]
                args.extend(['-r%s' % group, ip])
        client = Popen(args, env=self.env)#, stdout=DEV_NULL, stderr=DEV_NULL)
        self.processes.append(client)

        time.sleep(20)

        # send packets
        send_group_data = {}
        # broadcast
        send_data = []
        for i in range(0, 255):
            packet = IP(dst="255.255.255.255")/ICMP()
            sendp(packet, iface=self.server_iface, verbose=False)
            send_data.append(self._makeData(packet))
        # non-broadcast
        send_group_data['A'] = send_data
        TODO: make sure sending broadcast data
        TODO: send non broadcast data

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
                print "cfile: ", cfile
                # append packets
                data = receive_group_data.get(group, [])
                data.extend(rdpcap(cfile))
                receive_group_data[group] = data

                print "group: %s (%d)" % (group, len(data))

        # recv_data = sorted([self._makeData(p) for p in pcap])
        # send_data = sorted(send_data)

        # i = 0
        # for datum in send_data:
        #     # import base64
        #     #print "%d: %s" % (i, base64.b64encode(datum))
        #     # import binascii
        #     # print "%d: %s" % (i, binascii.hexlify(datum))
        #     found = False
        #     while i<len(recv_data):
        #         # note: maxethsimd is unfortuantely padding some data with zeros
        #         # check for datum
        #         recv_datum = recv_data[i]
        #         if recv_datum[:len(datum)] == datum:
        #             # check for '0' padding
        #             j = len(datum)
        #             while j<len(recv_datum) and recv_datum[j] == chr(0):
        #                 j += 1
        #             if j == len(recv_datum):
        #                 found = True
        #                 break
        #         i += 1
        #     self.assertTrue(found)



def make_suite():
    ts = unittest.TestSuite()
    ts.addTest(unittest.makeSuite(TestCapture))

    return ts

suite = make_suite()
        
if __name__ == "__main__":
    unittest.main()

