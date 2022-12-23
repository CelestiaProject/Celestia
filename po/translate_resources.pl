#!/usr/bin/perl

################################################################
#           Utility for i18n on Win32
#
#   Author: Christophe Teyssier <chris@teyssier.org>
#   date: July 2006
#
# - Loads translations from the po files and produces translated .rc files in src/celestia/res
# - Produces a dll with the translated resources for each po file in the locale/ dir
# - Produces a list of unicode codepoints for each language in the current dir
#   (to generate txf font textures)
# - Compiles .po files and installs catalogs under locale/
#
# Requirements:
# - rc.exe link.exe and msgfmt.exe must be in the PATH
################################################################

use Encode;
use File::Basename;
use File::Path qw(make_path);
use Cwd qw(getcwd);

my $platform = $#ARGV == -1 ? 'win32' : $ARGV[0];
my $machine = $platform == 'win32' ? 'X86' : $platform;

my $po_dir = dirname $0;
my $resource_file = "$po_dir/../src/celestia/win32/res/celestia.rc";
my $rc_dir = "$po_dir/../src/celestia/win32/res";
#my $build_dir = getcwd;
my $res_dir = "$po_dir/../build/src/celestia/win32/res";

opendir(DIR, $po_dir);
my @po_files = sort (grep( /\.po$/, readdir(DIR) ));
closedir DIR;

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
ar => [ '401', 1256 ],
be => [ '423', 1251 ],
bg => [ '402', 1251 ],
de => [ '407', 1252 ],
el => [ '408', 1253 ],
en => [ '409', 1252 ],
es => [ '40a', 1252 ],
fr => [ '40c', 1252 ],
gl => [ '456', 1252 ],
hu => [ '40e', 1250 ],
it => [ '410', 1252 ],
ja => [ '411',  932 ],
ko => [ '412',  949 ],
lt => [ '427', 1257 ],
lv => [ '426', 1257 ],
nb => [ '414', 1252 ],
nl => [ '413', 1252 ],
nn => [ '814', 1252 ],
pl => [ '415', 1250 ],
pt_BR => [ '416', 1252 ],
pt => [ '816', 1252 ],
ro => [ '418', 1250 ],
ru => [ '419', 1251 ],
sk => [ '41b', 1250 ],
sv => [ '41d', 1252 ],
tr => [ '41f', 1254 ],
uk => [ '422', 1251 ],
zh_CN => [ '804', 936 ],
zh_TW => [ '404', 950 ],
);

make_path($res_dir) if ! -d $res_dir;

foreach my $po (@po_files) {
    my $strings = load_po_strings("$po_dir/$po");
    my $res = $resource;
    my $lang = basename $po;
    $lang =~ s/\..*//o;

    while (my ($k, $v) = each %$strings) {
        Encode::from_to($v, 'UTF-8', "CP$lang{$lang}[1]");
        $res =~ s/"\Q$k\E"/"$v"/g;
    }

    if ($lang{$lang}) {
        $res=~ s/\Q#pragma code_page(1252)\E/#pragma code_page($lang{$lang}[1])/;
        $res=~ s/VALUE "Translation", 0x409, 1252/VALUE "Translation", 0x$lang{$lang}[0], $lang{$lang}[1]/;
        open OUT, "> $res_dir/celestia_$lang.rc";
        print OUT $res;
        close OUT;
        system qq{echo rc /l $lang{$lang}[0] /d NDEBUG /fo $res_dir\\celestia_$lang.res /i $rc_dir $res_dir\\celestia_$lang.rc};
        system qq{echo link /nologo /noentry /dll /machine:$machine /out:$res_dir\\res_$lang.dll $res_dir\\celestia_$lang.res};

        system qq{rc /l $lang{$lang}[0] /d NDEBUG /fo $res_dir\\celestia_$lang.res /i $rc_dir $res_dir\\celestia_$lang.rc};
        system qq{link /nologo /noentry /dll /machine:$machine /out:$res_dir\\res_$lang.dll $res_dir\\celestia_$lang.res};
    }
}

sub load_po_strings {
    my $po = shift;

    my %strings;
    open PO, "< $po";
    my $l1;
    my $l2;
    my $l3;
    while ($l3 = <PO>) {
        # The po file is read by groups of three lines.
        # we can safely ignore multiline msgids since resource files
        # use only single line strings
        if ($l2 =~ /^msgid\s+"(.*)"/) {
            my $msgid = $1;
            if ($l3 =~ /^msgstr\s+"(.+)"/ && $l1 !~ /fuzzy/ ) {
                $strings{$msgid} = $1;
            }
        }
        $l1 = $l2;
        $l2 = $l3;
    }
    close PO;
    return \%strings;
}
