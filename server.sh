#!/bin/sh
# $Id$

if pgrep -u yanovich xt3dmon >/dev/null 2>&1; then
	echo "xt3dmon server already running"
	exit 1
fi

if [ X$(id -un) != X"yanovich" ]; then
	echo "run as user yanovich"
	exit 1
fi

cd /home/yanovich/code/proj/xt3dmon
XAUTHORITY=/var/gdm/:0.Xauth ./xt3dmon -display :0 -d 2>&1 | mail -s "xt3dmon down" yanovich@psc.edu
