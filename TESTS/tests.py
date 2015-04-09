import unittest
import orig
import app


if __name__ == '__main__':
    runner = unittest.TextTestRunner(verbosity=2)

    print '[ORIG]'
    runner.run(orig.suite)
    print '[APP.client.simulation]'
    runner.run(app.client.simulation.suite)
    print '[APP.server]'
    runner.run(app.client.simulation.suite)