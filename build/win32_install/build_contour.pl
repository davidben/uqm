#! /usr/bin/perl -w

if (scalar (@ARGV) != 2) {
  print STDERR "$0 cvs_tag output_contour_file\n";
  exit 1;
}

$ver = shift @ARGV;
$file = shift @ARGV;
if ($ver =~ /(\d+_\d+\S*)$/) {
  $vernum = $1;
  $vernum =~ s/_/./g;
} else {
  print STDERR "Couldn't parse the cvs-tag for a release version\n";
  $vernum = "0.0";
}
open FH1, ">$file" or die "Couldn't write  to $file\n";
print FH1 "$vernum\n";
chdir "../../content";
open FH, "cvs log -h|" or die "Couldn't run 'cvs log -h'\n";
$file = "";
while (<FH>) {
  chomp;
  if (/Working file: (.+)\s*$/) {
    $file = $1;
  }
  if (/head: (\S+)/) {
    $head = $1;
  }
  if (/\s+${ver}:\s+(\S+)/) {
    print FH1 "$file : $1\n";
    $file = "";
    $head = "";
  }
}
close FH;
close FH1;
