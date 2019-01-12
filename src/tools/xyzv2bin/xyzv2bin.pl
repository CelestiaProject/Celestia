#!/usr/bin/perl

use warnings;
use strict;
use Config;
use Scalar::Util qw(looks_like_number);

die "Syntax: xyzv2bin.pl <infile> <outfile>\n" if($#ARGV != 1);

my $MAGIC = "CELXYZV";
my $byte_order = $Config{byteorder} == 12345678 ? 1234 : $Config{byteorder};
my $digits = $Config{doublemantbits} + 1;
my $reserved = 0;
my $count = -1; # unknown

open(INFILE, '<', $ARGV[0]) || die "Unable to open $ARGV[0] for reading\n";
open(OUTFILE, '>', $ARGV[1]) || die "Unable to open $ARGV[1] for writing\n";
binmode OUTFILE;

# only byte_order is in LE order, other values are in native order
my $header = pack "Z8vslq", ($MAGIC, $byte_order, $digits, $reserved, $count);
print OUTFILE $header;

my @values;
my $chunk;
my $line;
$count = 0;

outer:
while($line = <INFILE>)
{
    # remove leading and terminating spaces
    $line =~ s/^\s+|\s+$//g;

    # skip empty lines
    next if length $line == 0;

    # remove comments
    my $pos = index $line, '#';
    if ($pos > -1)
    {
        next if $pos == 0;
        $line = substr $line, 0, $pos;
    }

    @values = split /\s+/, $line;
    if ($#values != 6)
    {
        print "Skipping bad line: $line\n";
        next;
    }

    while (my $v = each @values)
    {
        unless (looks_like_number($v))
        {
          print "Skipping bad line: $_\n";
          next outer;
        }
    }

    $chunk = pack "d7", @values;
    print OUTFILE $chunk;
    ++$count;
}

seek OUTFILE, 0, 0;

# print the header once again to save actual number or records
$header = pack "Z8vslq", ($MAGIC, $byte_order, $digits, $reserved, $count);
print OUTFILE $header;

close OUTFILE;
close INFILE;
