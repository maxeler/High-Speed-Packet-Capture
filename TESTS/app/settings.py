"""
Configuration variables for server and client tests.
"""

CLIENT_APP = '../APP/client/RunRules/SIMULATION/binaries/capture_client'
SERVER_APP = '../APP/server/capture_server'

PACKET_COUNT = 255

SERVER_IP = '5.5.5.3'
SERVER_IPS = {
    'A' : ['5.5.5.3', '5.5.5.4'],
    'B' : ['5.5.5.5', '5.5.5.6'],
}
SERVER_NETMASK = '255.255.255.0'

DFE_IP = '5.5.5.2'
DFE_NETMASK = '255.255.255.0'
DFE_CAPTURE_PORT_IP = '172.0.0.1'
DFE_CAPTURE_PORT_NETMASK = '255.255.255.0'
DFE_SERVER_PORT_IP = '5.5.5.1'
DFE_SERVER_PORT_NETMASK = '255.255.255.0'

CAPTURE_FILE = 'capture.pcap'
CAPTURE_FILES = {}
for key in SERVER_IPS.keys():
    CAPTURE_FILES[key] = []
    for ip in SERVER_IPS[key]:
        CAPTURE_FILES[key].append('capture_%s.pcap' % ip)

SIM_NAME = 'SimplePacketCa'
SIM_ARGS = ['maxcompilersim',
            '-n', SIM_NAME,
            '-c', 'ISCA',
            '-k',
            '-d', '1',
            '-e', 'QSFP_TOP_10G_PORT1:%s:%s' % (DFE_CAPTURE_PORT_IP, DFE_CAPTURE_PORT_NETMASK),
            '-e', 'QSFP_TOP_10G_PORT2:%s:%s' % (DFE_SERVER_PORT_IP, DFE_SERVER_PORT_NETMASK)]