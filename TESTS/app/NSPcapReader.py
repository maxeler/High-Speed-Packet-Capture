from scapy.utils import RawPcapReader

class NSRawPcapReader(RawPcapReader):
    def __init__(self, filename):
    	try:
        	RawPcapReader.__init__(self, filename)
        except Scapy_Exception as e:
        	if 'Not a pcap capture file (bad magic)' == str(e):
        		# try parsing as ns pcap
        		print "F: ", self.f
        	else:
        		raise e

    def read_packet(self, size=MTU):
        rp = RawPcapReader.read_packet(self,size)
        if rp is None:
            return None
        s,(sec,usec,wirelen) = rp
        
        try:
            p = self.LLcls(s)
        except KeyboardInterrupt:
            raise
        except:
            if conf.debug_dissector:
                raise
            p = conf.raw_layer(s)
        p.time = sec+0.000001*usec
        return p
    def read_all(self,count=-1):
        res = RawPcapReader.read_all(self, count)
        import plist
        return plist.PacketList(res,name = os.path.basename(self.filename))
    def recv(self, size=MTU):
        return self.read_packet(size)