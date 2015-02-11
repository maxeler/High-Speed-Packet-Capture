#!/bin/bash -e
SERVER=./capture_server
IF=tap1
IPS=(5.5.5.2 5.5.5.3)
PORT=2511
MASK=24

CAPTURE_DIR=./captures
LOG_DIR=./logs

function cleanup( )
{
	kill $(jobs -p)
}

trap cleanup SIGINT SIGTERM EXIT

mkdir $LOG_DIR
mkdir $CAPTURE_DIR

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
	unbuffer $SERVER $ip $PORT $CAPTURE_DIR/$file 2>&1 | tee $LOG_DIR/log_$ip&
done
echo "(Press enter to exit)"
read
