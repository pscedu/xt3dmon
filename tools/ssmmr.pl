#!/usr/bin/perl -W

# This script uses output from xtmmr to create
# a data set for xt3dmon to visualize statistics
# stored in Seastar registers.
#
# To do this, two separate MMR dumps must be made,
# the data properly formatted for xt3dmon, and
# then finally loaded into xt3dmon.  Dumps should
# be created like this:
#
#	$ ./xtmmr -A \
#	  -M rtr.012345.PERF_VC0_PKTS \
#	  -M rtr.012345.PERF_VC1_PKTS \
#	  -M rtr.012345.PERF_VC2_PKTS \
#	  -M rtr.012345.PERF_VC3_PKTS \
#	  -M rtr.012345.PERF_VC0_FLITS \
#	  -M rtr.012345.PERF_VC1_FLITS \
#	  -M rtr.012345.PERF_VC2_FLITS \
#	  -M rtr.012345.PERF_VC3_FLITS \
#	  -M rtr.012345.PERF_VC0_BLOCKED \
#	  -M rtr.012345.PERF_VC1_BLOCKED \
#	  -M rtr.012345.PERF_VC2_BLOCKED \
#	  -M rtr.012345.PERF_VC3_BLOCKED \
#	  > mmr.before
#
# Then create a corresponding mmr.after file
# in the same manner, and finally create the
# "difference" file like this:
#
#	$ perl ssmerge.pl mmr.before mmr.after > ss
#
# Then place `ss' in the data subdirectory of
# xt3dmon and give it a run.

use strict;
use warnings;

my $hwid;
my $nvals = 4 * 3;	# 3 sets (flt,pkt,blk) of 4 values (vc0..3)
my @val = (0) x $nvals;

while (<>) {
	s/\s+$//;
	next if /^System Topology Class is /;
	next if /^Reading MMR /;
	next if /^$/ or /^-+$/;

	if (/^SeaStar node name: (.*)$/) {
		dumpvals() if defined $hwid;
		$hwid = $1;
		next;
	}

	if (/^\s+(?:Seastar #\d+:\s+)?RTR_PERF_VC(\d+)_(.*?)_(\d+) = (.*)$/i) {
		unless (defined $hwid) {
			warn "$ARGV:$.: no node hardware ID found for register value\n";
			next;
		}
		my ($vc, $type, $port, $val) = ($1, $2, $3, $4);
		my $idx = makeidx($type, $vc);
		$val[$idx] = $val if $idx > 0;
	}
}
dumpvals() if defined $hwid;

sub makeidx {
	my ($type, $vc) = @_;
	my $idx = 0;

	my %types = (
		blocked	=> 0,
		flits	=> 1,
		pkts	=> 2,
	);

	return (-1) unless exists $types{lc $type};
	return ($types{lc $type} * 4 + $vc);
}

sub dumpvals {
	print $hwid, " ", join(" ", @val), "\n";
	@val = (0) x $nvals;
}

__END__

8
+ Seastar #1:	RTR_PERF_VC0_PKTS_0 = 0x70000b492d22a
+       RTR_PERF_VC0_PKTS_1 = 0xa0000de0b259c