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

my $dir = dirname $0;
my $resource_file = "$dir/../src/celestia/win32/res/celestia.rc";
my $output_dir = "$dir/../src/celestia/win32/res/";

opendir(DIR, $dir);
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
lt => [ '427',  1257 ],
lv => [ '426', 1257 ],
nl => [ '413', 1252 ],
no => [ '414', 1252 ],
pl => [ '415', 1250 ],
pt_BR => [ '416', 1252 ],
pt => [ '816', 1252 ],
ro =>  [ '418', 1250 ],
ru =>  [ '419', 1251 ],
sk =>  [ '41b', 1250 ],
sv => [ '41d', 1252 ],
tr => [ '41f', 1254 ],
uk => [ '422', 1251 ],
zh_CN => [ '804', 936 ],
zh_TW => [ '404', 950 ],
);

mkdir "$dir\\..\\locale" if ! -d "$dir\\..\\locale";

my %codepoints; # hash holds unicode codepoints used by language

foreach my $po (@po_files) {
    my $strings = load_po_strings("$dir\\$po");
    my $res = $resource;
    my $lang = basename $po;
    $lang =~ s/\..*//o;
    mkdir "$dir\\..\\locale\\$lang" if ! -d "$dir\\..\\locale\\$lang";
    mkdir "$dir\\..\\locale\\$lang\\LC_MESSAGES" if ! -d "$dir\\..\\locale\\$lang\\LC_MESSAGES";

    while (my ($k, $v) = each %$strings) {
        map { $codepoints{$lang}{$_} = 1; } map { sprintf '%04X', ord($_); } split //, Encode::decode_utf8($v);
        Encode::from_to($v, 'UTF-8', "CP$lang{$lang}[1]");
        map { $c{ord($_)} = 1; } split //, Encode::decode("UTF-8", $v);
        $res =~ s/"\Q$k\E"/"$v"/g;
    }

    $res=~ s/\Q#pragma code_page(1252)\E/#pragma code_page($lang{$lang}[1])/;
    open OUT, "> $output_dir/celestia_$lang.rc";
    print OUT $res;
    close OUT;
    system qq{rc /l $lang{$lang}[0] /d "NDEBUG" /fo $dir\\..\\src\\celestia\\win32\\celestia_$lang.res /i "$dir\\..\\src\\celestia\\win32\\res" $dir\\..\\src\\celestia\\win32\\res\\celestia_$lang.rc};
    system qq{link /nologo /noentry /dll /machine:I386 /out:$dir\\..\\locale\\res_$lang.dll $dir\\..\\src\\celestia\\win32\\celestia_$lang.res};
    system qq{msgfmt $dir\\$lang.po -o $dir\\..\\locale\\$lang\\LC_MESSAGES\\celestia.mo};
}

opendir(DIR, "$dir\\..\\po2");
my @po_files = sort (grep( /\.po$/, readdir(DIR) ));
closedir DIR;
foreach my $po (@po_files) {
    my $lang = basename $po;
    $lang =~ s/\..*//o;

    my $strings = load_po_strings("$dir\\..\\po2\\$po");
    while (my ($k, $v) = each %$strings) {
        map { $codepoints{$lang}{$_} = 1; } map { sprintf '%04X', ord($_); } split //, Encode::decode_utf8($v);
    }

    mkdir "$dir\\..\\locale\\$lang" if ! -d "$dir\\..\\locale\\$lang";
    mkdir "$dir\\..\\locale\\$lang\\LC_MESSAGES" if ! -d "$dir\\..\\locale\\$lang\\LC_MESSAGES";
    system qq{msgfmt $dir\\..\\po2\\$lang.po -o $dir\\..\\locale\\$lang\\LC_MESSAGES\\celestia_constellations.mo};
}

foreach my $lang (keys %codepoints) {
    # list of unicode codepoints to generate font textures
    my $chr;
    map { $chr .= "$_ \n"; } sort keys %{$codepoints{$lang}};
    open CHR, "> $dir/codepoints_$lang.txt";
    print CHR $chr;
    close CHR;
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
