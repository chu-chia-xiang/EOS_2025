#!/bin/sh

set -x #prints each command and its arguments to the terminal before executing it
# set -e #Exit immediately if a command exits with a non-zero status

rmmod -f mydev
insmod mydev.ko

./writer JERRY & #run in subshell

# port number要跟 ​​​​python3 seg.py <port> 對到即可
./reader 192.168.245.249 8000 /dev/mydev

