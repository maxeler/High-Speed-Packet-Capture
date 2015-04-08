# Tests
Running the tests will require root access.
Testing is done using the Python [unittest](https://docs.python.org/2/library/unittest.html) framework.

## Requirements
A [pip](https://pypi.python.org/pypi/pip) requirements file is included.

```console
$ pip install -r requirements.txt 
```

## All Tests
Use `run_tests.sh` to run all the tests.
This must be run with root permissions so that some of the tests can create a network tap.

``` console
$ sudo ./run_tests.sh
```

## To Do
* add APP testing
