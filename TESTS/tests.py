import unittest
import orig


if __name__ == '__main__':
    runner = unittest.TextTestRunner(verbosity=2)

    print '[ORIG]'
    runner.run(orig.suite)
