#!/usr/bin/python
from mininet.topo import Topo
from mininet.net import Mininet
from mininet.cli import CLI
from mininet.link import TCLink
import time
#   0   1   2   3   4
#h1--r1--r2--r3--r4--h2
# 10.0.X.0
def rp_disable(host):
    ifaces = host.cmd('ls /proc/sys/net/ipv4/conf')
    ifacelist = ifaces.split()    # default is to split on whitespace
    for iface in ifacelist:
       if iface != 'lo': host.cmd('sysctl net.ipv4.conf.' + iface + '.rp_filter=0')

max_queue_size = 20
net = Mininet( cleanup=True )
h1 = net.addHost('h1',ip='10.0.0.1')
r1 = net.addHost('r1',ip='10.0.0.2')
r2 = net.addHost('r2',ip='10.0.1.2')
r3 = net.addHost('r3',ip='10.0.2.2')
r4 = net.addHost('r4',ip='10.0.3.2')
h2 = net.addHost('h2',ip='10.0.4.2')
c0 = net.addController( 'c0' )

net.addLink(h1,r1,intfName1='h1-eth0',intfName2='r1-eth0',cls=TCLink , bw=5, delay='20ms', max_queue_size=10*max_queue_size)
net.addLink(r1,r2,intfName1='r1-eth1',intfName2='r2-eth0',cls=TCLink , bw=5, delay='20ms', max_queue_size=10*max_queue_size)
net.addLink(r2,r3,intfName1='r2-eth1',intfName2='r3-eth0',cls=TCLink , bw=2, delay='10ms', max_queue_size=max_queue_size)
net.addLink(r3,r4,intfName1='r3-eth1',intfName2='r4-eth0',cls=TCLink , bw=5, delay='20ms', max_queue_size=10*max_queue_size)
net.addLink(r4,h2,intfName1='r4-eth1',intfName2='h2-eth0',cls=TCLink , bw=5, delay='20ms', max_queue_size=10*max_queue_size)
net.build()

h1.setIP('10.0.0.1', intf='h1-eth0')
h1.cmd("ifconfig h1-eth0 10.0.0.1 netmask 255.255.255.0")
h1.cmd("route add default gw 10.0.0.2 dev h1-eth0")

r1.cmd("ifconfig r1-eth0 10.0.0.2/24")
r1.cmd("ifconfig r1-eth1 10.0.1.1/24")
r1.cmd("ip route add to 10.0.4.0/24 via 10.0.1.2")
r2.cmd("ip route add to 10.0.0.0/24 via 10.0.0.1")
r1.cmd('sysctl net.ipv4.ip_forward=1')
rp_disable(r1)

r2.cmd("ifconfig r2-eth0 10.0.1.2/24")
r2.cmd("ifconfig r2-eth1 10.0.2.1/24")
r2.cmd("ip route add to 10.0.4.0/24 via 10.0.2.2")
r2.cmd("ip route add to 10.0.0.0/24 via 10.0.1.1")
r2.cmd('sysctl net.ipv4.ip_forward=1')
rp_disable(r2)

r3.cmd("ifconfig r3-eth0 10.0.2.2/24")
r3.cmd("ifconfig r3-eth1 10.0.3.1/24")
r3.cmd("ip route add to 10.0.4.0/24 via 10.0.3.2")
r3.cmd("ip route add to 10.0.0.0/24 via 10.0.2.1")
r3.cmd('sysctl net.ipv4.ip_forward=1')
rp_disable(r3)

r4.cmd("ifconfig r4-eth0 10.0.3.2/24")
r4.cmd("ifconfig r4-eth1 10.0.4.1/24")
r4.cmd("ip route add to 10.0.4.0/24 via 10.0.4.2")
r4.cmd("ip route add to 10.0.0.0/24 via 10.0.3.1")
r4.cmd('sysctl net.ipv4.ip_forward=1')
rp_disable(r4)

h2.setIP('10.0.4.2', intf='h2-eth0')
h2.cmd("ifconfig h2-eth0 10.0.4.2 netmask 255.255.255.0")
h2.cmd("route add default gw 10.0.4.1")

net.start()
time.sleep(1)
CLI(net)
net.stop()
