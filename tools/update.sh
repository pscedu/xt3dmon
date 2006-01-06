#!/bin/sh
# $Id$

servroot=~/code/proj/xt3dmon
webroot=/var/www/html/xtwmon
sdb=sdb
host=phantom.psc.edu

# end config

set -e

ssh -gNL 3306:$sdb:3306 $host &
pid=$(jobs -l | awk '{print $2}')

sleep 2

cd $servroot/tools
perl dumpdb.pl

kill $pid

cd $webroot/src
perl xtwmon.pl
