#!/usr/bin/python
from mininet.topo import Topo
from mininet.net import Mininet
from mininet.cli import CLI
from mininet.link import TCLink
import time
##https://serverfault.com/questions/417885/configure-gateway-for-two-nics-through-static-routeing
#    ____h2____
#   /          \ 2.2
# h1           h3
#   \___h4_____/4.2
#  
net = Mininet( cleanup=True )
h1 = net.addHost('h1',ip='10.0.1.1')
h2 = net.addHost('h2',ip='10.0.1.2')
h3 = net.addHost('h3',ip='10.0.2.2')
h4 = net.addHost('h4',ip='10.0.3.2')
c0 = net.addController( 'c0' )
max_queue_size = 20
net.addLink(h1,h2,intfName1='h1-eth0',intfName2='h2-eth0',cls=TCLink , bw=5, delay='20ms', max_queue_size=10*max_queue_size)
net.addLink(h2,h3,intfName1='h2-eth1',intfName2='h3-eth0',cls=TCLink , bw=2, delay='20ms', max_queue_size=max_queue_size)
net.addLink(h1,h4,intfName1='h1-eth1',intfName2='h4-eth0',cls=TCLink , bw=2, delay='20ms', max_queue_size=max_queue_size)
net.addLink(h4,h3,intfName1='h4-eth1',intfName2='h3-eth1',cls=TCLink , bw=5, delay='20ms', max_queue_size=10*max_queue_size)
net.build()
h1.setIP('10.0.1.1', intf='h1-eth0')
h1.cmd("ifconfig h1-eth0 10.0.1.1 netmask 255.255.255.0")

h1.setIP('10.0.3.1', intf='h1-eth1')
h1.cmd("ifconfig h1-eth1 10.0.3.1 netmask 255.255.255.0")

h1.cmd("ip route flush all proto static scope global")
h1.cmd("ip route add 10.0.1.1/24 dev h1-eth0 table 5000")
h1.cmd("ip route add default via 10.0.1.2 dev h1-eth0 table 5000")

h1.cmd("ip route add 10.0.3.1/24 dev h1-eth1 table 5001")
h1.cmd("ip route add default via 10.0.3.2 dev h1-eth1 table 5001")
h1.cmd("ip rule add from 10.0.1.1 table 5000")
h1.cmd("ip rule add from 10.0.3.1 table 5001")
h1.cmd("route add default gw 10.0.1.2  dev h1-eth0")

h2.setIP('10.0.1.2', intf='h2-eth0')
h2.setIP('10.0.2.1', intf='h2-eth1')
h2.cmd("ifconfig h2-eth0 10.0.1.2/24")
h2.cmd("ifconfig h2-eth1 10.0.2.1/24")
h2.cmd("ip route add 10.0.2.0/24 via 10.0.2.2")
h2.cmd("ip route add 10.0.1.0/24 via 10.0.1.1")
h2.cmd("echo 1 > /proc/sys/net/ipv4/ip_forward")


h4.setIP('10.0.3.2', intf='h4-eth0')
h4.setIP('10.0.4.1', intf='h4-eth1')
h4.cmd("ifconfig h4-eth0 10.0.3.2/24")
h4.cmd("ifconfig h4-eth1 10.0.4.1/24")
h4.cmd("ip route add 10.0.4.0  dev h4-eth1") #via 10.0.4.2
h4.cmd("ip route add 10.0.3.0 via 10.0.3.1")

h4.cmd("echo 1 > /proc/sys/net/ipv4/ip_forward")


h3.setIP('10.0.2.2', intf='h3-eth0')
h3.cmd("ifconfig h3-eth0 10.0.2.2 netmask 255.255.255.0")
h3.setIP('10.0.4.2', intf='h3-eth1')
h3.cmd("ifconfig h3-eth1 10.0.4.2 netmask 255.255.255.0")

h3.cmd("ip route flush all proto static scope global")
h3.cmd("ip route add 10.0.2.2/24 dev h3-eth0 table 5000")
h3.cmd("ip route add default via 10.0.2.1 dev h3-eth0 table 5000")

h3.cmd("ip route add 10.0.4.2/24 dev h3-eth1 table 5001")
h3.cmd("ip route add default via 10.0.4.1 dev h3-eth1 table 5001")
h3.cmd("ip rule add from 10.0.2.2 table 5000")
h3.cmd("ip rule add from 10.0.4.2 table 5001")

net.start()
time.sleep(1)
CLI(net)
net.stop()
