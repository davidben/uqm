#! /usr/bin/perl -w
use File::Find;
use Cwd;
# set use_zip = 1 for .zip, = 0 for .tgz
$use_zip = 0;


$version = shift @ARGV;
$file = shift @ARGV;
$tag = shift @ARGV;
$do_full = shift @ARGV;
if (! defined ($version) || ! defined ($file) || scalar(@ARGV)) {
  print STDERR "$0 version contour_file [full]\n";
  print STDERR "  or\n";
  print STDERR "$0 version cvs_log_file cvs_tag [full]\n";
  exit 1;
}
if (! defined ($do_full)) {
  if (defined ($tag) && $tag eq "full") {
    $do_full = $tag;
    $tag = "";
  } else {
    $do_full = "";
    if (!defined ($tag)) {
      $tag = "";
    }
  }
}
@changed = ();
@all = ();
$ignore = 0;
%alldata = ();
$cwd = cwd();
if ($use_zip) {
  $ext = "zip";
} else {
  $ext = "tgz";
}

if ($tag) {
  if ($tag =~ /(\d+_\d+\S*)$/) {
    $vernum = $1;
  } else {
    $vernum = "";
  }
  # read in a cvs log for the content dir.
  # log is made by doing:
  #   cd content
  #   cvs log -h > logfile
  open FH, $file || die "Couldn't read cvs-log '$file'\n";
  print "Reading revisions from cvs-log file\n";
  while (<FH>) {
    chomp;
    if (/RCS file: (.+)\s*$/) {
      if ($1 =~ /\/Attic\//) {
        $ignore = 1;
      } else {
        $ignore = 0;
      }
    }
    if (/Working file: (.+)\s*$/) {
      $file = $1;
      $alldata{"$file,head"} = "";
      $alldata{"$file,prev"} = "";
    }
    if (/head: (\S+)/) {
      $head = $1;
      $alldata{"$file,head"} = $head;
      if (! $ignore) {
        push @changed, $file;
        push @all, $file;
      }
    }
    if (/\s+${tag}: (\S+)/) {
       $alldata{"$file,prev"} = $1;
      if (!$ignore && ($head eq $1 || $head eq "1.1" && $1 eq "1.1.1.1")) {
        pop @changed;
      }
    }
  }
  close FH;
  chdir "../../content";
} else {
  print "Reading revisions from local cvs repository\n";
  open FH, $file || die "Couldn't read contour '$file'\n";
  chdir "../../content";
  find (\&find_callback, ".");
  # use a contour file and the 'CVS' directories to determine the file list
  $first = 1;
  $vernum = "";
  while (<FH>) {
    chomp;
    if ($first) {
      $vernum = $_;
      $first = 0;
      next;
    }
    if (/^(\S+) : (\S+)/) {
      if (defined ($alldata{"$1,prev"})) {
        if ($alldata{"$1,head"} ne $2 && $alldata{"$1,head"} ne "1.1" && 
            $alldata{"$1,head"} ne "1.1.1.1") {
          push @changed, $1;
        }
        $alldata{"$1,prev"} = $2;
      } else {
        print "Couldn't find '$1' in the repository\n";
      }
    }
  }
  close FH;
  foreach $change (@all) {
    if ($alldata{"$change,prev"} eq "") {
      push @changed, $change;
    }
  }
}
$change_zip = "$cwd/uqm_${version}_upgrade${vernum}.$ext";
$content_zip = "$cwd/uqm_${version}_content.$ext";


# build the upgrade .zip file
$last_dir = "";
$change_size = 0;
open FH1, ">tmpfile" || die "Couldn't create temp file!\n";
foreach $path (@changed) {
  if (-f $path) {
    ($null,$null,$null,$null,$null,$null,$null,$siz1) = stat $path;
    $change_size += $siz1;
    print FH1 "$path\n";
  } else {
    print "WARNING: '$path' was not found in local repository\n";
  }
}
close FH1;
unlink $change_zip;
if ($use_zip) {
  `cat tmpfile | zip -r $change_zip -@`;
} else {
  `tar -T tmpfile -czf $change_zip`;
}

open (FH, ">$cwd/content.nsh");
open FH1, ">tmpfile" || die "Couldn't create temp file!\n";
$last_dir = "";
$content_size = 0;
@dirs = ();
foreach $path (@all) {
  if (-f $path) {
    ($null,$null,$null,$null,$null,$null,$null,$siz1) = stat $path;
    $content_size += $siz1;
    print FH1 "$path\n";
  } else {
    print "WARNING: '$path' was not found in local repository\n";
  }
  if ($path =~ /^(.*\/)([^\/]+)$/) {
    ($dir, $file) = ($1, $2);
    if ($dir ne $last_dir) {
      push @dirs, $dir;
      $last_dir = $dir;
    }
    print FH "Delete \"\$INSTDIR\\content\\$path\"\n";
  }
}
close FH1;
foreach $dir (@dirs) {
  print FH "RMDir \"\$INSTDIR\\content\\$dir\"\n";
}
close FH;
if ($do_full) {
  unlink $content_zip;
  if ($use_zip) {
    `cat tmpfile | zip -r $content_zip -@`;
  } else {
    `tar -T tmpfile -czf $content_zip`;
  }
  unlink "tmpfile";
}
print "changed: #files: " . scalar (@changed) . " ($change_size kb)\n";
print "total:   #files: " . scalar (@all) . " ($content_size kb)\n";
exit;

##########################################
sub find_callback {
  my ($file) = $File::Find::name;
  my ($dir) = $File::Find::dir;
  return if ($dir !~ /(.*)\/CVS$/);
  my ($root) = ($1 =~ /[^\/]*\/(.*)$/);
  return if ($file !~ /Entries$/);
  open FH_find, $_ or die "Can't read file '$file'\n";
  while (<FH_find>) {
    chomp;
    my (@args) = split '/';
    if (scalar (@args) >= 3 && -f "../$args[1]") {
      if (defined ($root)) {
        $file1 = "$root/$args[1]";
      } else {
        $file1 = $args[1];
      }
      $alldata{"$file1,head"} = $args[2];
      $alldata{"$file1,prev"} = "";
      push @all, $file1;
    } else {
#      print "Couldn't parse '$root' @args\n";
    }
  }
  close FH_find;
}
