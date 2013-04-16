#!/bin/bash
echo 'Can I delete files-0 and files-1 directories ? (y/n)'
read rep
if [ $rep == 'y' ]
then
	rm -rf files-0 files-1
fi
mkdir -p files-0
mkdir -p files-1
echo copy Data files to node 0
cp erel_working.jpg mycat.tor files-0
echo copy torrent file to node 1
cp mycat.tor files-1
echo run scenario dce-bt1
../../waf --run dce-bt1
echo the file erel_working.jpg must exist in node 1
echo ls -l files-1
ls -l files-1


