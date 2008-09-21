#!/usr/bin/perl

# Builds a SIMBATCH script to get HIP identifiers for CHARM2 stars
# should be run in the same folder as charm2.dat

open CHARM2, '<', 'charm2.dat' or die "Could not open file charm2.dat\n";

%Sources = ();
%SAOs = ();

while($curLine = <CHARM2>) {
	chomp $curLine;
	next if($curLine eq '');
	$Type = Trim(substr($curLine, 25, 4));
	next if($Type eq 'UR');
	$UD = Trim(substr($curLine, 143, 6));
	$LD = Trim(substr($curLine, 162, 6));
	next if(($UD eq '') && ($LD eq ''));
	@Ids = (
		Trim(substr($curLine, 30, 24)),
		Trim(substr($curLine, 54, 24)),
		Trim(substr($curLine, 79, 24)),
		Trim(substr($curLine, 104, 24))
	);
	$Source = $Ids[$0];
	$Source =~ s/\s//g;
	if(!exists $Sources{$Source}) {
		$numStars++;
		$Sources{$Source} = $Ids[$0];
	} else {
		$Sources{$Source} = $Ids[$0] if($Source ne $Ids[$0]);	
	}
}

close CHARM2;

open SIMSCRIPT, '>', 'charm2.simbatch' or die "Could not write script file to charm2.simbatch\n";
open IDLIST, '>', 'idlist.txt' or die "Could not write identifiers list to idlist.txt\n";

print SIMSCRIPT "format obj \"\%IDLIST(S;HIP)\\n\"\n\n";

foreach $s (sort keys %Sources) {
	print SIMSCRIPT "$Sources{$s}\n";
	print IDLIST "$s\n";
}

close SIMSCRIPT;
close IDLIST;

sub Trim {
	$st = shift;
	$st =~ s/^\s+//;
	$st =~ s/\s+$//;
	return $st;
}
