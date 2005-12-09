#!/usr/bin/perl -W
# $Id$

use DBI;
use File::Basename;
use strict;
use warnings;

# start config

my $login_host = "tg-login.bigben.psc.teragrid.org";
my $host = "kaminari";
my $port = 3306;
my $user = "basic";
my $pass = "basic";
my $db   = "XTAdmin";

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

my %t_fn = (
	node	=> "$outdir/$c_fn{node}$tmpsufx",
	job	=> "$outdir/$c_fn{job}$tmpsufx",
	yod	=> "$outdir/$c_fn{yod}$tmpsufx",
);

my %f_fn = (
	node	=> "$outdir/$c_fn{node}",
	job	=> "$outdir/$c_fn{job}",
	yod	=> "$outdir/$c_fn{yod}",
);

my %o_fn = (
	node	=> "$outdir/$c_fn{node}$oldsufx",
	job	=> "$outdir/$c_fn{job}$oldsufx",
	yod	=> "$outdir/$c_fn{yod}$oldsufx",
);

open NODEFH, "> $t_fn{node}" or err($t_fn{node});
open JOBFH,  "> $t_fn{job}"  or err($t_fn{job});
open YODFH,  "> $t_fn{yod}"  or err($t_fn{yod});

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
		nid			= job_map.processor_id
	AND	job.partition_id	= job_map.partition_id
	AND	nid			= yod_map.processor_id
	AND	yod.yod_id		= yod_map.yod_id
SQL

my $row;
while ($row = $dbh->fetchrow_hashref($sth)) {
	my $status = $row->{status} eq "down" ? "n" :
	    ($row->{type} eq "service" ? "i" : "c");

	# <nid>	c<cb>-<r>c<cg>s<m>s<n>	<x>,<y>,<z>	<stat>	<enabled>	<jobid>	<temp>	<yodid>
	# 0	c0-0c0s0s0		0,0,0		i	1		111	58	2
	printf NODEFH "%d\tc%d-%dc%ds%ds%d\t%d,%d,%d\t%s\t%d\t%d\t%d\t%d\n",
	    @$row{qw(nid cb r cg m n x y z)}, $status,
	    @$row{qw(jobid  yodid)};
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

while ($row = $dbh->fetchrow_hashref($sth)) {
	# XXX: escape newlines in command?
	# <yodid>	<partition_id>	<ncpus>	<command>
	printf YODFH "%d\t%d\t%d\t%s\n",
	    @$row{qw(yod_id partition_id ncpus command)};
}

$sth->finish;

$dbh->disconnect;

my $line;
open CONNFH, "ssh $login_host qstat -f |" or err("ssh $login_host");
while (defined($line = <CONNFH>)) {
	print JOBFH $line;
}
close CONNFH;

close NODEFH;
close JOBFH;
close YODFH;

# delete old backup
unlink($o_fn{node}) or err("unlink $o_fn{node}");
unlink($o_fn{job})  or err("unlink $o_fn{job}");
unlink($o_fn{yod})  or err("unlink $o_fn{yod}");

# move current files to backup
rename($f_fn{node}, $o_fn{node}) or err("rename $f_fn{node} $o_fn{node}");
rename($f_fn{job},  $o_fn{job})  or err("rename $f_fn{job}  $o_fn{job} ");
rename($f_fn{yod},  $o_fn{yod})  or err("rename $f_fn{yod}  $o_fn{yod} ");

# move new to current
rename($t_fn{node}, $f_fn{node}) or err("rename $t_fn{node} $f_fn{node}");
rename($t_fn{job},  $f_fn{job})  or err("rename $t_fn{job}  $f_fn{job} ");
rename($t_fn{yod},  $f_fn{yod})  or err("rename $t_fn{yod}  $f_fn{yod} ");

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
