#!/bin/sh
# $Id$

if pgrep -u yanovich xt3dmon >/dev/null 2>&1; then
	echo "xt3dmon server already running"
	exit 1
fi

XAUTHORITY=/var/gdm/:0.Xauth sudo -u yanovich ~/code/proj/xt3dmon/xt3dmon -display :0 -d &
