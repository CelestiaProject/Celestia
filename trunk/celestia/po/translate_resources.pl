#!/usr/bin/perl

use Encode;
use File::Basename;
# Produces a translated resource file in the src/celestia/res/ directory
# from each po file in the current directory.

my $dir = dirname $0;
my $resource_file = "$dir/../src/celestia/res/celestia.rc";
my $output_dir = "$dir/../src/celestia/res/";

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
# http://msdn.microsoft.com/library/default.asp?url=/library/en-us/intl/nls_238z.asp
my %lang = (
ar => [ '401', 1256 ],
de => [ '407', 1252 ],
el => [ '408', 1253 ],
en => [ '409', 1252 ],
es => [ '40a', 1252 ],
fr => [ '40c', 1252 ],
gl => [ '406', 1252 ],
hu => [ '40e', 1250 ],
it => [ '410', 1252 ],
ja => [ '411',  932 ],
nl => [ '413', 1252 ],
pt_br => [ '416', 1252 ],
pt => [ '816', 1252 ],
ru =>  [ '419', 1251 ],
sv => [ '41d', 1252 ],
uk => [ '422', 1251 ],
);

mkdir "$dir\\..\\locale" if ! -d "$dir\\..\\locale";

foreach my $po (@po_files) {
    my $strings = load_po_strings($po);
    my $res = $resource;
    my $lang = basename $po;
    $lang =~ s/\..*//o;
    mkdir "$dir\\..\\locale\\$lang" if ! -d "$dir\\..\\locale\\$lang";
    mkdir "$dir\\..\\locale\\$lang\\LC_MESSAGES" if ! -d "$dir\\..\\locale\\$lang\\LC_MESSAGES";
    while (my ($k, $v) = each %$strings) {
        Encode::from_to($v, 'UTF-8', "CP$lang{$lang}[1]");
        $res =~ s/"\Q$k\E"/"$v"/g;
    }
    $res=~ s/\Q#pragma code_page(1252)\E/#pragma code_page($lang{$lang}[1])/;
    open OUT, "> $output_dir/celestia_$lang.rc";
    print OUT $res;
    close OUT;
    system qq{rc /l $lang{$lang}[0] /d "NDEBUG" /fo $dir\\..\\src\\celestia\\celestia_$lang.res /i "$dir\\..\\src\\celestia\\res" $dir\\..\\src\\celestia\\res\\celestia_$lang.rc};
    system qq{link /nologo /noentry /dll /machine:I386 /out:$dir\\..\\locale\\res$lang{$lang}[0].dll $dir\\..\\src\\celestia\\celestia_$lang.res};
    system qq{msgfmt $dir\\$lang.po -o $dir\\..\\locale\\$lang\\LC_MESSAGES\\celestia.mo};
}

opendir(DIR, "$dir\\..\\po2");
my @po_files = sort (grep( /\.po$/, readdir(DIR) ));
closedir DIR;
foreach my $po (@po_files) {
    my $lang = basename $po;
    $lang =~ s/\..*//o;
    mkdir "$dir\\..\\locale\\$lang" if ! -d "$dir\\..\\locale\\$lang";
    mkdir "$dir\\..\\locale\\$lang\\LC_MESSAGES" if ! -d "$dir\\..\\locale\\$lang\\LC_MESSAGES";
    system qq{msgfmt $dir\\..\\po2\\$lang.po -o $dir\\..\\locale\\$lang\\LC_MESSAGES\\celestia_constellations.mo};
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
