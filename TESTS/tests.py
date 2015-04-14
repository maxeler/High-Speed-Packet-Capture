import unittest
import orig
import app


if __name__ == '__main__':
    runner = unittest.TextTestRunner(verbosity=2)
    
    import os
    os.setpgrp()
    
    print '[ORIG]'
    runner.run(orig.suite)
    print '[APP.client.simulation]'
    runner.run(app.client.suite)
    print '[APP.server]'
    runner.run(app.server.suite)
    print '[APP.clientserver]'
    runner.run(app.clientserver.suite)