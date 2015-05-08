"""
Miscellaneous utility functions and classes.
"""

from subprocess import Popen, PIPE, CalledProcessError
from threading import Thread
from Queue import Queue
import os
from hashlib import md5
import time


DEV_NULL = open(os.devnull, 'w')


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


def make_sim_env(sim_name):
    env = os.environ.copy()

    maxcompiler_dir = env.get('MAXCOMPILERDIR', '')
    if not os.path.exists(maxcompiler_dir):
        raise EnvironmentError('MAXCOMPILERDIR not exist: %s' % maxcompiler_dir)

    maxeleros_sim_dir = os.path.join(maxcompiler_dir, 'lib', 'maxeleros-sim')
    sim_lib = os.path.join(maxeleros_sim_dir, 'lib', 'libmaxeleros.so')
    if not os.path.exists(sim_lib):
        raise EnvironmentError('Maxeleros Simulation Library does not exist: %s' % sim_lib)

    env['MAXELEROSDIR'] = maxeleros_sim_dir
    env['LD_PRELOAD'] = '%s:%s' % (sim_lib, env.get('LD_PRELOAD', ''))
    env['SLIC_CONF'] = '%s;use_simulation=%s' % (env.get('SLIC_CONF', ''), sim_name)

    return env


def interface_from_addr(ip):
    out, err = check_output(['ip', '-o', 'addr', 'show'])
    for line in out.split('\n'):
        if ip in line:
            return line.split(' ')[1]
    return None


def try_exit(fn):
    try:
        fn()
    except OSError as e:
        if e.errno == 3: # no such process
            # ignore already exited process
            pass
        else:
            raise e

def try_safe_exits(processes, timeout=1):
    # terminate
    for process in processes:
        try_exit(process.terminate)
    # wait
    time.sleep(timeout)
    # kill
    for process in processes:
        try_exit(process.kill)


class StreamReaderNB(object):

    def __init__(self, stream):
        self.stream = stream
        self.queue = Queue()
        self.thread_stop = False

        def service():
            while not self.thread_stop:
                line = self.stream.readline()
                if not line:
                    break
                else:
                    self.queue.put(line)
            self.stream.close()

        thread = Thread(target=service)
        thread.daemon = True
        thread.start()
        self.thread = thread

    def get_queue(self):
        return self.queue

    def stop(self, block=True):
        self.thread_stop = True
        if block:
            self.thread.join()


class PacketRecord(object):

    def __init__(self, ignore_zero_padding=False):
        self.should_hash = not ignore_zero_padding
        self.ignore_zero_padding = ignore_zero_padding
        self.sent_items = []
        self.received_items = []

    def __add(self, items, packet):
        raw = packet.build()
        if self.should_hash:
            item = md5(raw).digest()
        else:
            item = raw
        items.append(item)

    def add_sent(self, packet):
        self.__add(self.sent_items, packet)

    def add_received(self, packet):
        self.__add(self.received_items, packet)

    def verify(self):
        # TODO: switch to using a multiset for items when simulation bug is fixed
        # sort so we can do O(n) comparisons for a total cost of O(nlogn)
        received_items = sorted(self.received_items)
        sent_items = sorted(self.sent_items)

        # select check fn
        if self.ignore_zero_padding:
            def check(sent, received):
                # check for match from start and ignore remaining data if all 0s
                if sent == received[:len(sent)]:
                    # ensure remaining data is all zeros
                    j = len(sent_item)
                    while j < len(received) and 0 == ord(received[j]):
                        j += 1
                    if j == len(received):
                        return True
                return False
        else:
            def check(sent, received):
                return sent == received

        # check
        i = 0
        for sent_item in sent_items:
            found = False
            while i < len(received_items):
                received_item = received_items[i]
                if check(sent_item, received_item):
                    found = True
                    break
                i += 1
            if not found:
                return False
        return True
