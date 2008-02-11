#!/usr/bin/perl -W
# $Id$

use DBI;
use File::Basename;
use Symbol;
use POSIX;
use strict;
use warnings;

my $err = 1;
# start config

my $smwhost = "smw";
my $port = 3307;
my $user = "sys_mgmt";
my $pass = "sys_mgmt";
my $db   = "XTAdmin";
my $temp_path = "/home/crayadm/TempCheck/XT3CpuTemp*.txt"; # smw path to temperatures
my $mach_path = "/usr/local/pbs/partition_info"; # extra machine info

# data file base names
my %c_fn = (
	node	=> "node",
	job	=> "job",
	yod	=> "yod",
);

my $outdir = dirname($0) . "/../data";
my $tmpsufx = ".new";
my $oldsufx = ".bak";

my @date = localtime(time);
my $ardir   = strftime("$outdir/archive/%Y-%m/%Y-%m-%d/%H-%M", @date);
my $isdumpfn= strftime("$outdir/archive/%Y-%m/%Y-%m-%d/%H-%M/.isdump", @date);
my $lastdir = strftime("$outdir/archive/%Y-%m/%Y-%m-%d/.last", @date);
my $aridx   = strftime("$outdir/archive/%Y-%m/%Y-%m-%d/.isar", @date);

my $reel_last_symlink = "$outdir/latest-archive";
# relative to symlink location
my $reel_last_actual  = strftime("archive/%Y-%m/%Y-%m-%d", @date);

# end config

my $fn = dirname($0) . "/cfg-loginhost";
open F, "<", $fn or die "$fn: $!\n";
chomp(my $login_host = <F>);
close F;

$fn = dirname($0) . "/cfg-sqlhost";
open F, "<", $fn or die "$fn: $!\n";
chomp(my $host = <F>);
close F;

my $dbh = DBI->connect("DBI:mysql:database=$db;host=$host;port=$port",
    $user, $pass, { PrintError => $err }) or dberr("connect $host:$port");

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

# Gather extra machine info.
open CONNFH, "ssh $login_host cat $mach_path |" or err("ssh $login_host");
while (defined($line = <CONNFH>)) {
	$line =~ s/^\s+|\s+$//g;

	if ($line =~ /^DRAIN_TIME\s*=\s*(\d+)$/) {
		print { $fh{node} } "\@drain $1\n";
	}
}
close CONNFH;

# Build more fields for node data file.
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
		p.processor_id		AS nid,
		p.processor_status	AS status,
		p.processor_type	AS type,
		p.cab_row		AS r,
		p.cab_position		AS cb,
		p.cage			AS cg,
		p.slot			AS m,
		p.cpu			AS n,
		p.x_coord		AS x,
		p.y_coord		AS y,
		p.z_coord		AS z,
		l.recovery_status	AS lustat
	FROM
		processor p,
		lustre l
	WHERE
		p.processor_id = l.processor_id
SQL

print { $fh{node} }
    "# ", strftime("%Y-%m-%d %H:%M", @date), "\n",
    "# 1	2  3  4 5 6	7 8 9	10	11	12	13	14	15		16\n",
    "# nid	r cb cg m n	x y z	stat	enabled	jobid	temp	yodid	<UNUSED>	lustat\n";

$sth->execute();
while ($row = $sth->fetchrow_hashref()) {
	my %stat = (
		admindown	=> "d",
		down		=> "n",
		route		=> "r",
		suspect		=> "s",
		unavail		=> "u",
		up		=> "c",
	);
	my $status = $stat{$row->{status}};
	$status = "i" if $row->{type} eq "service";

	my $lustat = substr $row->{lustat}, 0, 1;

	my $temp = $temp[$row->{r}][$row->{cb}][$row->{cg}][$row->{m}][$row->{n}];
	$temp = 0 unless $temp;

	my $jobid = exists $jmap{$row->{nid}} ? $jmap{$row->{nid}} : 0;
	my $yodid = exists $ymap{$row->{nid}} ? $ymap{$row->{nid}} : 0;

	$row->{x} = "-1" unless defined $x->{x};
	$row->{y} = "-1" unless defined $x->{y};
	$row->{z} = "-1" unless defined $x->{z};

	printf { $fh{node} }
	#    1   2  3   4   5  6   7  8  9   10  11  12  13  14  15    16
	    "%d\t%d %2d %2d %d %d\t%d %d %d\t%s\t%d\t%d\t%d\t%d\t%d\t\t%s\n",
	    @$row{qw(nid r cb cg m n x y z)},
	    $status,	# 10
	    1,		# 11
	    $jobid,	# 12
	    $temp,	# 13
	    $yodid,	# 14
	    0,		# 15
	    $lustat;	# 16
}
$sth->finish;

# Gather and print yod data.
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

print { $fh{job} }
    "# ", strftime("%Y-%m-%d %H:%M", @date), "\n",
    "# 1	2		3	4	5	6	7	8\n",
    "# id	owner		tmdur	tmuse	mem	ncpus	queue	name\n";

# Gather data from qstat(1) and print job data file.
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
			$j{owner} .= "\t" if length $j{owner} < 8;
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

if (!-e $lastdir || `diff -qr $lastdir $outdir | grep -v ^Only`) {
	my $files = join ",", keys %c_fn;
	`mkdir -p $ardir && cp -R $outdir/{$files} $ardir`;
	`rm -f $lastdir && ln -s \$(basename $ardir) $lastdir`;
} else {
	my $lastreal = readlink $lastdir;
	`ln -s $lastreal $ardir`;
}
`touch $aridx $isdumpfn`; # XXX move inside first "if" branch above.

`rm -f $reel_last_symlink && ln -s $reel_last_actual $reel_last_symlink`;

sub dberr {
	warn "$0: ", @_, ": $DBI::errstr\n" if $err;
	exit $err;
}

sub errx {
	warn "$0: ", @_, "\n" if $err;
	exit $err;
}

sub err {
	warn "$0: ", @_, ": $!\n" if $err;
	exit $err;
}
