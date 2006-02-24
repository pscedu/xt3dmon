#!/usr/bin/perl -W
# $Id$

use File::Basename;
use Symbol;
use strict;
use warnings;

# start config

my $login_host = "phantom.psc.edu";

my %c_fn = (
	admin => "admin",
);

my $outdir = dirname($0) . "/../data";
my $tmpsufx = ".new";
my $oldsufx = ".bak";

# end config

my (%t_fn, %f_fn, %o_fn, %fh, $t);
foreach $t (keys %c_fn) {
	$t_fn{$t} = "$outdir/$c_fn{$t}$tmpsufx";
	$f_fn{$t} = "$outdir/$c_fn{$t}";
	$o_fn{$t} = "$outdir/$c_fn{$t}$oldsufx";
	$fh{$t} = gensym;
	open $fh{$t}, "> $t_fn{$t}" or err($t_fn{$t});
}

my (@temp, $line);

open CONNFH, "ssh $login_host grep ^staff: /etc/group \\| sed -e 's/.*://' \\&\\& " .
   "awk -F: \\'{print \\\$1}\\' /etc/passwd |" or err("ssh $login_host");
while (defined($line = <CONNFH>)) {
	print { $fh{admin} } $line;
}
close CONNFH;

my $fh = "$outdir/$c_fn{admin}.local";
open FH, "<", $fh;
print { $fh{admin} } <FH>;
close FH;

foreach $t (keys %c_fn) {
	close $fh{$t};
	unlink($o_fn{$t});			# delete old backup
#	print "unlink $o_fn{$t}\n";
	rename($f_fn{$t}, $o_fn{$t});		# move current files to backup
#	print "mv $f_fn{$t} $o_fn{$t}\n";
	rename($t_fn{$t}, $f_fn{$t})		# move new to current
	    or err("rename $t_fn{$t} $f_fn{$t}");
#	print "mv $t_fn{$t} $f_fn{$t}\n";
}

sub err {
	warn "$0: ", @_, ": $!\n";
	exit 1;
}
