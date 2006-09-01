#!/bin/sh
# $Id$

servroot=~/code/proj/xt3dmon
webroot=/var/www/html/xtwmon
sdb=sdb
host=phantom.psc.edu

# end config

pid=""
errh()
{
	if [ -n "$pid" ]; then
		kill $pid
	fi
	exit 0
}

set -e
trap errh ERR

ssh -qgNL 3306:$sdb:3306 $host &
pid=$(jobs -l | awk '{print $2}')

sleep 2

cd $servroot/tools
perl dumpdb.pl

disown
newpid=$pid
pid=""
kill $newpid

cd $webroot/src
perl xtwmon.pl

if ! [ -e $servroot/data/node ]; then
	echo "data/node not there!!" >&2
fi
