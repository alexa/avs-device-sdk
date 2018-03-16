#!/bin/bash
PIN=15
STATE1=0
STATE2=1

####################################################################################################

_psName2Ids(){
    ps -aux | grep -i "$1" | awk '{print $2}'
}

alive(){
    _psName2Ids "startsample.sh" | grep "$1" | wc | awk '{print $1}'
}

####################################################################################################

gpio mode $PIN out
if [ "$1" == "connecting" ]
then
    gpio write $PIN $STATE1
elif [ "$1" == "finished" ]
then
    gpio write $PIN $STATE1
elif [ "$1" == "idle" ]
then
    gpio mode $PIN out
    gpio write $PIN $STATE2
    sleep 0.5
    gpio write $PIN $STATE1
elif [ "$1" == "listening" ]
then
    gpio write $PIN $STATE2
elif [ "$1" == "not-connected" ]
then
    gpio write $PIN $STATE1
elif [ "$1" == "speaking" ]
then
    for i in `seq 1 2`;
        do
		gpio write $PIN $STATE2
		sleep $(echo 0.$RANDOM)
		gpio write $PIN $STATE1
        sleep $(echo 0.$RANDOM)        
	done 
elif [ "$1" == "heartbeat" ]
then
	gpio write $PIN $STATE2
	num=$(alive)
	if [ $num -gt 1 ] 
	then
		sleep 0.005
		gpio write $PIN $STATE1
	fi
else
    sleep 0.05
fi
