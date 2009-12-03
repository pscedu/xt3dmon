#!/bin/sh
# $Id$

user=_xt3dmon

if pgrep -u $user xt3dmon >/dev/null 2>&1; then
	echo "xt3dmon server already running"
	exit 1
fi

xauth=$(ls -dt /var/run/gdm/auth-for-* | head -1)/database

if ! [ -r $xauth ]; then
	echo "$xauth: no read permission"
	exit 1
fi

sudo chmod g+rw $xauth
mkdir -p /tmp/xtsess
sudo chown -R $user:$user /tmp/xtsess

cd /home/yanovich/code/proj/xt3dmon
sudo -u $user env XAUTHORITY=$xauth ./xt3dmon -display :0 -d 2>&1 | \
	tee /dev/fd/2 | mail -s "xt3dmon down" yanovich@psc.edu
