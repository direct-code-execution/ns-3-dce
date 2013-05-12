.. include:: replace.txt
DCE Cradle Use-case 4
=====================

This tutorial  document describes what |dce_cradle| is, how we can use it, how we extend it.

.. (from http://reproducingnetworkresearch.wordpress.com/2012/06/01/template-for-final-project-blog-posts/)

Team:
*****
 Hajime Tazaki, Frederic Urbani, Thierry Turletti

Key Result(s):
**************
.. one-sentence, easily accessible description of each result.

DCCP goodput performance in |dce_cradle| also shows similar results
with real network configuration and software on Linux.


Source(s):
**********
.. papers (appropriately referenced, and with a link), websites, videos.

The paper describing this text was submitted to Workshop on ns-3 2013 (under review).


Contacts:
*********
.. names / emails.

Hajime Tazaki (tazaki at nict.go.jp)

Introduction:
*************
.. Briefly introduce the paper, its motivation, and explain the graph you hoped to replicate.

|dce_cradle| allows us to use ns-3 native application with Linux kerne
as a network stack.  The key question is: how different/similar
|dce_cradle| behaves ? This tutorial tries to answer this question
with showing DCCP goodput performance.


Methods:
********
.. How was the experiment implemented and run?  What software and algorithms did you use?  This section might include graphs to validate that a project works as expected.

We need to prepare the following simulation codes to conduct the experiment.

* ns-3-dev
* ns-3-dce
* ns-3-linux
* |dce_cradle| `patch <https://codereview.appspot.com/download/issue6856090_7001.diff>`_
* iperf DCCP `extension <http://www.erg.abdn.ac.uk/~gerrit/dccp/apps/>`_
* Linux TC (packet loss and delay injection)

Results:
********
.. Describe your results.  Do they appear to be the same as the paper?  Assuming you could replicate the key results, what other results do you have?

The goodput performance of DCCP with |dce_cradle| also obtains
consistent performance like real Linux DCCP performance as with TCP
under the simple dumbbel topology scenario.

.. figure:: images/goodput-dccp.png
   :scale: 70 %


.. Lessons Learned: 
.. ****************
.. What did you learn?  Explain scaling limits of your experiment, any aspects of the paper that you found to be underspecified, stuff in Linux you needed to fix, and anything that would not be obvious or apparent after reading the paper.  What were your implementation experiences - what was hard, and what just worked?  Could you use Mininet HiFi as-is, or did it require changes?  Did running EC2 present any issues?

Instructions to Replicate This Experiment:
******************************************
.. Provide sufficiently detailed instructions for anyone to repeat the results, ideally in less than 15 minutes and with a single script.  Note the location of all code, along with specific version numbers (like SHA1's for git).  A custom AMI would ideally not be needed to replicate the experiment, but if this makes replication much easier, then go ahead.  We prefer if a single Ubuntu 12.04 EC2 c1.xlarge can run everything, even if you need to update the kernel with custom .deb packages.


#. Experiment on ns-3

   #. Setup ns-3-dce

      ::

      % mkdir dce-cradle-test
      % cd dce-cradle-test
      % hg clone -r 327 http://code.nsnam.org/furbani/ns-3-dce
      % ./utils/clone_and_compile_ns3_dce.sh

      for more information, see the DCE documentation.
   #. patch |dce_cradle| extension

      ::

      % cd dce-cradle-test
      % wget https://codereview.appspot.com/download/issue6856090_7001.diff
      % cd ns-3-dce
      % patch -p1 < ../issue6856090_7001.diff
      % ./waf
      % ./waf install

   #. run a script

      .. literalinclude:: dce-cradle-test.sh
	 :emphasize-lines: 20
         :linenos:
	 :lines: 1-24, 40-

#. Experiment on Linux real network

   #. prepare dumbbell topology

      ::

            node1              node5
                \              /
               node3 ======== node4
                /              \
            node2              node6

   #. configure bottleneck link

      at node3, packet loss and delay are configured with tc.

      ::

            sudo tc qdisc add dev eth0 root handle 1:0 tbf limit 15Kb buffer 10Kb/8 rate 2Mbit
      	    sudo tc qdisc add dev eth0 parent 1:0 handle 10: netem delay 100ms loss 5%
	    sudo sysctl -w net.ipv4.ip_forward=1


      at node4, packet loss and delay are configured with tc as well.

      ::

            sudo tc qdisc add dev eth0 root handle 1:0 tbf limit 15Kb buffer 10Kb/8 rate 2Mbit
      	    sudo tc qdisc add dev eth0 parent 1:0 handle 10: netem delay 100ms loss 5%
            sudo sysctl -w net.ipv4.ip_forward=1

   #. prepare iperf DCCP extension

      ::

      % wget http://www.erg.abdn.ac.uk/~gerrit/dccp/apps/iperf/zip/iperf-2.0.2_DCCP-patched-CBR-continuous.tar.bz2
      % tar xfj iperf-2.0.2_DCCP-patched-CBR-continuous.tar.bz2
      % cd iperf-2.0.2_DCCP-patched-CBR-continuous 
      % ./configure
      % make
      % cp src/iperf (somewhere_in_path)/iperf-dccp
      
      and put iperf-dccp to node1, node2, node5 and node6

   #. run a script at node1

      .. literalinclude:: iperf-dumbbell.sh
         :linenos:
	 :lines: 1-8,14-25,35-41

#. Plot the results of above experiment

   #. run a script

      You can gather the text outputs from the above experiments and plot the figure with gnuplot.
   

      .. literalinclude:: dce-cradle-gnuplot.sh
         :linenos:
	 :lines: 1-10,39-71
