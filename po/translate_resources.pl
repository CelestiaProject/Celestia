#!/usr/bin/perl

################################################################
#           Utility for i18n on Win32
#
#   Author: Christophe Teyssier <chris@teyssier.org>
#   date: July 2006
#
# - Loads translations from the po files and produces translated .rc files in src/celestia/res
# - Produces a dll with the translated resources for each po file in the locale/ dir
# - Compiles .po files and installs catalogs under locale/
#
# Requirements:
# - rc.exe link.exe must be in the PATH
################################################################

use Encode;
use File::Basename;
use File::Path qw(make_path);
use Cwd qw(getcwd);
use Getopt::Long;

my $platform = 'win32';
my $mingw = 0;
GetOptions('platform=s' => \$platform,
           'mingw'      => \$mingw)
or die ("Error in command line arguments!\n");

my $machine = $platform == 'win32' ? 'x86' : $platform;

my $po_dir = dirname $0;
my $resource_file = "$po_dir/../src/celestia/win32/res/celestia.rc";
my $rc_dir = dirname $resource_file;
my $build_dir = getcwd;
my $res_dir = "$build_dir/src/celestia/win32/res";

make_path($res_dir) if ! -d $res_dir;

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
no => [ '814', 1252 ],
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

my $locale_dir = "$build_dir/locale";
my $dll_dir = "$locale_dir/$platform";

make_path($dll_dir) if ! -d $dll_dir;

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

foreach my $po (@po_files) {
    my $lang = basename($po, ".po");
    next unless $lang{$lang};
    my $strings = load_po_strings("$po_dir/$po");
    my @res = split(/\n|\r\n/, $resource);
    my $lang_id = $lang{$lang}[0];
    my $codepade = $lang{$lang}[1];
    Encode::from_to($v, 'UTF-8', "CP$codepade");

    while (my ($k, $v) = each %$strings) {
        next if $k !~ /^(.+)\004(.+)$/;
        my $msgctxt = $1;
        my $msgid = $2;

        foreach my $line (@res) {
            if ($line =~ /^\s*(?:$keys)/) {
                $line =~ s/\bNC_\(\s*"\Q$msgctxt\E"\s*,\s*"\Q$msgid\E"\s*\)/"$v"/g;
            }
        }
    }

    while (my ($k, $v) = each %$strings) {
        next if $k =~ /^(.+)\004(.+)$/;
        foreach my $line (@res) {
            if ($line =~ /^\s*(?:$keys)/) {
                $line =~ s/"\Q$k\E"/"$v"/g;
            }
        }
    }

    foreach my $line (@res) {
        $line =~ s/\Q#pragma code_page(1252)\E/#pragma code_page($codepade)/;
        $line =~ s/VALUE "Translation", 0x409, 1252/VALUE "Translation", 0x$lang_id, $codepade/;
    }
    print "Saving as $res_dir/celestia_$lang.rc";
    open OUT, "> $res_dir/celestia_$lang.rc";
    print OUT join("\r\n", @res);
    close OUT;
    my $outfile = compile_rc($lang_id, "$res_dir/celestia_$lang.rc");
    link_dll($machine, $outfile, "$dll_dir/res_$lang.dll");
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

sub compile_rc {
    my $windres = $ENV{'RC'} || 'rc';
    my $language = shift;
    my $in = shift;
    my $include = dirname $in;
    my $outfile;
    my $out = $in;
    if ($mingw) {
        $out =~ s/rc$/o/;
        system qq{$windres -l $language -D NDEBUG -o $out -I $include -I $rc_dir -i $in};
    } else {
        $out =~ s/rc$/res/;
        print qq{$windres /l $language /d NDEBUG /fo $out /i $include $in};
        system qq{$windres /l $language /d NDEBUG /fo $out /i $include $in};
    }
    return $out;
}

sub link_dll {
    my $linker = $ENV{'LINKER'} || 'link';
    my $machine = shift;
    my $in = shift;
    my $out = shift;
    if ($mingw) {
        system qq{$linker -shared -o $out $in};
    } else {
        print qq{$linker /nologo /noentry /dll /machine:$machine /out:$out $in};
        system qq{$linker /nologo /noentry /dll /machine:$machine /out:$out $in};
    }
}
