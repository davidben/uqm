#! /usr/bin/perl -w

if (scalar (@ARGV) != 2) {
  print STDERR "$0 cvs_log_file cvs_tag\n";
  exit 1;
}

$file = shift @ARGV;
$ver = shift @ARGV;
if ($ver =~ /(\d+_\d+\S*)$/) {
  $vernum = $1;
} else {
  print STDERR "Couldn't parse the cvs-tag for a release version\n";
  $vernum = "0_0";
}
print "$vernum\n";
open FH, $file || die "Couldn't find file $file\n";
while (<FH>) {
  chomp;
  if (/Working file: (.+)\s*$/) {
    $file = $1;
  }
  if (/head: (\S+)/) {
    $head = $1;
  }
  if (/\s+${ver}:\s+(\S+)/) {
    print "$file : $1\n";
    $file = "";
    $head = "";
  }
}
close FH;
