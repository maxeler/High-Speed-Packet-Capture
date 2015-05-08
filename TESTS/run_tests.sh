#!/bin/sh
if [ -z $MAXELEROSDIR ]
then
	echo "Error: MAXELEROSDIR is not set" >&2
	exit 1
fi

if [ $EUID -ne 0 ]
then
    echo "Error: Must be run with root permissions" >&2
    echo "Error: sudo -E $0"
    echo "Error: Some of the tests require root access in order to create a network tap." >&2
    exit 1
fi

echo "Do NOT forget to install the required python packages from requirements.txt"

python ./tests.py
