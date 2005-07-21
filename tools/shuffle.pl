#!/usr/bin/perl -W
# $Id$

use strict;
use warnings;

my @lines = <>;

while (@lines) {
	my $n = int rand (1 + $#lines);
	print $lines[$n];
	$lines[$n] = $lines[$#lines];
	splice @lines, $#lines, 1, ();
}
