#!/usr/bin/perl -W
# $Id$

use DBI;
use File::Basename;
use Symbol;
use strict;
use warnings;

# start config

my $login_host = "tg-login.bigben.psc.teragrid.org";
my $host = "kaminari";
my $smwhost = "smw";
my $port = 3306;
my $user = "basic";
my $pass = "basic";
my $db   = "XTAdmin";
my $temp_path = "/home/crayadm/TempCheck/XT3CpuTemp*.txt"; # smw path to temperatures

# data file base names
my %c_fn = (
	node	=> "node",
	job	=> "job",
	yod	=> "yod",
);

my $outdir = dirname($0) . "/../data";
my $tmpsufx = ".new";
my $oldsufx = ".bak";

# end config

my $dbh = DBI->connect("DBI:mysql:database=$db;host=$host;port=$port", $user, $pass)
    or dberr("connect $host:$port");

my (%t_fn, %f_fn, %o_fn, %fh, $t);
foreach $t (keys %c_fn) {
	$t_fn{$t} = "$outdir/$c_fn{$t}$tmpsufx";
	$f_fn{$t} = "$outdir/$c_fn{$t}";
	$o_fn{$t} = "$outdir/$c_fn{$t}$oldsufx";
	$fh{$t} = gensym;
	open $fh{$t}, "> $t_fn{$t}" or err($t_fn{$t});
}

my (@temp, $line);

# ok, the fun part:
# run a command on the smw to get latest temp filename
# bonus points for strict
open CONNFH, "ssh $login_host \"ssh $smwhost \\\"cat \\\\\\\$(perl -Mstrict -We " .
    "our\@d=[our\@f=sort{\\\\\\\\\\\\\\\${b}cmp\\\\\\\\\\\\\\\$a}\\<$temp_path\\>xor+print+\\\\\\\\\\\\\\\$f[0]])\\\"\" |"
    or err("ssh $login_host");
while (defined($line = <CONNFH>)) {
	# cx0y0c1s3 44 46 48 48
	my ($cb, $r, $cg, $m, $n) = ($line =~ /^cx(\d+)y(\d+)c(\d+)s(\d+)((?:\s+\d+)+)\s*$/)
	    or next;
	$n =~ s/^\s+//;

	$temp[$r]		= [] unless ref $temp[$r]		eq "ARRAY";
	$temp[$r][$cb]		= [] unless ref $temp[$r][$cb]		eq "ARRAY";
	$temp[$r][$cb][$cg]	= [] unless ref $temp[$r][$cb][$cg]	eq "ARRAY";
	$temp[$r][$cb][$cg][$m] = [ split / /, $n ];
}
close CONNFH;

my $sth = $dbh->prepare(<<SQL) or dberr("preparing sql");
	SELECT
		cpu.processor_id	AS nid,
		cpu.processor_status	AS status,
		cpu.processor_type	AS type,
		cpu.cab_row		AS r,
		cpu.cab_position	AS cb,
		cpu.cage		AS cg,
		cpu.slot		AS m,
		cpu.cpu			AS n,
		cpu.x_coord		AS x,
		cpu.y_coord		AS y,
		cpu.z_coord		AS z,
		job.job_id		AS jobid,
		yod.yod_id		AS yodid
	FROM
		processor		AS cpu,
		yod			AS yod,
		yod_allocation		AS yod_map,
		partition		AS job,
		partition_allocation	AS job_map
	WHERE
		cpu.processor_id	= job_map.processor_id
	AND	job.partition_id	= job_map.partition_id
	AND	cpu.processor_id	= yod_map.processor_id
	AND	yod.yod_id		= yod_map.yod_id
SQL

my $row;
$sth->execute();
while ($row = $sth->fetchrow_hashref()) {
	my $status = $row->{status} eq "down" ? "n" :
	    ($row->{type} eq "service" ? "i" : "c");

	my $temp = $temp[$row->{r}][$row->{cb}][$row->{cg}][$row->{m}][$row->{n}];
	$temp = 0 unless $temp;

	$row->{jobid} = $row->{jobid} =~ /(\d+)/ ? $1 : 0;

	# 1	2  3 4  5 6	7 8 9	10	11	12	13	14	15
	# nid	cb r cg m n	x y z	stat	enabled	jobid	temp	yodid	nfails
	# 0	0  0 0  0 0	0 0 0	i	1	111	58	2	0

	#                     1   2  3  4  5  6   7  8  9   10  11  12  13  14  15
	printf { $fh{node} } "%d\t%d %d %d %d %d\t%d %d %d\t%s\t%d\t%d\t%d\t%d\t%d\n",
	    @$row{qw(nid r cb cg m n x y z)}, $status,
	    1, $row->{jobid}, $temp, $row->{yodid}, 0;
}
$sth->finish;

$sth = $dbh->prepare(<<SQL) or dberr("preparing sql");
	SELECT
		yod_id,
		partition_id,
		num_of_compute_processors	AS ncpus,
		command
	FROM
		yod
SQL

$sth->execute();
while ($row = $sth->fetchrow_hashref()) {
	# XXX: escape newlines in command?
	# <yodid>	<partition_id>	<ncpus>	<command>
	printf { $fh{yod} } "%d\t%d\t%d\t%s\n",
	    @$row{qw(yod_id partition_id ncpus command)};
}

$sth->finish;

$dbh->disconnect;

open CONNFH, "ssh $login_host qstat -f |" or err("ssh $login_host");
while (defined($line = <CONNFH>)) {
	print { $fh{job} } $line;
}
close CONNFH;

foreach $t (keys %c_fn) {
	close $fh{$t};
	unlink($o_fn{$t});			# delete old backup
	rename($f_fn{$t}, $o_fn{$t});		# move current files to backup
	rename($t_fn{$t}, $f_fn{$t})		# move new to current
	    or err("rename $t_fn{$t} $f_fn{$t}");
}

sub dberr {
	warn "$0: ", @_, ": $DBI::errstr\n";
	exit 1;
}

sub errx {
	warn "$0: ", @_, "\n";
	exit 1;
}

sub err {
	warn "$0: ", @_, ": $!\n";
	exit 1;
}
