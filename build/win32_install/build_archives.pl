#! /usr/bin/perl -w
use File::Find;
use Cwd;
# set use_zip = 1 for .zip, = 0 for .tgz
$use_zip = 1;

$make_archive = 1;

$contour_file = shift @ARGV;
$tag = shift @ARGV;
$do_full = shift @ARGV;
if (! defined ($contour_file) || scalar(@ARGV)) {
  print STDERR "$0 contour_file [full]\n";
#  print STDERR "  or\n";
#  print STDERR "$0 cvs_log_file cvs_tag [full]\n";
  exit 1;
}

# fetch the version from uqmversion.h
if (-f "../../src/uqmversion.h") {
  open (FH, "../../src/uqmversion.h") or die "couldn't read uqmversion.h\n";
  while (<FH>) {
    if (/^\s*\#define\s+UQM_MAJOR_VERSION\s+(\d+)/) {
      $major = $1;
    }
    if (/^\s*#define\s+UQM_MINOR_VERSION\s+(\d+)/) {
      $minor = $1;
    }
    if (/^\s*#define\s+UQM_EXTRA_VERSION\s+"(.*)"/) {
      $extra = $1;
    }
  }
  close FH;
  if (!defined ($major) || !defined ($minor) || !defined ($extra)) {
    die "Error reading uqmversion.h\n";
  }
  $version = "$major.${minor}${extra}";
} else {
  die "couldn't find uqmversion.h\n";
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
@dirs = ();
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
  print STDERR "reading from a cvs log is not suppported\n";
  exit 1;

  if ($tag =~ /(\d+[._]\d+\S*)$/) {
    $oldver = $1;
  } else {
    $oldver = "";
  }
  # read in a cvs log for the content dir.
  # log is made by doing:
  #   cd content
  #   cvs log -h > logfile
  open FH, $contour_file || die "Couldn't read cvs-log '$contour_file'\n";
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
  open FH, $contour_file || die "Couldn't read contour '$contour_file'\n";
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
        print "Couldn't find '$1'.  OK if removed since $oldver\n";
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
$change_zip = "uqm-${oldver}_to_${version}-content.$ext";
$change_ogg_zip = "uqm-${oldver}_to_${version}-voice.$ext";
$content_zip = "uqm-${version}-content.$ext";
$voice_zip = "uqm-${version}-voice.$ext";
$threedo_zip = "uqm-${version}-3domusic.$ext";

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
unlink "$cwd/$change_zip";
unlink "$cwd/$change_ogg_zip";
if ($make_archive) {
  if ($use_zip) {
    `cat tmpfile | zip -r "$cwd/$change_zip" -@`;
    `cat tmpfile1 | zip -r "$cwd/$change_ogg_zip" -@` if ($change_ogg_size);
  } else {
    `tar -T tmpfile -czf "$cwd/$change_zip"`;
    `tar -T tmpfile1 -czf "$cwd/$change_ogg_zip"` if ($change_ogg_size);
  }
}
open (FH, ">$cwd/content.nsh");
print FH "!verbose 3\n";
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
foreach $path (@all) {
  if (-f $path) {
    ($null,$null,$null,$null,$null,$null,$null,$siz1) = stat $path;
    if ($path =~ /comm\/.*\/.*\.ogg$/) {
      $voice_size += $siz1;
      $num_voice++;
      print FH2 "$path\n";
    } elsif ($path =~ /\.ogg$/) {
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
    @partdir = split /\//, $dir;
    $newdir = "";
    foreach $dir (@partdir) {
      $newdir .= "/" if ($newdir);
      $newdir .= "$dir";
      if (! grep(m{^$newdir$}, @dirs)) {
        unshift @dirs, $newdir;
      }
    } 
  }
#    if ($dir ne $last_dir) {
#      push @dirs, $dir;
#      $last_dir = $dir;
#    }
#  }
  $path =~ s/\//\\/g;
  print FH "Delete \"\$INSTDIR\\content\\$path\"\n";
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
  unlink "$cwd/$content_zip";
  unlink "$cwd/$voice_zip";
  unlink "$cwd/$threedo_zip";
  if ($use_zip) {
    `cat tmpfile | zip -r "$cwd/$content_zip" -@`;
    `cat tmpfile1 | zip -r "$cwd/$voice_zip" -@` if($voice_size);
    `cat tmpfile2 | zip -r "$cwd/$threedo_zip" -@` if($threedo_size);
  } else {
    `tar -T tmpfile -czf "$cwd/$content_zip"`;
    `tar -T tmpfile1 -czf "$cwd/$voice_zip"` if ($voice_size);
    `tar -T tmpfile2 -czf "$cwd/$threedo_zip"` if ($threedo_size);
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

print FH "!verbose 4\n\n";
print FH "!ifndef MUI_VERSION\n";
print FH "  !define MUI_VERSION \"$version\"\n";
print FH "!endif\n";
print FH "\n";
print FH "!define CONTENT_SIZE  $content_size\n";
print FH "!define VOICE_SIZE    $voice_size\n";
print FH "!define THREEDO_SIZE  $threedo_size\n";
print FH "!define UPGRADE_SIZE  $change_size\n";
print FH "!define VUPGRADE_SIZE $change_ogg_size\n";
print FH "\n";
if ($do_full) {
  print FH "!ifndef CONTENT_PACKAGE_FILE\n";
  print FH "  !define CONTENT_PACKAGE_FILE  \"$content_zip\"\n";
  print FH "!endif\n";
  print FH "!ifndef CONTENT_TYPE\n";
  print FH "  !define CONTENT_TYPE          \"$ext\"\n";
  print FH "!endif\n";
  print FH "!ifndef CONTENT_ROOT\n";
  print FH "  !define CONTENT_ROOT          \"\\content\"\n";
  print FH "!endif\n";
  print FH "\n";
  print FH "!ifndef VOICE_PACKAGE_FILE\n";
  print FH "  !define VOICE_PACKAGE_FILE    \"$voice_zip\"\n";
  print FH "!endif\n";
  print FH "!ifndef VOICE_TYPE\n";
  print FH "  !define VOICE_TYPE            \"$ext\"\n";
  print FH "!endif\n";
  print FH "!ifndef VOICE_ROOT\n";
  print FH "  !define VOICE_ROOT            \"\\content\"\n";
  print FH "!endif\n";
  print FH "\n";
  print FH "!ifndef THREEDO_PACKAGE_FILE\n";
  print FH "  !define THREEDO_PACKAGE_FILE  \"$threedo_zip\"\n";
  print FH "!endif\n";
  print FH "!ifndef THREEDO_TYPE\n";
  print FH "  !define THREEDO_TYPE          \"$ext\"\n";
  print FH "!endif\n";
  print FH "!ifndef THREEDO_ROOT\n";
  print FH "  !define THREEDO_ROOT          \"\\content\"\n";
  print FH "!endif\n";
  print FH "\n";
}
print FH "!ifndef UPGRADE_PACKAGE_FILE\n";
print FH "  !define UPGRADE_PACKAGE_FILE  \"$change_zip\"\n";
print FH "!endif\n";
print FH "!ifndef UPGRADE_TYPE\n";
print FH "  !define UPGRADE_TYPE          \"$ext\"\n";
print FH "!endif\n";
print FH "!ifndef UPGRADE_ROOT\n";
print FH "  !define UPGRADE_ROOT          \"\\content\"\n";
print FH "!endif\n";
print FH "\n";
print FH "!ifndef VUPGRADE_PACKAGE_FILE\n";
print FH "  !define VUPGRADE_PACKAGE_FILE \"$change_ogg_zip\"\n";
print FH "!endif\n";
print FH "!ifndef VUPGRADE_TYPE\n";
print FH "  !define VUPGRADE_TYPE         \"$ext\"\n";
print FH "!endif\n";
print FH "!ifndef VUPGRADE_ROOT\n";
print FH "  !define VUPGRADE_ROOT         \"\\content\"\n";
print FH "!endif\n";
print FH "\n";
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
  if ($dir !~ /(.*)\/CVS$/) {
    return;
  } 
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
