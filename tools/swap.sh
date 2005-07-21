#!/bin/sh

doexit=0

trap clean INT

clean()
{
	doexit=1
}

while true; do
	mv nids_list_phantom nids_list_phantom.t
	mv nids_list_phantom2 nids_list_phantom
	mv nids_list_phantom.t nids_list_phantom2
	if [ x"$doexit" = x"1" ]; then
		exit
	fi
	sleep 5
done
