#! /usr/bin/perl -w
use File::Find;
use Cwd;

# fetch the version from uqmversion.h
if (-f "../../src/uqmversion.h") {
  open (FH, "../../src/uqmversion.h") or die "couldn't read uqmversion.h\n";
  while (<FH>) {
    if (/^\s*#define\s+UQM_MAJOR_VERSION\s+(\d+)/) {
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

@dirs = ();
@all = ();
$cwd = cwd();
chdir "../../content";
find (\&find_callback, ".");

$content_zip = "uqm-${version}-content.zip";
$voice_zip = "uqm-${version}-voice.zip";
$threedo_zip = "uqm-${version}-3domusic.zip";

open (FH, ">$cwd/content.nsh");

#print FH "!verbose 3\n";
#print FH "!macro REMOVE_CONTENT\n";
#print FH "Delete \"\$INSTDIR\\content\\packages\\$content_zip\"\n";
#print FH "Delete \"\$INSTDIR\\content\\packages\\$voice_zip\"\n";
#print FH "Delete \"\$INSTDIR\\content\\packages\\$threedo_zip\"\n";
#print FH "!macroend\n";

open FH1, ">tmpfile" || die "Couldn't create temp file!\n";
open FH2, ">tmpfile1" || die "Couldn't create temp file!\n";
open FH3, ">tmpfile2" || die "Couldn't create temp file!\n";

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
    $dir = $1;
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
}
close FH1;
close FH2;
close FH3;

unlink "$cwd/$content_zip";
unlink "$cwd/$voice_zip";
unlink "$cwd/$threedo_zip";
`cat tmpfile | zip -r "$cwd/$content_zip" -@`;
`cat tmpfile1 | zip -r -n .ogg "$cwd/$voice_zip" -@` if($voice_size);
`cat tmpfile2 | zip -r -n .ogg "$cwd/$threedo_zip" -@` if($threedo_size);

unlink "tmpfile";
unlink "tmpfile1";
unlink "tmpfile2";

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
print FH "\n";
print FH "!ifndef CONTENT_PACKAGE_FILE\n";
print FH "  !define CONTENT_PACKAGE_FILE  \"$content_zip\"\n";
print FH "!endif\n";
print FH "\n";
print FH "!ifndef VOICE_PACKAGE_FILE\n";
print FH "  !define VOICE_PACKAGE_FILE    \"$voice_zip\"\n";
print FH "!endif\n";
print FH "\n";
print FH "!ifndef THREEDO_PACKAGE_FILE\n";
print FH "  !define THREEDO_PACKAGE_FILE  \"$threedo_zip\"\n";
print FH "!endif\n";
print FH "\n";
close FH;

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
#     print "Couldn't parse '$root' @args\n";
    }
  }
  close FH_find;
}
