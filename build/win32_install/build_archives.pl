#! /usr/bin/perl -w
use File::Find;
use Cwd;
# set use_zip = 1 for .zip, = 0 for .tgz
$use_zip = 1;


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
  if ($tag =~ /(\d+[._]\d+\S*)$/) {
    $oldver = $1;
  } else {
    $oldver = "";
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
  $oldver = "";
  while (<FH>) {
    chomp;
    if ($first) {
      $oldver = $_;
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
$change_zip = "$cwd/uqm-${oldver}_to_${version}-content.$ext";
$change_ogg_zip = "$cwd/uqm-${oldver}_to_${version}-voice.$ext";
$content_zip = "$cwd/uqm-${version}-content.$ext";
$voice_zip = "$cwd/uqm-${version}-voice.$ext";
$threedo_zip = "$cwd/uqm-${version}-3domusic.$ext";

# build the upgrade .zip file
$last_dir = "";
$change_size = 0;
$change_ogg_size = 0;
$num_change = 0;
$num_change_ogg = 0;
open FH1, ">tmpfile" || die "Couldn't create temp file!\n";
open FH2, ">tmpfile1" || die "Couldn't create temp file!\n";
foreach $path (@changed) {
  if (-f $path) {
    ($null,$null,$null,$null,$null,$null,$null,$siz1) = stat $path;
    if ($path =~ /comm\/.*\/.*\.ogg$/) {
      $change_ogg_size += $siz1;
      $num_change_ogg++;
      print FH2 "$path\n";
    } else {
      $change_size += $siz1;
      $num_change++;
      print FH1 "$path\n";
    }
  } else {
    print "WARNING: '$path' was not found in local repository\n";
  }
}
close FH1;
close FH2;
unlink $change_zip;
unlink $change_ogg_zip;
if ($use_zip) {
  `cat tmpfile | zip -r $change_zip -@`;
  `cat tmpfile1 | zip -r $change_ogg_zip -@` if ($change_ogg_size);
} else {
  `tar -T tmpfile -czf $change_zip`;
  `tar -T tmpfile1 -czf $change_ogg_zip` if ($change_ogg_size);
}

open (FH, ">$cwd/content.nsh");
print FH "!macro REMOVE_CONTENT\n";
open FH1, ">tmpfile" || die "Couldn't create temp file!\n";
open FH2, ">tmpfile1" || die "Couldn't create temp file!\n";
open FH3, ">tmpfile2" || die "Couldn't create temp file!\n";
$last_dir = "";
$content_size = 0;
$voice_size = 0;
$threedo_size = 0;
$num_voice = 0;
$num_content = 0;
$num_3do = 0;
@dirs = ();
foreach $path (@all) {
  if (-f $path) {
    ($null,$null,$null,$null,$null,$null,$null,$siz1) = stat $path;
    if ($path =~ /comm\/.*\/.*\.ogg$/) {
      $voice_size += $siz1;
      $num_voice++;
      print FH2 "$path\n";
    } elsif ($path =~ /\.ogg$/) {
      print "$path : $siz1\n";
      $threedo_size += $siz1;
      $num_3do++;
      print FH3 "$path\n";
    } else {
      $content_size += $siz1;
      $num_content++;
      print FH1 "$path\n";
    }
  } else {
    print "WARNING: '$path' was not found in local repository\n";
  }
  if ($path =~ /^(.*\/)([^\/]+)$/) {
    ($dir, $file) = ($1, $2);
    if ($dir ne $last_dir) {
      push @dirs, $dir;
      $last_dir = $dir;
    }
    $path =~ s/\//\\/g;
    print FH "Delete \"\$INSTDIR\\content\\$path\"\n";
  }
}
close FH1;
close FH2;
close FH3;
foreach $dir (@dirs) {
    $dir =~ s/\/$//;
    $dir =~ s/\//\\/g;
  print FH "RMDir \"\$INSTDIR\\content\\$dir\"\n";
}
print FH "!macroend\n";
if ($do_full) {
  unlink $content_zip;
  unlink $voice_zip;
  if ($use_zip) {
    `cat tmpfile | zip -r $content_zip -@`;
    `cat tmpfile1 | zip -r $voice_zip -@` if($voice_size);
    `cat tmpfile2 | zip -r $threedo_zip -@` if($threedo_size);
  } else {
    `tar -T tmpfile -czf $content_zip`;
    `tar -T tmpfile1 -czf $voice_zip` if ($voice_size);
    `tar -T tmpfile2 -czf $threedo_zip` if ($threedo_size);
  }
}
unlink "tmpfile";
unlink "tmpfile1";
unlink "tmpfile2";
$change_size = int(0.5 + $change_size / 1000);
$change_ogg_size = int(0.5 + $change_ogg_size / 1000);
$content_size = int(0.5 + $content_size / 1000);
$voice_size = int(0.5 + $voice_size / 1000);
$threedo_size = int(0.5 + $threedo_size / 1000);
print FH "!define CONTENT_SIZE  $content_size\n";
print FH "!define VOICE_SIZE    $voice_size\n";
print FH "!define THREEDO_SIZE  $threedo_size\n";
print FH "!define UPGRADE_SIZE  $change_size\n";
print FH "!define VUPGRADE_SIZE $change_ogg_size\n";
close FH;
print "upgrade     : #files: $num_change ($change_size kb)\n";
print "voiceupgrade: #files: $num_change_ogg ($change_ogg_size kb)\n";
print "content     : #files: $num_content ($content_size kb)\n";
print "voice       : #files: $num_voice ($voice_size kb)\n";
print "threedo     : #files: $num_3do ($threedo_size kb)\n";
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
