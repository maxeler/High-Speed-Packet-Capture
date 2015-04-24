from subprocess import check_call, Popen, PIPE, CalledProcessError
from threading import Thread
from Queue import Queue
import os


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

    maxcompilerDir = env.get('MAXCOMPILERDIR', '')
    if not os.path.exists(maxcompilerDir):
        raise EnvironmentError('MAXCOMPILERDIR not exist: %s' % maxcompilerDir)

    maxelerosSimDir = os.path.join(maxcompilerDir, 'lib', 'maxeleros-sim')
    simLib = os.path.join(maxelerosSimDir, 'lib', 'libmaxeleros.so')
    if not os.path.exists(simLib):
        raise EnvironmentError('Maxeleros Simulation Library does not exist: %s' % simLib)

    env['MAXELEROSDIR'] = maxelerosSimDir
    env['LD_PRELOAD'] = '%s:%s' % (simLib, env.get('LD_PRELOAD', ''))
    env['SLIC_CONF'] = '%s;use_simulation=%s' % (env.get('SLIC_CONF', ''), sim_name)

    return env


def lookup_interface_from_addr(ip):
    out, err = check_output(['ip', '-o', 'addr', 'show'])
    for line in out.split('\n'):
        if ip in line:
            return line.split(' ')[1]
    return None


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

    def stop(block=True):
        self.thread_stop = True
        if block:
            self.thread.join()