# Erzeugt Header File mit GIT-Versionsinfo
#
# 180824, Rp: erzeugt
#

my $GITPATH="C:\\Users\\Andreas\\AppData\\Local\\Atlassian\\SourceTree\\git_local\\bin";
open(my $vh, ">", "version_info.h")
 	or die "Can't open > version_info.h: $!";

my $Version = `$GITPATH\\git describe --tags`;
chomp $Version;
print $vh "#define PRG_VERS \"$Version\"\n";
my $VersInfo = `$GITPATH\\git log --pretty=full -n 1`;
$VersInfo =~ s/\n/<br>/g;
print $vh "#define PRG_CHANGE_DESC \"$VersInfo\"";
close $vh;