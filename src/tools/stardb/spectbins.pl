#!/usr/bin/perl 
# Author: Fridger.Schrempp@desy.de
use Math::Libm ':all';

open(ELMTS, ">spectbins.stc") || die "Can not create spectbins.stc\n";
# boilerplate
($ver = "Revision: 1.00 ") =~ s/\$//g;
($me = $0) =~ s/.*\///;
($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst) = gmtime;
$year += 1900;
$mon += 1;
print ELMTS "# Celestia binary orbit elements for spectroscopic binaries:\n"; 
print ELMTS "# generated with Perl extraction script: $me $ver\n";
print ELMTS "# from D. Poubaix, Astron. Astrophys. Suppl. Ser. 145, 2000, 215-222\n";
print ELMTS "# http://www.edpsciences.org/journal/index.cfm?v_url=aas/full/2000/14/ds9259/ds9259.html\n";
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
$count = 0;

open(HIP,"<stars.txt")|| die "Can not read stars.txt\n";
while (<HIP>){
chop();
$line = $_;
($hipnr1,$tmp) = split (/  /);

# squeeze out all spaces and use as a key

$hipnr1 =~ s/ //g;
$stars{$hipnr1} = $line;

#print STDOUT "$stars{$hipnr1}\n";
}
close (HIP);

open(BINDAT1,"<Pourbaix-stars.txt")|| die "Can not read Pourbaix-stars.txt\n"; 
while (<BINDAT1>){
chop();
next if (/^#/);
$line = $_;
($hipnr,$tmp) = split (/\|/);

# squeeze out all spaces and use as a key

$hipnr =~ s/_| //g;
$spects{$hipnr} = $line;
}
close (BINDAT1);

open(BINDAT2,"<Pourbaix-orbits.txt")|| die "Can not read Pourbaix-orbits.txt\n"; 
while (<BINDAT2>) {
chop();
next if (/^#/);
($hip,$a,$i,$omega,$OMEGA,$e,$Per,$T,$v0,$plx,$kappa,$MA,$MB) = split (/\|/,$_);
# squeeze out all superfluous spaces
$hip=~s/_| //g;
$a=~s/_| //g;
$i=~s/_| //g;
$omega=~s/_| //g;
$OMEGA=~s/_| //g;
$e=~s/_| //g;
$Per=~s/_| //g;
$T=~s/_| //g;
$plx=~s/_| //g;
$kappa=~s/_| //g;
$MA=~s/_| //g;
$MB=~s/_| //g;
#print STDOUT "$hip $a $i $omega $OMEGA $e $Per $T  $plx $kappa $MA $MB\n";
#exit
next if($plx eq "");
$d=1000/($ly2AU*$plx)*3600/$pi*180; # d in [ly]; $plx in [mas]
$q= $MB/$MA;
$a2 = $d*$c*$a/(1.0 + $q); # a2 [ly]
$a1 = $a2*$q;              # a1 [ly]
#
next if ($stars{$hip} eq "");
($h,$c1,$c2,$dd,$magapp,$color) = split(/[ \t]+/,$stars{$hip});
# coordinates in decimal-degrees

$c1 =~ s/ //g;
$c2 =~ s/ //g;

($hip2,$name,$hd,$mvA,$colorA,$mvB,$colorB,$type,$ref) = split(/\|/,$spects{$hip});

$name=~s/_//g;
$mvA=~s/_| //g;
$colorA=~s/_| //g;
$colorA=$colorA eq ""?"?":"$colorA";
$mvB=~s/_| //g;
$mvB=$mvB eq ""?"5.0":"$mvB";
$colorB=~s/_| //g;
$colorB=$colorB eq ""?"?":"$colorB";
#print STDOUT "$hip $name $mvA $colorA $mvB $colorB\n";
next if($plx eq "");
$d=1000/($ly2AU*$plx)*3600/$pi*180; # d in [ly]; $plx in [mas]
$q= $MB/$MA;
# $a is in mas! $a1, $a2 in arc_secs
$a2 = 0.001*$d*$c*$a/(1.0 + $q); # a2 [ly]
$a1 = $a2*$q;              # a1 [ly]
#
$alt{$hip,'A'} = $name ne ""?"$name A":"HIP$hip A";
$alt{$hip,'B'} = $name ne ""?"\"$name B\"":"HIP$hip B";
$alt{$hip,'AB'} = $name ne ""?"$name":"HIP$hip";
if ($hip == 71683){
$alt{$hip,'B'} = "71681 \"$name B\""; $mvB = 1.34; $colorB = "K0V"; }

#print STDOUT "$hip $c1 $c2 $d $a $i $omega $OMEGA $e $Per $T  $plx $kappa $MA $MB $name $mvA $colorA $mvB $colorB\n";

&RotOrbits($c1,$c2,$Per,$a,$i,$OMEGA,$T,$e,$omega,$d);
print  ELMTS "Barycenter \"$alt{$hip,'AB'}\"\n";
print  ELMTS "{\n";
printf ELMTS "RA       %10.6f\n", $c1;
printf ELMTS "Dec      %10.6f\n",$c2;
printf ELMTS "Distance %10.6f\n",$d;
print  ELMTS "}\n\n";
print  ELMTS "\n";
print  ELMTS "$hip \"$alt{$hip,'A'}\" \n";
print  ELMTS "{\n";
print  ELMTS "OrbitBarycenter \"$alt{$hip,'AB'}\"\n";
print  ELMTS "SpectralType \"$colorA\"\n";
print  ELMTS "AppMag $mvA\n";
print  ELMTS "\n";
print  ELMTS "        EllipticalOrbit {\n";
printf ELMTS "                Period          %10.3f\n",$Period;  
printf ELMTS "                SemiMajorAxis   %10.3f \# mass ratio %4.2f : %4.2f\n",$a1,$MA,$MB;
printf ELMTS "                Eccentricity    %10.3f\n",$Eccentricity;
printf ELMTS "                Inclination     %10.3f\n",$Inclination;
printf ELMTS "                AscendingNode   %10.3f\n",$AscendingNode;
$ArgOfPeri1 = $ArgOfPeri - 180;
if ($ArgOfPeri1 < 0.0) { $ArgOfPeri1 = $ArgOfPeri + 180; }
printf ELMTS "                ArgOfPericenter %10.3f\n",$ArgOfPeri1;
printf ELMTS "                MeanAnomaly     %10.3f\n",$MeanAnomaly;
print  ELMTS "        }\n";
print  ELMTS "}\n\n";
print  ELMTS "$alt{$hip,'B'}\n";
print  ELMTS "{\n";
print  ELMTS "OrbitBarycenter \"$alt{$hip,'AB'}\"\n";
print  ELMTS "SpectralType \"$colorB\"\n";
print  ELMTS "AppMag $mvB\n";
print  ELMTS "\n";
print  ELMTS "        EllipticalOrbit {\n";
printf ELMTS "                Period          %10.3f\n",$Period;  
printf ELMTS "                SemiMajorAxis   %10.3f \# mass ratio %4.2f : %4.2f\n",$a2,$MA,$MB;
printf ELMTS "                Eccentricity    %10.3f\n",$Eccentricity;
printf ELMTS "                Inclination     %10.3f\n",$Inclination;
printf ELMTS "                AscendingNode   %10.3f\n",$AscendingNode;
printf ELMTS "                ArgOfPericenter %10.3f\n",$ArgOfPeri;
printf ELMTS "                MeanAnomaly     %10.3f\n",$MeanAnomaly;
printf ELMTS "        }\n";
print  ELMTS "}\n\n";
    $count++;
}
print "$count\n";
close (BINDAT2);

sub RotOrbits {
 
my($ra_deg,$del_deg,$P,$a_arcsec,$i,$PA_of_Node,$Epoch_of_peri,$e,$Arg_of_peri
,$dist_ly) = @_;
my $del_rad = -$del_deg*$pi/180.0;
my $ra_rad = $ra_deg*$pi/180.0 - $pi;
my $eps = $pi/180.0*23.4392911;
my $ii = $pi/180.0*(90.0 - $i);
my $om = $pi/180.0*($PA_of_Node - 270.0)+1.0e-8;
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
