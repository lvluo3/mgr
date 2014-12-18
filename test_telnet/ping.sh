#!/bin/bash
touch xxxxx

for loop in {0..10}
do
#for ip in {$1..$2}
for((ip=$1 ; ip <= $2 ; ip++))
do
	sh killping.sh &
	ping 192.168.200.$ip
done
done
