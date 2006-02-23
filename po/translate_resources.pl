#!/usr/bin/perl

# Produces a translated resource file in the src/celestia/res/ directory
# from each po file in the current directory.

my $resource_file = "../src/celestia/res/celestia.rc";
my $output_dir = "../src/celestia/res/";

opendir(DIR, ".");
my @po_files = sort (grep( /\.po$/, readdir(DIR) ));
closedir DIR;

open RES, "< $resource_file";
my $resource;
{ 
    local $/;
    $resource = <RES>;
}
close RES;

foreach my $po (@po_files) {
    my $strings = load_po_strings($po);
    my $res = $resource;
    while (my ($k, $v) = each %$strings) {
        $res =~ s/"\Q$k\E"/"$v"/g;
    }
    $res=~ s/\Q#pragma code_page(1252)\E/#pragma code_page(65001)/;
    my $lang = $po;
    $lang =~ s/\..*//o;
    open OUT, "> $output_dir/celestia_$lang.res";
    print OUT $res;
    close OUT;
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