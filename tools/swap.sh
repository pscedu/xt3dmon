#!/bin/sh
# $Id$

dir=../data
doexit=0

trap clean INT

clean()
{
	doexit=1
}

while true; do
	mv $dir/nids_list_phantom $dir/nids_list_phantom.t
	mv $dir/nids_list_phantom.swap $dir/nids_list_phantom
	mv $dir/nids_list_phantom.t $dir/nids_list_phantom.swap
	if [ x"$doexit" = x"1" ]; then
		exit
	fi
	sleep 5
done
