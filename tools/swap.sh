#!/bin/sh
# $Id$

doexit=0

trap clean INT

clean()
{
	doexit=1
}

while true; do
	mv nids_list_phantom nids_list_phantom.t
	mv nids_list_phantom.swap nids_list_phantom
	mv nids_list_phantom.t nids_list_phantom.swap
	if [ x"$doexit" = x"1" ]; then
		exit
	fi
	sleep 5
done
