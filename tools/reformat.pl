#!/usr/bin/perl

my (%n, $nid, $port, $rnid, $rport, $err,
    $e_recover, $e_fatal, $e_router);

# date   time     nid        p remote nid p n recoverable
# 060222 19:02:07 c0-0c2s7s2 1 c1-0c2s7s2 1 2 uPacket Squash
# 060309 15:33:43 c2-1c0s2s0 6 *          *
while (<>) {
	next unless m{
		^
		\s*
		\d+				# date
		\s+
		\d+:\d+:\d+			# time
		\s+
	}x;

	my $rest = $';

	# Continuation lines start with lots of space.
	goto add if $rest =~ /^      /;

	next unless $rest =~ /^c/;
	(undef, undef, $nid, $port,
	    $rnid, $rport, $err) = split /\s+/, $_, 7;

	$n{$nid} = { } unless ref $n{$nid} eq "HASH";
	$n{$nid}{$port} = {
		recover	=> 0,
		fatal	=> 0,
		router	=> 0,
	} unless ref $n{$nid}{$port} eq "HASH";

add:
	$e_recover = substr $rest, 42, 22;
	$e_fatal   = substr $rest, 42 + 24, 22;
	$e_router  = substr $rest, 42 + 24 + 24;

	$n{$nid}{$port}{recover} += int($e_recover);
	$n{$nid}{$port}{fatal}++  unless $e_fatal  =~ /^\s+$/;
	$n{$nid}{$port}{router}++ unless $e_router =~ /^\s+$/;
}

my ($v, $t);
while (($nid, $v) = each %n) {
	while (($port, $t) = each %$v) {
		print "$nid\t$port\t@{$t}{qw(recover fatal router)}\n";
	}
}
