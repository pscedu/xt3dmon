#!/usr/bin/perl
# $Id$

use warnings;
use strict;

my $startaddr = '0xff804080';
my $words = 8 * 3;
my $nproc = 3;

my $NROWS = 2;
my $NCABS = 11;
my $NCAGES = 3;
my $NMODS = 8;
my $NNODES = 4;

my ($r, $cb, $cg, $m, $n, @nodes);

for ($r = 0; $r < $NROWS; $r++) {
 for ($cb = 0; $cb < $NCABS; $cb++) {
  for ($cg = 0; $cg < $NCAGES; $cg++) {
   for ($m = 0; $m < $NMODS; $m++) {
    for ($n = 0; $n < $NNODES; $n++) {
	push @nodes, "c$cb-${r}c${cg}s${m}s${n}";
    }
   }
  }
 }
}

my $chunksiz = int @nodes / $nproc;

for (my $i = 0; $i < $nproc; $i++) {
	my $pid = fork;
	die "fork: $!\n" unless defined $pid;

	probe($i) if $pid == 0;
}

1 until wait == -1;

sub probe {
	my ($chunk) = @_;

	my ($node, $out, $s);
	my $start = $chunk * $chunksiz;
	my $end = ($chunk == $nproc - 1) ?
	    @nodes : $start + $chunksiz;

	for (my $pos = $start; $pos < $end; $pos++) {
		$node = $nodes[$pos];

		# c0-0c0s0n0/ffffffe4: 0x000fd2e8
		# Received all 1 responses.
		# Got them all.
		# Done with event loop.
		$out = `xtmemio -t 250 ::$node $startaddr $words`;
		$out =~ s/.*: //gm;
		my @f = split /\s+/, $out;
		$s = $node;

		for (my $j = 0; $j < 24; $j += 2) {
			$s .= " " . ((eval($f[$j]) ) | eval($f[$j + 1]) << 32);
#			$s .= " " . $f[$j] . " " . $f[$j + 1];
		}
		print "$s\n";
		sleep 1;
	}
	exit;
}
