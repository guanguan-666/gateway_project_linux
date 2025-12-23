#!/bin/sh
ifconfig eth0 192.168.5.20 netmask 255.255.255.0 up
echo "IP set to 192.168.5.20"
