#!/usr/bin/perl

use Math::Libm ':all';

open(BINDAT,"<doubles99.txt")|| die "Can not read doubles99.txt\n";
open(ELMTS, ">visualbins.stc") || die "Can not create visualbins.stc\n";

# boilerplate
($ver = "Revision: 1.6.0 ") =~ s/\$//g;
($me = $0) =~ s/.*\///;
($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst) = gmtime;
$year += 1900;
$mon += 1;
print ELMTS "# ---------------------------------------\n";
print ELMTS "# Orbits of visual binaries for Celestia: $me, $ver,\n";
print ELMTS "# ---------------------------------------\n";
print ELMTS "# from S. Soederhjelm, Astronomy and Astrophysics, v.341, p.121-140 (1999).\n";
print ELMTS "# Refereed publication freely available at\n";
print ELMTS "# http://adsabs.harvard.edu/cgi-bin/nph-bib_query?1999A%26A...341..121S\n";
print ELMTS "#\n";
print ELMTS "# Binaries within 25 ly distance have been commented out here.\n"; 
print ELMTS "# They are included in Celestia's 'nearstars.stc'.\n";
print ELMTS "# This update features SIMBAD-compatible nomenclature for all barycenters.\n"; 
print ELMTS "# Leading and alternative star designations were extracted via SIMBAD's\n"; 
print ELMTS "# scripting facility, available at http://simbad.u-strasbg.fr/simbad/,\n";
print ELMTS "# and from Celestia's 'starnames.dat'.\n";
print ELMTS "# Missing spectral types for components were added from scanning\n";  
print ELMTS "# 'The Washington Visual Double Star Catalog (WDS) (Mason+ 2001-2009)',\n";
print ELMTS "# http://cdsarc.u-strasbg.fr/viz-bin/Cat?B/wds,\n";
print ELMTS "# and from redoubled entries in Celestia's 'spectbins.stc'\n"; 
print ELMTS "# \n";
print ELMTS "# Coordinates and distances for barycenters and numerous spectral types are from\n"; 
print ELMTS "# Celestia's stars, ('stars.txt' merged with 'revised.stc'), based on\n";
print ELMTS "# Floor van Leeuwen, 2007 'Hipparcos, the New Reduction of the Raw Data',\n";
print ELMTS "# Astrophysics & Space Science Library #350.\n";
print ELMTS "# available at http://cdsarc.u-strasbg.fr/viz-bin/Cat?I/311\n";
print ELMTS "#\n";
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
$c = $ly2AU/3600.0*$pi/180.0; # conversion a["] -> a[ly]
$cons='And|Cap|Col|Dra|Lac|Mus|Psc|Tau|Ant|Car|Com|Eql|Leo|Nor|Pup|Tel|Aps|Cas|CrA|Eri|Lep|Oct|Pyx|TrA|Aql|Cen|CrB|For|Lib|Oph|Ret|Tri|Aqr|Cep|Crt|Gem|LMi|Ori|Scl|Tuc|Ara|Cet|Cru|Gru|Lup|Pav|Sco|UMa|Ari|Cha|Crv|Her|Lyn|Peg|Sct|UMi|Aur|Cir|CVn|Hor|Lyr|Per|Ser|Vel|Boo|CMa|Cyg|Hya|Men|Phe|Sex|Vir|Cae|CMi|Del|Hyi|Mic|Pic|Sge|Vol|Cam|Cnc|Dor|Ind|Mon|PsA|Sgr|Vul';

@commenting_out = ("","#  Star already in 'nearstars.stc', since d < 25 ly\n#\n", "# Commented out to avoid redoubling\n#\n");

@quality = ("","","\# lacking, used HIP data of the entire system!", "\# mentioned in D.Pourbaix's paper (discussion)");

# 
# check for matching entries in Pourbaix' spectroscopic data
#
open(SPECT,"<Pourbaix-stars.txt")|| die "Can not read Pourbaix-stars.txt\n";
while (<SPECT>){
    chop();
    next if (/^#/);
    s/_+//g;
    ($hipSp,$nameSp,$hd,$mvA,$colorSpA,$mvB,$colorSpB,$type,$ref) = split(/\|/); 
    $colorSpA{$hipSp} = $colorSpA;
    $colorSpB{$hipSp} = $colorSpB;  
    $mvSpA{$hipSp} = $mvA;
    $mvSpB{$hipSp} = $mvB;
    
    #print "$hipSp  $altSp{$hipSp} $colorSpA{$hipSp}  $colorSpB{$hipSp}\n";
}
close (SPECT);
#
# implement matching Celestia starnames
#
open(STN,"<starnames.dat")|| die "Can not read starnames.dat\n";
while (<STN>){
    chop();
    # extract main name
    ($hipStn, $name1, $dummy) = ($_ =~ /(^\d+):([a-zA-Z]+\s?[a-zA-Z]{4,}):(.*)/);
     # is there a second name?
    ($name2, $dummy2) = ($dummy =~ /(^[a-zA-Z]+\s?[a-zA-Z]{4,}):(.*)/);
    if ($name1){
        $name1A = "$name1 A";
        $name1B = "$name1 B";
    }
    if ($name2){
        $name2  = ":$name2";
        $name2A = "$name2 A";
        $name2B = "$name2 B";
    } else {
        $name2A = "";
        $name2B = "";
    }
    $Stn{$hipStn}  = $name1.$name2;
    $StnA{$hipStn} = $name1A.$name2A;
    $StnB{$hipStn} = $name1B.$name2B;
    
    #print "$hipStn      $Stn{$hipStn}  $StnA{$hipStn}  $StnB{$hipStn}\n" if $Stn{$hipStn};
}
close (STN);
#
# implement SIMBAD naming compatibility, via SIMBATCH ('simbad.txt' output)
#
open(SIMBAD,"<simbad_vis.txt")|| die "Can not read simbad_vis.txt\n";
while (<SIMBAD>){
    chop();
    $alt  = ""; $altA = ""; $altB = ""; $nx = ":"; $comp="";    
    if (/^Identifiers=/){
        if (/\|HIP (\d+)\|/){$hipSim = $1;}
        if (/\|.*\s([a-zA-Z]+ )($cons)(\s?[A-B]?)\|/){
            $comp = $3;
            $constell = $2;
            $pre  = $1;            
            $pre  =~ tr/a-z/A-Z/;
            $pre  =~ s/KSI/XI/g;
            &update ($pre, $constell, $comp);
        }                
        if (/\|V\*\s(V\d+ )($cons)(\s?[A-B]?)\|/){             
            $comp = $3;
            $constell = $2;
            $pre  = $1;
            &update ($pre, $constell, $comp);           
        }
        if (/\|.*\s(\w+\.0\d )(($cons))/){
            $constell = $2;
            $pre  = $1;            
            $pre  =~ s/\.01/1/g;
            $pre  =~ s/\.02/2/g;
            $pre  =~ tr/a-z/A-Z/;
            $pre  =~ s/KSI/XI/g;
            &update ($pre, $constell, " ");
        }             
        if (/\|.?\s(\d+ )($cons)(\s?[A-B]?)\|/){             
            $comp = $3;
            $constell = $2;
            $pre  = $1;
            &update ($pre, $constell, $comp);
        }                       
        if (/\|(GJ \d+)(\s?[A-C]*)\|/){
            $pre = $1;
            $comp = $2;
            &update ($pre, "", $comp);
        }
        if (/\|(ADS \d+)(\s?[A-P]*)\|/){
            $pre = $1;            
            $comp = $2;            
            &update ($pre, "", $comp);
        }        
        if (/\|(CCDM\sJ\d+[\+\-]\d+)([A-P]*)\|/){
            $pre = $1;
            $comp = $2;
            &update ($pre, "", $comp);
        }
        if (/\|(p\.\sWDS\sJ\d+[\+\-]\d+)([A-P]*)\|/){
            $pre = $1;
            $comp = $2;
            # &update ($pre, "", $comp);
            $pre =~ s/p\.\sWDS\sJ//g;
            $wdsSim{$hipSim} = $pre;
            #print "$hipSim  $wdsSim{$hipSim}\n";
        }
        
             
        $alt{$hipSim, 'AB'} = $alt?$alt:"HIP $hipSim";       
        $alt{$hipSim, 'A'}  = $altA?$altA:"HIP $hipSim A";
        $alt{$hipSim, 'B'}  = $altB?$altB:"HIP $hipSim B";
        
        if ($Stn{$hipSim}){
            $alt{$hipSim, 'AB'} =  $Stn{$hipSim}.$nx.$alt{$hipSim, 'AB'};
            $alt{$hipSim, 'A'}  =  $StnA{$hipSim}.$nx.$alt{$hipSim, 'A'};
            $alt{$hipSim, 'B'}  =  $StnB{$hipSim}.$nx.$alt{$hipSim, 'B'};
        }         
                 
        ($orbitRef{$hipSim},$tmp) = split(":",$alt{$hipSim, 'AB'});
    }
}     
close(SIMBAD);
#
# read in WDS looking for spectral types
#
open(WDS,"< wds.dat")|| die "Can not read wds.dat\n";
while (<WDS>){
    chop();
    $wds     = &clean(&ss(1,10));
    ($sptype1, $sptype2) = split(/[\/ \+]+/,&clean(&ss(71,80)));
    
    # ($sptype1, $sptype2)
    $sptype1{$wds} = $sptype1; 
    $sptype2{$wds} = $sptype2;
}
close(WDS);
#
# read in the entire stars.txt for fast lookup
#
open(HIP,"< stars.txt")|| die "Can not read stars.txt\n";
while (<HIP>){
    chop();
    $line = $_;
    ($hipnr,$tmp) = split (/  /);

    # squeeze out all spaces and use as a key

    $stars{$hipnr} = &clean($line);
}
close (HIP);

#
# merge with corrections in revised.stc
#
open(HIPREV,"<revised.stc")|| die "Can not read revised.stc\n";
while (<HIPREV>){
    next if (/^#/);
    if (/(^\d+)/ || /^Modify (\d+)/){
        $hiprev = $1; 
        ($h,$c1,$c2,$dd,$magapp,$color) = split(/[ \t]+/,$stars{$hiprev});
#        if (!$stars{$hiprev}){
#            print "ORG: HIP star missing!\n";
#        } else {    
#            print "ORG: HIP $stars{$hiprev}\n";
#        }
        next;
    }
    next if(/^\{/);
    if(/RA\b\s+([\d.]+)/){$c1              = $1; next;} 
    if(/Dec\b\s+([-\d.]+)/){$c2            = $1; next;}
    if(/Distance\b\s+([\d.]+)/){$dd        = $1; next;}
    if(/SpectralType\b\s+\"(.*)\"/){$color = $1; next;}
    if(/AppMag\b\s+([\d.]+)/){$magapp      = $1; next;}
    if (/^\}/){        
        $stars{$hiprev} = join (" ",$hiprev,$c1,$c2,$dd,$magapp,$color);
#       print "REV: HIP $stars{$hiprev}\n\n"; 
    }   
}
close (HIPREV);

$count = 0;
$k = 0;
$kAwds = 0; $kBwds = 0; $kAsb = 0; $kBsb = 0; $kAhip = 0; $kBhip = 0;
while (<BINDAT>) {
	next if (/^R.*$/);
	chop();
	($hip,$n_hip,$Hp1,$dm2m1,$V_I,$plx,$plxHIP,$Hpa,$Msum,$q,$Per,$T,$a,$e,$i,$omega,$OMEGA,$recno) = split (/\|/,$_);
	# squeeze out all superfluous spaces
	$hip   =~s/ //g;
	$n_hip =~s/ //g;
	$plx   =~s/ //g;
	$q     =~s/ //g;
    $dm2m1 =~s/ //g;
       
	next if($plx eq "");
    $d=1000/($ly2AU*$plx)*3600/$pi*180; # d in [ly]; $plx in [mas]
	next if ($q eq "");
	$m1 = $Msum/(1.0+$q);
	$m2 = $m1*$q;

	$a2 = $d*$c*$a/(1.0 + $q); # a2 [ly]
	$a1 = $a2*$q;              # a1 [ly]
	next if ($n_hip eq "b");
	next if ($stars{$hip} eq "");
    
    $comment_flag = 0;
    $onoff = "";
    #
	# extract distance [ly] from 'stars.txt & revised stc'
	# use it to compile absolute magnitude
	#
	($h,$c1,$c2,$dd,$magapp,$color) = split(/[ \t]+/,$stars{$hip});
	#       
    # coordinates in decimal-degrees
    #
    $c1 =~ s/ //g;
    $c2 =~ s/ //g;
    
    #
    # app.mag of secondary
    #
    $Hp2 = $Hp1 + $dm2m1;
    
    #print "$hip  $Hp1  $dm2m1  $Hp2\n"; 
        
    $epsrel = 0;	
    if($d){ $epsrel = 100 * ($dd - $d)/$d;}
	if($epsrel > 10){
		print STDOUT "Distance mismatch of $epsrel % with (revised) stars.txt for HIP $hip\n";	    
	}
	$d = $dd;
    #
    # comment out all binaries with earthbound distance <= 25 ly. 
    # They are included in Grant Hutchison's 'nearstars.stc' file
    #        
    if ($d <= 25){
        $onoff = "# ";
        $comment_flag = 1;
        #print "$hip   $d\n";
    }             

	$color =~ s/ //g;
    
    if ($color =~ /(\w+)\/(\w+)/){
        $hipcolorA = $1;
        $hipcolorB = $2;
    } else {
        $hipcolorA = $color;
        $hipcolorB = $color;
    }     
    #
    # separate spectral types of both components
    #         
    if ($sptype1{$wdsSim{$hip}}){                     
       # scan first the WDS catalog for spectral types         
        $spectA = $sptype1{$wdsSim{$hip}};
        $quA = $quality[0];
        $kAwds++ if !$onoff;
    } elsif ($colorSpA{$hip}){
        # use up next the known spectral types from D. Pourbaix 
        $spectA = $colorSpA{$hip};
        $quA = $quality[1];
        #print "$hip A-color: $spectA\n";
        $kAsb++ if !$onoff;
    } else {
        # identify the leftovers with the system spectral type from HIP
        $spectA = $hipcolorA;
        $quA = $quality[2];
        $kAhip++ if !$onoff;
    }   
    if ($sptype2{$wdsSim{$hip}}){
        # scan first the WDS catalog for spectral types                 
        $spectB = $sptype2{$wdsSim{$hip}};
        $quB = $quality[0];
        $kBwds++ if !$onoff;
    } elsif ($colorSpB{$hip}){
        # use up next the known spectral types from D. Pourbaix 
        $spectB = $colorSpB{$hip};
        $quB = $quality[1];
        #print "$hip B-color: $spectB\n";
        $kBsb++ if !$onoff;
    } else {
        # identify the leftovers with the system spectral type from HIP
        $spectB = $hipcolorB;
        $quB = $quality[2];
        $kBhip++ if !$onoff;
    }
    # cleaning up for Celestia
    $spectA =~ s/[a-z]|\.{2,}|\-I?V|:|\+//g;
    $spectB =~ s/[a-z]|\.{2,}|\-I?V|:|\+//g;
    $spectA = "\"$spectA\"";
    $spectB = "\"$spectB\"";
    
    #printf "%2s %6d %10.2f %12s %8s %8s %8s %8s\n", $onoff, $hip, $d,  $sptype1{$wdsSim{$hip}},$sptype2{$wdsSim{$hip}},$colorSpA{$hip},$colorSpB{$hip},$hipcolorB; 
    
    #printf "%2s %6d %15s %10.2f %10.2f %5s\n", $onoff, $hip, $wdsSim{$hip}, $d, $Hp1, $Hp2; 
        
    #if ($alt{$hip,'AB'} =~/($cons)/){$k++; print "$k   $alt{$hip,'AB'}\n";}
    #print STDOUT "$Period $SemiMajorAxis $Eccentricity $Inclination $AscendingNode $ArgOfPeri $MeanAnomaly\n";
    
    &RotOrbits($c1,$c2,$Per,$a,$i,$OMEGA,$T,$e,$omega,$d);
	print  ELMTS "$onoff Barycenter $hip \"$alt{$hip,'AB'}\"\n";
	print  ELMTS "$onoff {\n";
	printf ELMTS "$onoff     RA       %10.6f\n", $c1;
	printf ELMTS "$onoff     Dec      %10.6f\n",$c2;
	printf ELMTS "$onoff     Distance %10.6f\n",$d;
    print  ELMTS "$onoff }\n";
	print  ELMTS "$onoff \n";
	print  ELMTS "$onoff \"$alt{$hip,'A'}\"\n";
	print  ELMTS "$onoff {\n";
	print  ELMTS "$onoff     OrbitBarycenter \"$orbitRef{$hip}\"\n";
    printf ELMTS "$onoff     SpectralType    %-8s %-40s\n",$spectA,$quA;
    printf ELMTS "$onoff     AppMag          %-5.2f\n",$Hp1;  	
    print  ELMTS "$onoff \n";
	print  ELMTS "$onoff     EllipticalOrbit {\n";
	printf ELMTS "$onoff         Period          %10.3f\n",$Period;
	printf ELMTS "$onoff         SemiMajorAxis   %10.3f \# mass ratio %4.2f : %4.2f\n",$a1,$m1,$m2;
	printf ELMTS "$onoff         Eccentricity    %10.3f\n",$Eccentricity;
	printf ELMTS "$onoff         Inclination     %10.3f\n",$Inclination;
	printf ELMTS "$onoff         AscendingNode   %10.3f\n",$AscendingNode;
	$ArgOfPeri1 = $ArgOfPeri - 180;
	if ($ArgOfPeri1 < 0.0) { $ArgOfPeri1 = $ArgOfPeri + 180; }
	printf ELMTS "$onoff         ArgOfPericenter %10.3f\n",$ArgOfPeri1;
	printf ELMTS "$onoff         MeanAnomaly     %10.3f\n",$MeanAnomaly;
	print  ELMTS "$onoff     }\n";
	print  ELMTS "$onoff }\n\n";
	print  ELMTS "$onoff \"$alt{$hip,'B'}\"\n";
	print  ELMTS "$onoff {\n";
	print  ELMTS "$onoff     OrbitBarycenter \"$orbitRef{$hip}\"\n";
	printf ELMTS "$onoff     SpectralType    %-8s %-40s\n",$spectB,$quB;    
    printf ELMTS "$onoff     AppMag          %-5.2f\n",$Hp2;
	print  ELMTS "$onoff \n";
	print  ELMTS "$onoff     EllipticalOrbit {\n";
	printf ELMTS "$onoff         Period          %10.3f\n",$Period;
	printf ELMTS "$onoff         SemiMajorAxis   %10.3f \# mass ratio %4.2f : %4.2f\n",$a2,$m1,$m2;
	printf ELMTS "$onoff         Eccentricity    %10.3f\n",$Eccentricity;
	printf ELMTS "$onoff         Inclination     %10.3f\n",$Inclination;
	printf ELMTS "$onoff         AscendingNode   %10.3f\n",$AscendingNode;
	printf ELMTS "$onoff         ArgOfPericenter %10.3f\n",$ArgOfPeri;
	printf ELMTS "$onoff         MeanAnomaly     %10.3f\n",$MeanAnomaly;
	printf ELMTS "$onoff     }\n";
	print  ELMTS "$onoff }\n\n";
	$count++ if !$onoff;
}
close (BINDAT);

print STDOUT "\nNumber of enabled visual binaries: $count\n";
print "\nA component:\n------------\n";
printf STDOUT "\nspectral types from WDS catalog: => %3d\n",$kAwds;
printf STDOUT "spectral types from 'spectbins': => %3d\n",$kAsb;
printf STDOUT "spectral types from HIP catalog: => %3d\n",$kAhip;
print "\nB component:\n------------\n";
printf STDOUT "spectral types from WDS catalog: => %3d\n", $kBwds;
printf STDOUT "spectral types from 'spectbins': => %3d\n",$kBsb;
printf STDOUT "spectral types from HIP catalog: => %3d\n\n",$kBhip;


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

sub clean {
# squeeze out superfluous spaces
my($string) = @_;
$string =~ s/^\s*//;
$string =~ s/\s*$//;
$string =~ s/\s+/ /g;
$string;
}

# like substr($_,first,last), but one-based.
sub ss {
    substr ($_, $_[0]-1, $_[1]-$_[0]+1);
}
sub update {
my($pre, $constell, $comp) = @_;
# component parser for visual binaries
my $name = $pre.$constell;
my ($a, $b) = "";

if (!$comp||$comp eq " "){$a = " A"; $b = " B"; $comp = "";}
elsif ($comp =~ /(\s?)ABC?P?/){$a = $1."A"; $b = $1."B";}
elsif ($comp =~ /\b[AB]\b/){$a = $comp."a"; $b = $comp."b";}
elsif ($comp =~ /(\s?)BC/){$a = $1."B"; $b = $1."C";}
elsif ($comp =~ /(\s?)([AB])([PC])/){$a = $1.$2; $b = $1.$3;}

$alt  = $alt?$alt.$nx.$name.$comp:$name.$comp;
$altA = $altA?$altA.$nx.$name.$a:$name.$a;
$altB = $altB?$altB.$nx.$name.$b:$name.$b;
}

