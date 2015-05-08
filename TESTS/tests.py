"""
Runs tests.
"""

import os

import unittest
import orig
import app


def run():
    """
    Runs ORIG and APP tests.
    """

    # create test suite
    ts = unittest.TestSuite()
    ts.addTest(orig.suite)
    ts.addTest(app.client.suite)
    ts.addTest(app.server.suite)

    # kill children if parent fails
    os.setpgrp()

	# run
    runner = unittest.TextTestRunner(verbosity=2)
    runner.run(ts)


if __name__ == '__main__':
    run()
