#!/usr/bin/perl

# buildcrossidx.pl by Andrew Tribick
# version 1.0 - 2008-08-26

open XIDS, '<', 'crossids.txt';

@HDtoHIP  = ();
@HDlevels = ();
@SAOtoHIP = ();

while($curLine = <XIDS>) {
	chomp $curLine;

	# get Hipparcos designation
	$HIP = '';
	$HIP = $2 if($curLine =~ m/(^|:)HIP ([0-9]+)(:|$)/);
	next if($HIP eq ''); # ignore entries which are not Hipparcos stars

	@dList = split(/:/, $curLine);
	for($i = 0; $i <= $#dList; $i++) {
		# no component identifiers on SAO designations - makes things easy...
		$SAOtoHIP{$1} = $HIP if($dList[$i] =~ m/^SAO ([0-9]+)$/);

		# only use HD designations with component identifiers A,J or none
		if($dList[$i] =~ m/^HD ([0-9]+)([AJ]?)$/) {
			$HDnum = $1;
			$Level = HDlevel($2);
			if(!exists($HDtoHIP{$HDnum})) {
				# if this HD number is not already assigned, add it to list
				$HDtoHIP{$HDnum}  = $HIP;
				$HDlevels{$HDnum} = $Level;
			} elsif($Level > $HDlevels{$HDnum}) {
				# otherwise we prefer A over none over J.
				$HDtoHIP{$HDnum}  = $HIP;
				$HDlevels{$HDnum} = $Level;
			}
		}
	}
}

close XIDS;

# write out HD index file
open HDX,  '>', 'hdxindex.dat';
binmode HDX;
print HDX pack('a8S', 'CELINDEX', 0x0100);
foreach $HD (sort { $a <=> $b } keys %HDtoHIP) {
	print HDX pack('LL', $HD, $HDtoHIP{$HD});
}
close HDX;

# write out SAO index file
open SAOX, '>', 'saoxindex.dat';
binmode SAOX;
print SAOX pack('a8S', 'CELINDEX', 0x0100);
foreach $SAO (sort { $a <=> $b } keys %SAOtoHIP) {
	print SAOX pack('LL', $SAO, $SAOtoHIP{$SAO});
}
close SAOX;

# ---END OF MAIN PROGRAM---

sub HDlevel {
	# return a score based on component identifier
	my $d = shift;
	return 0 if($d eq 'J');
	return 1 if($d eq '');
	return 2 if($d eq 'A');
	return -999;
}