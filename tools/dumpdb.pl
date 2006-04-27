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
my $user = "sys_mgmt";
my $pass = "sys_mgmt";
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
		processor_id		AS nid,
		yod_id			AS yid
	FROM
		yod_allocation
SQL

my ($row, %ymap, %jmap);

$sth->execute();
while ($row = $sth->fetchrow_hashref()) {
	$ymap{$row->{nid}} = $row->{yid};
}
$sth->finish;

$sth = $dbh->prepare(<<SQL) or dberr("preparing sql");
	SELECT
		jobmap.processor_id	AS nid,
		job.job_id		AS jid
	FROM
		partition_allocation	jobmap,
		partition		job
	WHERE
		job.partition_id = jobmap.partition_id
SQL

$sth->execute();
while ($row = $sth->fetchrow_hashref()) {
	($jmap{$row->{nid}} = $row->{jid}) =~ s/(\d+).*/$1/;
}
$sth->finish;

$sth = $dbh->prepare(<<SQL) or dberr("preparing sql");
	SELECT
		processor_id		AS nid,
		processor_status	AS status,
		processor_type		AS type,
		cab_row			AS r,
		cab_position		AS cb,
		cage			AS cg,
		slot			AS m,
		cpu			AS n,
		x_coord			AS x,
		y_coord			AS y,
		z_coord			AS z
	FROM
		processor
SQL

$sth->execute();
while ($row = $sth->fetchrow_hashref()) {
	my $status = $row->{status} eq "down" ? "n" :
	    ($row->{type} eq "service" ? "i" : "c");

	my $temp = $temp[$row->{r}][$row->{cb}][$row->{cg}][$row->{m}][$row->{n}];
	$temp = 0 unless $temp;

	my $jobid = exists $jmap{$row->{nid}} ? $jmap{$row->{nid}} : 0;
	my $yodid = exists $ymap{$row->{nid}} ? $ymap{$row->{nid}} : 0;

	# 1	2  3 4  5 6	7 8 9	10	11	12	13	14	15
	# nid	cb r cg m n	x y z	stat	enabled	jobid	temp	yodid	nfails
	# 0	0  0 0  0 0	0 0 0	i	1	111	58	2	0

	#                     1   2  3  4  5  6   7  8  9   10  11  12  13  14  15
	printf { $fh{node} } "%d\t%d %d %d %d %d\t%d %d %d\t%s\t%d\t%d\t%d\t%d\t%d\n",
	    @$row{qw(nid r cb cg m n x y z)}, $status,
	    1, $jobid, $temp, $yodid, 0;
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

my %j = (state => "");
open CONNFH, "ssh $login_host \"perl -We 'my \\\$pid = fork; " .
    "exit if \\\$pid == -1; if (\\\$pid) { " .
    "\\\$SIG{ALRM}=sub{kill 1, \\\$pid}; alarm(5); wait; " .
    "} else { exec qw{qstat -f} }'\" |" or err("ssh $login_host");
for (;;) {
	# XXX: clear $! ?
	unless (defined($line = <CONNFH>)) {
		err("read qstat") if $!;
	}
	if (eof CONNFH or $line =~ /^Job Id: (\d+)/) {
		if ($j{state} eq "R") {
			printf { $fh{job} } "%d\t%s\t%d\t%d\t%d\t%d\t%s\t%s\n",
			    @j{qw(id owner tmdur tmuse mem ncpus queue name)};
		}
		last if eof CONNFH;

		%j = (id => $1, state => "", name => "?",
		    owner => "?", queue => "?", ncpus => 0,
		    tmdur => 0, tmuse => 0, mem => 0);
	} elsif ($line =~ /\s*Job_Name = (.*)/) {
		$j{name} = $1;
	} elsif ($line =~ /\s*Job_Owner = (.*)/) {
		($j{owner} = $1) =~ s/@.*//;
	} elsif ($line =~ /\s*job_state = (.*)/) {
		$j{state} = $1;
	} elsif ($line =~ /\s*queue = (.*)/) {
		$j{queue} = $1;
	} elsif ($line =~ /\s*Resource_List\.size = (\d+)/) {
		$j{ncpus} = $1;
	} elsif ($line =~ /\s*Resource_List\.walltime = (\d+):(\d+):(\d+)/) {
		$j{tmdur} = $1 * 60 + $2;
	} elsif ($line =~ /\s*resources_used\.walltime = (\d+):(\d+):(\d+)/) {
		$j{tmuse} = $1 * 60 + $2;
	} elsif ($line =~ /\s*resources_used\.mem = (\d+)kb/) {
		$j{mem} = $1;
	}
}
close CONNFH;

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
