#!/bin/bash -e
SERVER=./capture_server
IF=tap1
IPS=(5.5.5.3 5.5.5.4 5.5.5.5)
MASK=24

CAPTURE_DIR=./captures
LOG_DIR=./logs

: ${LOG_ENABLE:=1}

function cleanup( )
{
	local jobs=$(jobs -p)
	if [ "$jobs" != "" ]
	then
		kill $jobs
	fi
}

trap cleanup SIGINT SIGTERM EXIT

mkdir -p $LOG_DIR
mkdir -p $CAPTURE_DIR

echo "Configuring interfaces"
for ip in ${IPS[@]};
do
	if ! $(ip addr show $IF | grep $ip/$MASK > /dev/null)
	then	
		sudo ip addr add $ip/$MASK dev $IF
	fi
done
ip addr show $IF | grep inet

for ip in ${IPS[@]};
do
	echo "Creating server for $ip"
	file="capture_$ip.pcap"
	if [ $LOG_ENABLE -eq 1 ]
	then
		unbuffer $SERVER $ip $PORT $CAPTURE_DIR/$file 2>&1 | tee $LOG_DIR/log_$ip&
	else
		$SERVER $ip $CAPTURE_DIR/$file&
	fi
done
echo "(Press enter to exit)"
read
