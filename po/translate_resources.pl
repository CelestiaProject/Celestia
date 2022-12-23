#!/usr/bin/perl

################################################################
#           Utility for i18n on Win32
#
#   Copyright: 2006-2021, Celestia Development Team
#   Original author: Christophe Teyssier <chris@teyssier.org>
#   Original date: July 2006
#
# - Loads translations from the po files and produces translated .rc
#   files in src/celestia/win32/res
################################################################

use Encode;
use File::Basename;
use File::Path qw(make_path);
use Cwd qw(getcwd);

my $lang = $ARGV[0];
my $compiler = $ARGV[1];
my $machine = $ARGV[2];
my $rc_compiler = $ARGV[3];
my $linker = $ARGV[4];

my $po_dir = dirname $0;
my $resource_file = "$po_dir/../src/celestia/win32/res/celestia.rc";
my $rc_dir = dirname $resource_file;
my $build_dir = getcwd;

open RES, "< $resource_file";
my $resource;
{
    local $/;
    $resource = <RES>;
}
close RES;

# LangID and code page
# http://msdn.microsoft.com/en-us/library/dd318693
my %lang = (
ar => [ '401', 1256, 'LANG_ARABIC, SUBLANG_DEFAULT'],
be => [ '423', 1251, 'LANG_BELARUSIAN, SUBLANG_DEFAULT'],
bg => [ '402', 1251, 'LANG_BULGARIAN, SUBLANG_DEFAULT'],
de => [ '407', 1252, 'LANG_GERMAN, SUBLANG_DEFAULT'],
el => [ '408', 1253, 'LANG_GREEK, SUBLANG_DEFAULT'],
en => [ '409', 1252, 'LANG_ENGLISH, SUBLANG_DEFAULT'],
es => [ '40a', 1252, 'LANG_SPANISH, SUBLANG_DEFAULT'],
fr => [ '40c', 1252, 'LANG_FRENCH, SUBLANG_DEFAULT'],
gl => [ '456', 1252, 'LANG_GALICIAN, SUBLANG_DEFAULT' ],
hu => [ '40e', 1250, 'LANG_HUNGARIAN, SUBLANG_DEFAULT'],
it => [ '410', 1252, 'LANG_ITALIAN, SUBLANG_DEFAULT'],
ja => [ '411',  932, 'LANG_JAPANESE, SUBLANG_DEFAULT'],
ko => [ '412',  949, 'LANG_KOREAN, SUBLANG_DEFAULT'],
lt => [ '427', 1257, 'LANG_LITHUANIAN, SUBLANG_DEFAULT'],
lv => [ '426', 1257, 'LANG_LATVIAN, SUBLANG_DEFAULT'],
nb => [ '414', 1252, 'LANG_NORWEGIAN, SUBLANG_NORWEGIAN_BOKMAL'],
nl => [ '413', 1252, 'LANG_DUTCH, SUBLANG_DEFAULT'],
nn => [ '814', 1252, 'LANG_NORWEGIAN, SUBLANG_NORWEGIAN_NYNORSK'],
pl => [ '415', 1250, 'LANG_POLISH, SUBLANG_DEFAULT'],
pt_BR => [ '416', 1252, 'LANG_PORTUGUESE, SUBLANG_PORTUGUESE_BRAZILIAN'],
pt => [ '816', 1252, 'LANG_PORTUGUESE, SUBLANG_PORTUGUESE_PORTUGAL'],
ro => [ '418', 1250, 'LANG_ROMANIAN, SUBLANG_DEFAULT'],
ru => [ '419', 1251, 'LANG_RUSSIAN, SUBLANG_DEFAULT'],
sk => [ '41b', 1250, 'LANG_SLOVAK, SUBLANG_DEFAULT'],
sv => [ '41d', 1252, 'LANG_SWEDISH, SUBLANG_DEFAULT'],
tr => [ '41f', 1254, 'LANG_TURKISH, SUBLANG_DEFAULT'],
uk => [ '422', 1251, 'LANG_UKRAINIAN, SUBLANG_DEFAULT'],
zh_CN => [ '804', 936, 'LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED'],
zh_TW => [ '404', 950, 'LANG_CHINESE, SUBLANG_CHINESE_TRADITIONAL'],
);

my @keywords = qw{
    POPUP
    MENUITEM
    DEFPUSHBUTTON
    LTEXT
    CAPTION
    PUSHBUTTON
    RTEXT
    CTEXT
    LTEXT
    GROUPBOX
    CONTROL
    GROUPBOX
    AUTOCHECKBOX
    AUTORADIOBUTTON
};

my $keys = join('|', @keywords);

my $strings = load_po_strings("$po_dir/$lang.po");
my @res = split(/\n|\r\n/, $resource);
my $lang_id = $lang{$lang}[0];
my $codepade = $lang{$lang}[1];
my $lang_name =  $lang{$lang}[2];

while (my ($k, $v) = each %$strings) {
    Encode::from_to($v, 'UTF-8', "CP$codepade");
    my $msgctxt;
    my $msgid;
    my $context = 0;

    if ($k =~ /^(.+)\004(.+)$/) {
        $msgctxt = $1;
        $msgid = $2;
        $context = 1;
    }
    foreach my $line (@res) {
        if ($line =~ /^\s*(?:$keys)/) {
            if ($context) {
                $line =~ s/\bNC_\(\s*"\Q$msgctxt\E"\s*,\s*"\Q$msgid\E"\s*\)/"$v"/g;
            } else {
                $line =~ s/"\Q$k\E"/"$v"/g;
            }
        }
    }
}

#while (my ($k, $v) = each %$strings) {
#    next if $k !~ /^(.+)\004(.+)$/;
#    Encode::from_to($v, 'UTF-8', "CP$codepade");
#    my $msgctxt = $1;
#    my $msgid = $2;
#
#    foreach my $line (@res) {
#        if ($line =~ /^\s*(?:$keys)/) {
#            $line =~ s/\bNC_\(\s*"\Q$msgctxt\E"\s*,\s*"\Q$msgid\E"\s*\)/"$v"/g;
#        }
#    }
#}
#
#while (my ($k, $v) = each %$strings) {
#    next if $k =~ /^(.+)\004(.+)$/;
#    Encode::from_to($v, 'UTF-8', "CP$codepade");
#    foreach my $line (@res) {
#        if ($line =~ /^\s*(?:$keys)/) {
#            $line =~ s/"\Q$k\E"/"$v"/g;
#        }
#    }
#}

foreach my $line (@res) {
    $line =~ s/LANGUAGE LANG_ENGLISH, SUBLANG_ENGLISH_US/\/\/LANGUAGE $lang_name/;
    $line =~ s/\Q#pragma code_page(1252)\E/#pragma code_page($codepade)/;
    $line =~ s/VALUE "Translation", 0x0?409, (?:1252|0x04B0)/VALUE "Translation", 0x$lang_id, $codepade/;
}
open OUT, "> celestia_$lang.rc";
print OUT join("\r\n", @res);
close OUT;

if ($compiler eq "msvc") {
  system qq{rc /l $lang_id /d NDEBUG /fo celestia_$lang.res /i $rc_dir celestia_$lang.rc};
  system qq{link /nologo /noentry /dll /machine:$machine /out:res_$lang.dll celestia_$lang.res};
} else {
  system qq{$rc_compiler -i celestia_$lang.rc -o celestia_$lang.o -O coff -F pe-$machine -I $rc_dir -l $lang_id -D NDEBUG};
  system qq{$linker -o res_$lang.dll --dll celestia_$lang.o};
}

sub load_po_strings {
    my $po = shift;

    my %strings;
    open PO, "< $po";
    my $l1;
    my $l2;
    my $l3;
    my $l4;
    while ($l4 = <PO>) {
        # The po file is read by groups of three lines.
        # we can safely ignore multiline msgids since resource files
        # use only single line strings
        if ($l3 =~ /^msgid\s+"(.*)"/) {
            my $msgid = $1;
            if ($l4 =~ /^msgstr\s+"(.+)"/) {
                my $translation = $1;
                if ($l2 =~ /^msgctxt\s+"(.+)"/) {
                    my $context = $1;
                    if ($l1 !~ /fuzzy/) {
                        $strings{"$context\004$msgid"} = $translation;
                    }
                } elsif ($l2 !~ /fuzzy/) {
                    $strings{$msgid} = $translation;
                }
            }
        }
        $l1 = $l2;
        $l2 = $l3;
        $l3 = $l4;
    }
    close PO;
    return \%strings;
}
