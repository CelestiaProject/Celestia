#!/usr/bin/perl
# Author: Fridger.Schrempp@desy.de
use Math::Libm ':all';

open(ELMTS, ">spectbins.stc") || die "Can not create spectbins.stc\n";
# boilerplate
($ver = "Revision: 1.6.0 ") =~ s/\$//g;
($me = $0) =~ s/.*\///;
($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst) = gmtime;
$year += 1900;
$mon += 1;
print ELMTS "# ---------------------------------------------\n";
print ELMTS "# Orbits of spectroscopic binaries for Celestia: $me, $ver,\n";
print ELMTS "# ---------------------------------------------\n";
print ELMTS "# from D. Pourbaix, Astron. Astrophys. Suppl. Ser. 145, 2000, 215-222.\n";
print ELMTS "# Refereed publication freely available at\n"; 
print ELMTS "# http://aas.aanda.org/index.php?option=article&access=standard&Itemid=129&url=/articles/aas/pdf/2000/14/ds9259.pdf\n";
print ELMTS "\n";
print ELMTS "# Binaries within 25 ly distance have been commented out here.\n"; 
print ELMTS "# They are included in Celestia's 'nearstars.stc'.\n";
print ELMTS "# Entries appearing also in 'visualbins.stc' are commented out here as well\n"; 
print ELMTS "# to avoid redoubling. Data on spectral types were moved to 'visualbins.stc'.\n"; 
print ELMTS "# This update features SIMBAD-compatible nomenclature for all barycenters.\n"; 
print ELMTS "# Leading and alternative star designations were extracted via SIMBAD's\n"; 
print ELMTS "# scripting facility, available at http://simbad.u-strasbg.fr/simbad/,\n";
print ELMTS "# and from Celestia's 'starnames.dat'. Missing spectral types\n";
print ELMTS "# and visual magnitudes for components were added from scanning the\n"; 
print ELMTS "# '9th Catalogue of Spectroscopic Binary Orbits (SB9) (Pourbaix+ 2004-2009)',\n";
print ELMTS "# http://cdsarc.u-strasbg.fr/viz-bin/Cat?B/sb9,\n";
print ELMTS "# Coordinates and distances for barycenters and some spectral types are from\n"; 
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

%Vis = NULL;
#
# check visual binaries for doubles <=> $Vis{$hipSim} = 1
#
open(SIMVIS,"<simbad_vis.txt")|| die "Can not read simbad_vis.txt\n";
while (<SIMVIS>){
    chop();    
    if (/^Identifiers=/){
        if (/\|HIP (\d+)\|/){$hipSim = $1;$Vis{$hipSim} = 1;}
    }
}
close(SIMVIS);    
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
        $name2 = ":$name2";
        $name2A = "$name2 A";
        $name2B = "$name2 B";
    } else {
        $name2A = "";
        $name2B = "";    
    }
    $Stn{$hipStn} = $name1.$name2;
    $StnA{$hipStn} = $name1A.$name2A;
    $StnB{$hipStn} = $name1B.$name2B;
    
    #print "$hipStn      $Stn{$hipStn}\n" if $Stn{$hipStn};
}
close (STN);
#
# implement SIMBAD naming compatibility, SIMBATCH output ('simbad_spect.txt')
#
open(SIMBAD,"<simbad_spect.txt")|| die "Can not read simbad_spect.txt\n";
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
        if (/\|.*\s(\w+\.?0\d )(($cons))/){
            $constell = $2;
            $pre  = $1;            
            $pre  =~ s/\.?01/1/g;
            $pre  =~ s/\.?02/2/g;
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
# read in SB9 looking for spectral types/mags of components
#
open(SB9,"< SB9.dat")|| die "Can not read SB9.dat\n";
while (<SB9>){
    #chop();
    $hipSb9             =  &clean(&ss(86,110));
    $hipSb9             =~ s/HIP //g;
    $colorSb9A{$hipSb9} = &clean(&ss(58,73));
    $colorSb9B{$hipSb9} = &clean(&ss(75,84));
    $mVSb9A{$hipSb9}    = &clean(&ss(42,47));
    $mVSb9B{$hipSb9}    = &clean(&ss(50,55));
    $filterA{$hipSb9}   = &ss(48);
    $filterB{$hipSb9}   = &ss(56);    
    #print "$hipSb9     $colorSb9A{$hipSb9}    $colorSb9B{$hipSb9}  $mVSb9A{$hipSb9}   $mVSb9B{$hipSb9}  $filterA{$hipSb9}  $filterB{$hipSb9}\n";
}
close(SB9);
#
# merge with corrections in revised.stc
#
open(HIP,"<stars.txt")|| die "Can not read stars.txt\n";
while (<HIP>){
    chop();
    $line = $_;
    ($hipnr1,$tmp) = split (/  /);

    # squeeze out all spaces and use as a key

    $hipnr1 =~ s/ //g;
    $stars{$hipnr1} = &clean($line);

    #print STDOUT "$stars{$hipnr1}\n";
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
#        print "REV: HIP $stars{$hiprev}\n\n"; 
    }   
}
close (HIPREV);

$count = 0;
$kAsb9 = 0; $kBsb9 = 0; $kApx = 0; $kBpx = 0; $kAhip = 0; $kBhip = 0;
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

	($hip2,$name,$hd,$mvPxA,$colorA,$mvPxB,$colorB,$type,$ref) = split(/\|/,$spects{$hip});

	$name=~s/_//g;
	$mvPxA=~s/_| //g;
	$colorA=~s/_| //g;
	$colorB=~s/_| //g;
	$mvPxB=~s/_| //g;
    # try app.mag entries from SB9 first      
    if ($mVSb9A{$hip}){
        $mquA = $quality[0];
        $mvA = $mVSb9A{$hip};
    } elsif ($mvPxA) {
        $mquA = $quality[1];
        $mvA = $mvPxA;
    }
    if ($mVSb9B{$hip}){
        $mquB = $quality[0];
        $mvB = $mVSb9B{$hip};
    } elsif ($mvPxB) {
        $mquB = $quality[1];
        $mvB = $mvPxB;
    } elsif ($hip == 10644){                         
        # HIP 10644 = DEL Tri
        # rough estimate from discussion about DEL Tri in Pourbaix's paper
        $mquB = $quality[3]; 
        $mvB = $mvA + 2.0;         
    } else {
        # identify the rest with the visual app. mag of the entire system 
        # from HIP. A rough approximation, but at this point unavoidable...
        $mquB = $quality[2];
        $mvB = $magapp; 
    }
    #print "$hip $mvA   $mvB\n"; 
	
    next if($plx eq "");
	$d=1000/($ly2AU*$plx)*3600/$pi*180; # d in [ly]; $plx in [mas]
	$q= $MB/$MA;
	# $a is in mas! $a1, $a2 in arc_secs
	$a2 = 0.001*$d*$c*$a/(1.0 + $q); # a2 [ly]
	$a1 = $a2*$q;              # a1 [ly]
	#            
	# eliminate certain binaries that are already in visualbins.stc
    # have checked that this condition includes ALL previously known cases 
    # (Cham's list)!	
    
    $comment_flag = 0;
    $onoff = "";
    if ($Vis{$hip}){
        $onoff = "# " if $Vis{$hip};
        $comment_flag = 2;
    }
    #print "$hip    $Vis{$hip}\n"  if $Vis{$hip};   
    
	#
	# extract distance [ly] from 'stars.txt & revised stc'
	# use it to compile absolute magnitude
	#
	
	$epsrel = 0;	
	if($d){ $epsrel = 100 * ($dd - $d)/$d;}
	if($epsrel > 10){
		print STDOUT "Distance mismatch of $epsrel % with (revised) stars.txt for HIP $hip\n";	    
	} 
	$d = $dd;
    #
    # comment out all binaries with earthbound distance <= 25 ly. 
    # They are included in Grant Hutchison's 'nearstars.stc' file
    # ALF Cen, 70 Oph
            
    if ($d <= 25){
        $onoff = "# ";
        $comment_flag = 1;
        #print "$hip   $d\n";
    }             
	#
    # separate spectral types of both components
    #   	   
    if ($colorSb9A{$hip}){
        # first use SB9 catalog data that are most recent...        
        $spectA = $colorSb9A{$hip};
        $quA = $quality[0];
        $kAsb9++ if !$onoff;
    } elsif ($colorA){
        # use up next the known spectral types from D. Pourbaix
        $spectA = $colorA;
        $quA = $quality[1];
        $kApx++ if !$onoff;
    } else {
        # identify the leftovers with the system spectral type from HIP
        $spectA = $color;
        $quA = $quality[2];
        $kAhip++ if !$onoff;
    }
    if ($colorSb9B{$hip}){
        # first use SB9 data that are most recent...
        $spectB = $colorSb9B{$hip};
        $quB = $quality[0];
        $kBsb9++ if !$onoff;
    } elsif ($colorB){
        # use up next the known spectral types from D. Pourbaix
        $spectB = $colorB;
        $quB = $quality[1];
        $kBpx++ if !$onoff;
    } else {
        # approximate the rest by the system spectral type from HIP
        $spectB = $color;
        $quB = $quality[2];
        $kBhip++ if !$onoff;
    }
    # cleaning up for Celestia notation     
    $spectA =~ s/[a-z]|\-I?V|\.{3,}//g;  
    $spectB =~ s/[a-zS]|\-I?V|\.{3,}//g;
    $spectA = "\"$spectA\"";
    $spectB = "\"$spectB\"";
    
    #printf "%2s %6d %10.2f %12s %8s %8s %8s %8s\n", $onoff, $hip, $d,  $colorSb9A{$hip},$colorSb9B{$hip},$colorA,$colorB,$color; 
    
    #printf "%2s %6d %10.2f %10.2f %5s %5.2f %5s %5.2f\n", $onoff, $hip, $d,  $mVSb9A{$hip},$mVSb9B{$hip},$mvPxA,$mvPxB,$magapp; 
    
    #if ($alt{$hip,'AB'} =~/($cons)/){$k++; print "$k $onoff $alt{$hip,'AB'}\n";}
    #if ($onoff){print "$hip $commenting_out[$comment_flag]\n";}
	#print STDOUT "$hip $c1 $c2 $d $a $i $omega $OMEGA $e $Per $T  $plx $kappa $MA $MB $name $mvA $colorA $mvB $colorB\n";

	&RotOrbits($c1,$c2,$Per,$a,$i,$OMEGA,$T,$e,$omega,$d);
		
    print  ELMTS "$commenting_out[$comment_flag]";      
	print  ELMTS "$onoff Barycenter $hip \"$alt{$hip,'AB'}\"\n";
	print  ELMTS "$onoff {\n";
	printf ELMTS "$onoff     RA       %10.6f\n", $c1;
	printf ELMTS "$onoff     Dec      %10.6f\n",$c2;
	printf ELMTS "$onoff     Distance %10.6f\n",$d;
	print  ELMTS "$onoff }\n";
	print  ELMTS "$onoff \n";
	print  ELMTS "$onoff \"$alt{$hip,'A'}\" \n";
	print  ELMTS "$onoff {\n";
	print  ELMTS "$onoff     OrbitBarycenter \"$orbitRef{$hip}\"\n";
    printf ELMTS "$onoff     SpectralType    %-8s %-40s\n",$spectA,$quA;
    printf ELMTS "$onoff     AppMag          %-5.2f    %-40s\n",$mvA,$mquA;	
    print  ELMTS "$onoff \n";
	print  ELMTS "$onoff     EllipticalOrbit {\n";
	printf ELMTS "$onoff         Period          %10.3f\n",$Period;
	printf ELMTS "$onoff         SemiMajorAxis   %10.3f \# mass ratio %4.2f : %4.2f\n",$a1,$MA,$MB;
	printf ELMTS "$onoff         Eccentricity    %10.3f\n",$Eccentricity;
	printf ELMTS "$onoff         Inclination     %10.3f\n",$Inclination;
	printf ELMTS "$onoff         AscendingNode   %10.3f\n",$AscendingNode;
	$ArgOfPeri1 = $ArgOfPeri - 180;
	if ($ArgOfPeri1 < 0.0) { $ArgOfPeri1 = $ArgOfPeri + 180; }
	printf ELMTS "$onoff         ArgOfPericenter %10.3f\n",$ArgOfPeri1;
	printf ELMTS "$onoff         MeanAnomaly     %10.3f\n",$MeanAnomaly;
	print  ELMTS "$onoff     }\n";
	print  ELMTS "$onoff }\n\n";
	print  ELMTS "$onoff \"$alt{$hip,'B'}\" \n";
	print  ELMTS "$onoff {\n";
	print  ELMTS "$onoff     OrbitBarycenter \"$orbitRef{$hip}\"\n";
    printf ELMTS "$onoff     SpectralType    %-8s %-40s\n",$spectB,$quB;
    printf ELMTS "$onoff     AppMag          %-5.2f    %-40s\n",$mvB,$mquB;
	print  ELMTS "$onoff \n";
	print  ELMTS "$onoff     EllipticalOrbit {\n";
	printf ELMTS "$onoff         Period          %10.3f\n",$Period;
	printf ELMTS "$onoff         SemiMajorAxis   %10.3f \# mass ratio %4.2f : %4.2f\n",$a2,$MA,$MB;
	printf ELMTS "$onoff         Eccentricity    %10.3f\n",$Eccentricity;
	printf ELMTS "$onoff         Inclination     %10.3f\n",$Inclination;
	printf ELMTS "$onoff         AscendingNode   %10.3f\n",$AscendingNode;
	printf ELMTS "$onoff         ArgOfPericenter %10.3f\n",$ArgOfPeri;
	printf ELMTS "$onoff         MeanAnomaly     %10.3f\n",$MeanAnomaly;
	print  ELMTS "$onoff     }\n";
	print  ELMTS "$onoff }\n\n";
    $count++ if !$onoff;
}
close (BINDAT2);

print "\nNumber of enabled spectroscopic binaries: $count\n";
print "\nA component:\n------------\n";
printf STDOUT "\nspectral types from SB9 catalog: => %3d\n",$kAsb9;
printf STDOUT "spectral types from D.Pourbaix : => %3d\n",$kApx;
printf STDOUT "spectral types from HIP catalog: => %3d\n",$kAhip;
print "\nB component:\n------------\n";
printf STDOUT "spectral types from SB9 catalog: => %3d\n", $kBsb9;
printf STDOUT "spectral types from D. Pourbaix: => %3d\n",$kBpx;
printf STDOUT "spectral types from HIP catalog: => %3d\n\n",$kBhip;



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