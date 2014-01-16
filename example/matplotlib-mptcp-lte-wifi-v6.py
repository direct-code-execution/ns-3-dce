from matplotlib.pylab import *
import time
import numpy as np
import subprocess


# interaction mode needs to be turned off
ion()
 
fig = gcf()
fig.canvas.set_window_title('Multipath TCP Throughput')

while(True):                 # we'll limit ourselves to 5 seconds.
    clf()
    time.sleep (1)

    # lte
    with open('/tmp/tcp-lte.dat', 'w') as f:
        p = subprocess.Popen(['/usr/local/bin/tcpstat', '-r', 'dce-mptcp-lte-wifi-v6-4-3.pcap', '-o',  '%R %b\n', '1'], 
                             stdout=f)
        p.wait ()
    subplot(311)
    x,y = loadtxt('/tmp/tcp-lte.dat',delimiter=' ',unpack=True, 
                  dtype = {'names': ('time', 'bw'), 'formats':('i9', 'f9')})
    line, = plot(x,y/(1000*1000), 'b-',  label='LTE')
    legend (loc = 'upper left')
    ylim (ymax=10)
    ylabel ('Throughput (Mbps)')

    # wifi
    with open('/tmp/tcp-wifi.dat', 'w') as f:
        p = subprocess.Popen(['/usr/local/bin/tcpstat', '-r', 'dce-mptcp-lte-wifi-v6-2-0.pcap', '-o',  '%R %b\n', '1'], 
                             stdout=f)
        p.wait ()
    subplot(312)
    x,y = loadtxt('/tmp/tcp-wifi.dat',delimiter=' ',unpack=True, 
                  dtype = {'names': ('time', 'bw'), 'formats':('i9', 'f9')})
    line, = plot(x,y/(1000*1000), 'r-', label='Wi-Fi')
    legend (loc = 'upper left')
    ylim (ymax=10)
    ylabel ('Throughput (Mbps)')

    # aggregated
    with open('/tmp/tcp-aggr.dat', 'w') as f:
        p = subprocess.Popen(['/usr/local/bin/tcpstat', '-r', 'dce-mptcp-lte-wifi-v6-0-0.pcap', '-o',  '%R %b\n', '1'], 
                             stdout=f)
        p.wait ()
    subplot(313)
    x,y = loadtxt('/tmp/tcp-aggr.dat',delimiter=' ',unpack=True, 
                  dtype = {'names': ('time', 'bw'), 'formats':('i9', 'f9')})
    line, = plot(x,y/(1000*1000), 'g-', label='Aggregated')
    legend (loc = 'upper left')
    ylim (ymax=10)
    ylabel ('Throughput (Mbps)')

    xlabel ('Time (sec)')
    draw()   
    show()
