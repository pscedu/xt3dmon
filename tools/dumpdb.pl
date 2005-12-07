#!/usr/bin/perl -W
# $Id$

use DBI;
use File::Basename;

# start config

my $host = "localhost";
my $port = 3306;
my $db   = "XTAdmin";

my $outdir = dirname($0) . "/data";
my $c_cpufn = "rtrtrace";
my $c_jobfn = "nids_list_phantom";
my $tmpsufx = ".new";
my $oldsufx = ".bak";

# end config

my $dbh = DBI->connect("dbi:mysql:$db\@$host:$port")
    or dberr("connect $host:$port");

my $t_cpufn = "$outdir/$c_cpufn$tmpsufx";
my $t_jobfn = "$outdir/$c_jobfn$tmpsufx";

my $f_cpufn = "$outdir/$c_cpufn";
my $f_jobfn = "$outdir/$c_jobfn";

my $o_cpufn = "$outdir/$c_cpufn$oldsufx";
my $o_jobfn = "$outdir/$c_jobfn$oldsufx";

open CPUFH, "> $t_cpufn" or err($t_cpufn);
open JOBFH, "> $t_jobfn" or err($t_jobfn);

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
		yod_allocation		AS yod_map
		partition		AS job,
		partition_allocation	AS job_map
	WHERE
		cpu.nid			= job_map.processor_id
	AND	job.partition_id	= job_map.partition_id
	AND	cpu.nid			= yod_map.processor_id
	AND	yod.yod_id		= yod_map.yod_id
SQL

my $row;
while ($row = $dbh->selectrow_hashref($sth)) {
	my $status = $row->{status} eq "down" ? "n" :
	    ($row->{type} eq "service" ? "i" : "c");

	# 0      c0-0c0s0s0      0,0,0   i
	printf JOBFH, "%d\tc%d-%dc%ds%ds%d\t%d,$d,%d\t$s\n",
	    @$row{qw(nid cb r cg m n x y z)}, $status;
}
$sth->finish;

$dbh->disconnect;

close JOBFH;
close CPUFH;

# delete old backup
unlink($o_cpufn) or err("unlink $o_cpufn");
unlink($o_jobfn) or err("unlink $o_jobfn");

# move current files to backup
rename($f_cpufn, $o_cpufn) or err("rename $f_cpufn $o_cpufn");
rename($f_jobfn, $o_jobfn) or err("rename $f_jobfn $o_jobfn");

# move new to current
rename($t_cpufn, $f_cpufn) or err("rename $t_cpufn $f_cpnfn");
rename($t_jobfn, $f_jobfn) or err("rename $t_jobfn $f_jobfn");

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
