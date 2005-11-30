#!/usr/bin/perl

use Math::Libm ':all';

open(BINDAT,"<doubles99.txt")|| die "Can not read doubles99.txt\n";
open(ELMTS, ">visualbins.stc") || die "Can not create visualbins.stc\n";

# boilerplate
($ver = "Revision: 1.2 ") =~ s/\$//g;
($me = $0) =~ s/.*\///;
($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst) = gmtime;
$year += 1900;
$mon += 1;
print ELMTS "# Celestia binary orbit elements: $me $ver\n";
print ELMTS "# generated from S. Söderhjelm,\n";
print ELMTS "# Astronomy and Astrophysics, v.341, p.121-140 (1999)\n";
print ELMTS "# http://adsabs.harvard.edu/cgi-bin/nph-bib_query?1999A%26A...341..121S\n";
print ELMTS "# RA, Dec & primary's spectral classes from the Hipparcos catalog (stars.txt)\n";
print ELMTS "# cross reference index from table6 of Söderhjelm(1999)\n";
print ELMTS "# For the time being: approximate standard visual magnitude (V_Johnson) by Hp,\n";
print ELMTS "# the Hipparcos visual magnitude\n";
print ELMTS "# Blundly assume unknown secondary spectral classes to be G2V.\n";
print ELMTS "# Just to have something there, until respective analysis is finished\n";
print ELMTS "# Processed $year-$mon-$mday $hour:$min:$sec UTC\n";
print ELMTS "# by Dr. Fridger Schrempp, fridger.schrempp\@desy.de\n";
print ELMTS "# ------------------------------------------------------ \n";
print ELMTS "\n";

#
# constants
#
$pi = 3.14159265359;
$ly2AU = 63239.7;
$d_sol = 1.0/$ly2AU; # in ly
$m_sol = -26.73; # apparent magnitude
$solfac = 10**(0.4*($m_sol+$bmag_correctionGV[2]))/$d_sol**2; # 1/L_sol
$c = $ly2AU/3600.0*$pi/180.0; # conversion a["] -> a[ly]
#
open(CROSS,"<table6.txt")    || die "Can not read table6.txt\n";
while (<CROSS>){
chop();
($cross,$hipnmb,$tmp) = split (/\|/);

# squeeze out all spaces and use as a key

$hipnmb =~ s/ //g;
$cross =~ s/\s*$//g;
$alt{$hipnmb,'A'} = $cross ne ""?"$cross A":"HIP$hipnmb A";
$alt{$hipnmb,'B'} = $cross ne ""?"$cross B":"HIP$hipnmb B";
$alt{$hipnmb,'AB'} = $cross ne ""?"$cross":"HIP$hipnmb";
#print STDOUT "$alt{$hipnmb}\n";
}
close (CROSS);

#
# read in the entire stars.txt for fast lookup
#
open(HIP,"<stars.txt")|| die "Can not read stars.txt\n";
while (<HIP>){
chop();
$line = $_;
($hipnr,$tmp) = split (/  /);

# squeeze out all spaces and use as a key

$hipnr =~ s/ //g;
$stars{$hipnr} = $line;

#print STDOUT "$stars{$hipnr}\n";
}
close (HIP);

$count =0;
while (<BINDAT>) {
next if (/^R.*$/);
chop();
($hip,$n_hip,$Hp1,$m2_m1,$V_I,$plx,$plxHIP,$Hpa,$Msum,$q,$Per,$T,$a,$e,$i,$omega,$OMEGA,$recno) = split (/\|/,$_);
# squeeze out all superfluous spaces
$hip=~s/ //g;
$n_hip=~s/ //g;
$plx=~s/ //g;
$q=~s/ //g;
next if($plx eq "");
$d=1000/($ly2AU*$plx)*3600/$pi*180; # d in [ly]; $plx in [mas]
next if ($q eq "");
$m1 = $Msum/(1.0+$q);
$m2 = $m1*$q;

$a2 = $d*$c*$a/(1.0 + $q); # a2 [ly]
$a1 = $a2*$q;              # a1 [ly]
next if ($n_hip eq "b");
next if ($stars{$hip} eq "");

# eliminate certain binaries that are already included in
# Grant Hutchison's 'nearstars.stc' file
next if ($hip =~ /110893/);
next if ($hip =~/71683/); # ALF Cen
next if ($hip =~/72659/);
next if ($hip =~/88601/); # 70 Oph
next if ($hip =~/30920/);
next if ($hip =~/84709/);
next if ($hip =~/82817/);
next if ($hip =~/87409/);

#
# extract distance [ly] from 'stars.txt'
# use it to compile absolute magnitude
#

($h,$c1,$c2,$d,$magapp,$color) = split(/[ \t]+/,$stars{$hip});
# coordinates in decimal-degrees

$c1 =~ s/ //g;
$c2 =~ s/ //g;
$color =~ s/ //g;
#&RotOrbits(101.28855,-16.713142,50.09,7.5,136.53,44.57,1894.13,0.59,147.27,8.6);
#print STDOUT "$Period $SemiMajorAxis $Eccentricity $Inclination $AscendingNode $ArgOfPeri $MeanAnomaly\n";
&RotOrbits($c1,$c2,$Per,$a,$i,$OMEGA,$T,$e,$omega,$d);
print  ELMTS "Barycenter \"$alt{$hip,'AB'}\"\n";
print  ELMTS "{\n";
printf ELMTS "RA       %10.6f\n", $c1;
printf ELMTS "Dec      %10.6f\n",$c2;
printf ELMTS "Distance %10.6f\n",$d;
print  ELMTS "}\n\n";
print  ELMTS "\n";
print  ELMTS "$hip \"$alt{$hip,'A'}\" \# component A\n";
print  ELMTS "{\n";
print  ELMTS "OrbitBarycenter \"$alt{$hip,'AB'}\"\n";
print  ELMTS "SpectralType \"$color\"\n";
print  ELMTS "AppMag $Hp1\n";
print  ELMTS "\n";
print  ELMTS "        EllipticalOrbit {\n";
printf ELMTS "                Period          %10.3f\n",$Period;
printf ELMTS "                SemiMajorAxis   %10.3f \# mass ratio %4.2f : %4.2f\n",$a1,$m1,$m2;
printf ELMTS "                Eccentricity    %10.3f\n",$Eccentricity;
printf ELMTS "                Inclination     %10.3f\n",$Inclination;
printf ELMTS "                AscendingNode   %10.3f\n",$AscendingNode;
$ArgOfPeri1 = $ArgOfPeri - 180;
if ($ArgOfPeri1 < 0.0) { $ArgOfPeri1 = $ArgOfPeri + 180; }
printf ELMTS "                ArgOfPericenter %10.3f\n",$ArgOfPeri1;
printf ELMTS "                MeanAnomaly     %10.3f\n",$MeanAnomaly;
print  ELMTS "        }\n";
print  ELMTS "}\n\n";
print  ELMTS "\"$alt{$hip,'B'}\" \# component B\n";
print  ELMTS "{\n";
print  ELMTS "OrbitBarycenter \"$alt{$hip,'AB'}\"\n";
print  ELMTS "SpectralType \"?\"\n";
$Hp2 = $Hp1 + $m2_m1;
print  ELMTS "AppMag $Hp2\n";
print  ELMTS "\n";
print  ELMTS "        EllipticalOrbit {\n";
printf ELMTS "                Period          %10.3f\n",$Period;
printf ELMTS "                SemiMajorAxis   %10.3f \# mass ratio %4.2f : %4.2f\n",$a2,$m1,$m2;
printf ELMTS "                Eccentricity    %10.3f\n",$Eccentricity;
printf ELMTS "                Inclination     %10.3f\n",$Inclination;
printf ELMTS "                AscendingNode   %10.3f\n",$AscendingNode;
printf ELMTS "                ArgOfPericenter %10.3f\n",$ArgOfPeri;
printf ELMTS "                MeanAnomaly     %10.3f\n",$MeanAnomaly;
printf ELMTS "        }\n";
print  ELMTS "}\n\n";
$count++;
}
print STDOUT "$count\n";

sub RotOrbits {

my($ra_deg,$del_deg,$P,$a_arcsec,$i,$PA_of_Node,$Epoch_of_peri,$e,$Arg_of_peri
,$dist_ly) = @_;
my $del_rad = -$del_deg*$pi/180.0;
my $ra_rad = $ra_deg*$pi/180.0 - $pi;
my $eps = $pi/180.0*23.4392911;
my $ii = $pi/180.0*(90.0 - $i);
my $om = $pi/180.0*($PA_of_Node - 270.0);
my $alpha = atan(cos($ii)*cos($pi/180.0*($PA_of_Node))/(sin($ii)*cos($del_rad) -
cos($ii)*sin($del_rad)*sin($pi/180.0*($PA_of_Node)))) + $ra_rad;
if( sin($ii)*cos($del_rad)-cos($ii)*sin($del_rad)*sin($pi/180.0*$PA_of_Node) < 0 ) { $alpha = $alpha + $pi };
my $delta=asin(cos($ii)*cos($del_rad)*sin($pi/180.0*$PA_of_Node)+sin($ii)*sin(
$del_rad));
my $lambda=atan((sin($alpha)*cos($eps)+tan($delta)*sin($eps))/cos($alpha));
if( cos($alpha) < 0 ) { $lambda = $lambda + $pi };
my $beta = asin(sin($delta)*cos($eps) - cos($delta)*sin($eps)*sin($alpha));
my $alphaOm = atan(cos($om)/(-sin($del_rad))/sin($om)) + $ra_rad;
if( -sin($del_rad)*sin($om) < 0 ) { $alphaOm = $alphaOm + $pi };
my $deltaOm = asin(cos($del_rad)*sin($om));
my $lambdaOm = atan((sin($alphaOm)*cos($eps) +
tan($deltaOm)*sin($eps))/cos($alphaOm));
if( cos($alphaOm) < 0 ) { $lambdaOm = $lambdaOm + $pi };
my $betaOm = asin(sin($deltaOm)*cos($eps) -
cos($deltaOm)*sin($eps)*sin($alphaOm));
my $sign = $betaOm > 0? 1.0:-1.0;
my $dd = acos(cos($betaOm)*cos($lambdaOm - $lambda - $pi/2.0))*$sign;
$Period = $P;
$SemiMajorAxis = $dist_ly*63239.7*tan($pi/180.0*$a_arcsec/3600.0);
$Eccentricity =  $e;
$Inclination = 90 - $beta/$pi*180;
$AscendingNode = $lambda/$pi*180 + 90  - floor(($lambda/$pi*180+90)/360.0)*360;
$ArgOfPeri = $Arg_of_peri + $dd/$pi*180 - floor(($Arg_of_peri + $dd/$pi*180)/360.0)*360;
$MeanAnomaly = 360*((2000.0 - $Epoch_of_peri)/$P - floor((2000.0 - $Epoch_of_peri)/$P));
}
