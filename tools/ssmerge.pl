#!/usr/bin/perl
# $Id$

use strict;
use warnings;

die "usage" unless @ARGV == 2;

open FA, "<", $ARGV[0] or die "$!\n";
open FB, "<", $ARGV[1] or die "$!\n";

my $max = (0xffffffff < 32) | 0xffffffff;
my ($lna, $lnb, $na, $nb);
for (my $lineno = 0; ; $lineno++) {
	$lna = <FA>;
	$lnb = <FB>;

	last if !defined($lna) && !defined($lnb);
	die "bad lineno: $lineno\n" if !defined($lna) || !defined($lnb);

	my @sa = split /\s+/, $lna;
	my @sb = split /\s+/, $lnb;

	die "(@sa) != (@sb), line $lineno\n" unless @sa eq @sb;
	die "$sa[0] != $sb[0], line $lineno\n" unless $sa[0] eq $sb[0];

	print $sa[0];
	for (my $i = 1; $i < @sa; $i++) {
		if ($sb[$i] < $sa[$i]) {
			print " ", $sb[$i] + ($max - $sa[$i]);
		} else {
			print " ", $sb[$i] - $sa[$i];
		}
	}
	print "\n";
}

close FA;
close FB;
