#!/bin/bash

sudo /etc/init.d/network-manager stop

#############################################
#sudo ip rule add table hip0 pref 200
#sudo ip rule add table eth0 pref 201
#sudo ip rule add table wlan0 pref 202
#sudo ip rule add table ppp0 pref 203
#sudo ip rule add table sine pref 204
#sudo ip route add table sine default dev eth0
#############################################

# Turn down ethernet :- Hence escape route is blocked
sudo ifconfig eth0 down
#sudo ip route del default dev eth0

# Bring Up Wifi
sudo ifconfig wlan0 down
sudo iwconfig wlan0 mode Ad-Hoc essid "mip6test" channel 1
# Alternate Hotspots
# sudo iwconfig wlan0 essid "IRT SINE" channel 10
# sudo iwconfig wlan0 essid "Columbia University" channel 6
sudo dhclient wlan0

# Remove Default Route added by dhclient
sudo ip route del default dev wlan0

# Add route to Shiboleth idp server & (forgot the name of server)
# Also, add route to Smart Router Socks Proxy 
# via wlan0
sudo route add 128.59.20.83 gw 192.168.1.1 dev wlan0
sudo route add 128.59.20.188 gw 192.168.1.1 dev wlan0

# Prepare ppp0 :- LTE (Sorry! "The LTE")
sudo wvdial Verizon

# Pray to God!!!

# Sample Table Entry :- 
# sudo route -n
# Kernel IP routing table
# Destination     Gateway         Genmask         Flags Metric Ref    Use Iface
# 10.64.64.64     0.0.0.0         255.255.255.255 UH    0      0        0 ppp0
# 128.59.20.83    192.168.1.1     255.255.255.255 UGH   0      0        0 wlan0
# 128.59.20.188   192.168.1.1     255.255.255.255 UGH   0      0        0 wlan0
# 192.168.1.0     0.0.0.0         255.255.255.0   U     0      0        0 wlan0
# 1.0.0.0         0.0.0.0         255.0.0.0       U     0      0        0 hip0

# All the following tables should be empty
# sudo ip route show table wlan0
# sudo ip route show table eth0
# sudo ip route show table ppp0
# sudo ip route show table sine

# If you want to delete an entry use :-
# sudo ip route del 128.59.20.83 dev wlan0

# If you want to flush a table
# sudo ip route flush table wlan0
# sudo ip route flush table sine
# sudo ip route flush table eth0
# sudo ip route flush table ppp0

# When hip sets an interface it would modify the corresponding table 
# Sample table entry will look like below 
# sudo ip route show table wlan0
# 128.59.20.0/24 dev wlan0  scope link  src 192.168.1.2 

# BEST 
# IRT SINE ALPHA TEAM :- Yan Sabari Aman Kshitij Abhijeet
