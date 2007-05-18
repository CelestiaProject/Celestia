#!/usr/bin/perl -w
#
# Author: Fridger.Schrempp@desy.de
#
#use Math::Libm ':all';
use Math::Trig;
use Math::Quaternion qw(slerp);
#
open(DSC, ">deepsky.dsc") || die "Can not create deepsky.dsc\n";
#
# Boiler plate
#-------------
#
($ver = "Revision: 1.50 ") =~ s/\$//g;
($me = $0) =~ s/.*\///;
($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst) = gmtime;
$year += 1900;
$mon += 1;
print DSC "# Revised NGC and IC Catalog, Wolfgang Steinicke, January 6, 2006\n";
print DSC "# http://www.ngcic.org/steinicke/default.htm\n";
print DSC "#\n";
print DSC "# Augmented by galaxies from Local Group volume (V <~ (10 Mpc)^3)\n";
print DSC "# Catalog of Neighboring Galaxies (Karachentsev+, 2004)\n";
print DSC "# http://cdsweb.u-strasbg.fr/viz-bin/Cat?J/AJ/127/2031\n";
print DSC "#\n";
print DSC "# Augmented by averages of distance entries from\n";
print DSC "# NED-1D: NASA/IPAC Extragalactic Database of Distances\n";
print DSC "# http://nedwww.ipac.caltech.edu/level5/NED1D/\n";
print DSC "#\n";
print DSC "# Augmented by\n";
print DSC "# NASA/IPAC Extragalactic Database (NED, batch)\n";
print DSC "# http://nedwww.ipac.caltech.edu/\n";
print DSC "#\n";
print DSC "# Augmented by\n";
print DSC "# Revised 3rd Reference Catalog of Bright Galaxies (RC3,VII/155)\n";
print DSC "# http://cdsweb.u-strasbg.fr/viz-bin/Cat?VII/155\n";
print DSC "#\n";
print DSC "# Augmented by\n";
print DSC "# Mark III Catalog of Galaxy Peculiar Velocities (Willick+ 1997,VII/198)\n";
print DSC "# http://cdsweb.u-strasbg.fr/viz-bin/Cat?VII/198#sRM2.18\n";
print DSC "#\n";
print DSC "# Augmented by distances from\n";
print DSC "# The SBF Survey of Galaxy Distances. IV.\n";
print DSC "# SBF Magnitudes, Colors, and Distances, \n";
print DSC "# J.L. Tonry et al., Astrophys J 546, 681 (2001)\n";
print DSC "#\n";
print DSC "# Augmented by distances from\n";
print DSC "# Compilation of \"200 Brightest Galaxies\"\n";
print DSC "# http://www.anzwers.org/free/universe/galax200.html\n";
print DSC "#\n";
print DSC "# Augmented by redshifts-> distances from\n";
print DSC "# VII/193 The CfA Redshift Catalogue, Version June 1995  (Huchra+ 1995)\n";
print DSC "# http://cdsweb.u-strasbg.fr/viz-bin/Cat?VII/193\n";
print DSC "#\n";
print DSC "# Using today's Hubble constant = 73.2 [km/sec/Mpc]\n";  
print DSC "# (WMAP 2007, 3 years running, legacy archive)\n";
print DSC "#\n";
print DSC "# Abreviations for various distance methods used:\n";
print DSC "# SBF= SBF (Surface Brightness Fluctuations), T-F= Tully-Fischer\n";
print DSC "# V_cmb = rad. velocity in CMB frame & Hubble law \n";
print DSC "# cep=Cepheids, P=photometric, N(G)=planetary nebula (globular cluster) luminosity function\n";
print DSC "#\n";
print DSC "# Adapted for Celestia with Perl script: $me $ver\n";
print DSC "# Processed $year-$mon-$mday $wday $yday $isdst $hour:$min:$sec UTC\n";
print DSC "#\n";
print DSC "# by Dr. Fridger Schrempp, fridger.schrempp\@desy.de\n";
print DSC "# ------------------------------------------------------ \n";
print DSC "\n\n";
#
# Constants
#----------
#
$H0 = 73.2;       # today's Hubble constant [km/sec/Mpc], WMAP 2007, 3 years
$c = 299792.458;  # Celestia's speed of light [km/sec]
$Omega_matter = 0.27;    # flat Universe
$Omega_lambda = 0.73;
$pc2ly = 3.26167; # parsec-to-lightyear conversion
$epsilon = 23.4392911; # ecliptic angle

# from peak of SBF-survey distribution <=== Maple
$M_abs_ell = -20.133; # absolute (Blue) magnitude for elliptical galaxies
$M_abs_spir = -19.07; # absolute (Blue) magnitude for spiral galaxies

# output of LG member galaxy parameters, set $lgout = 0; to suppress.
$lgout = 0;

$count = 0;
#
# Constant hashes and arrays
# --------------------------
#
# hash with distance methods used, key = $dist_flag
%method =  ("0" => "  # distance uncertain!\n",
            "1" => "  # method: SBF\n",
            "2" => "  # method: T-F\n",
            "3" => "  # method: V_cmb\n",
            "4" => "  # method: B200\n",
            "5" => "  # method: ZCAT\n",
            "6" => "  # method: NED-1D average\n",
            "7" => "  # method: other\n",
            "8" => "  # method: NED (batch)\n"
            );

# Hubble types of nearby galaxies
%HType =("-6" => "E0",
		 "-5" => "E0",
		 "-4" => "E0",
		 "-3" => "S0",
		 "-2" => "S0",
		 "-1" => "S0",
		  "0" => "S0",
		  "1" => "Sa",
		  "2" => "Sb",
		  "3" => "Sb",
		  "4" => "Sc",
		  "5" => "Sc",
		  "6" => "Sc",
		  "7" => "Sc",
		  "8" => "Sc",
		  "9" => "Irr",
		 "90" => "Irr",
		 "10" => "Irr",
		 "11" => "Irr");
		 
#
# add Milky Way (J2000)
# --------------
#
$name = "Milky Way";
$altname = " ";
		 		 
# galactic center direction

$ra  = "17 45 37.224";   #IAU 1959
$dec = "-28 56 10.23";   #IAU 1959

# galactic north pole at (J2000)

$ra_pole  = "12 51 26.282";  #IAU 1959
$dec_pole = "+27 07 42.01";  #IAU 1959

# convert to hours, degrees, respectively

$alpha = &ra2h ($ra);
$delta = &dec2deg ($dec);

$delta_pole = &dec2deg($dec_pole);
$alpha_pole = &ra2h ($ra_pole);

# position angle 
$PA = 122.932 - 90;# - 90;  #IAU 1959


$Mabs = -20.6;
$tp = "SBc";
$dist = 2.772e4;

# Orientation (Axis, Angle)
#

&orientation($alpha_pole,$delta_pole,"MilkyWay",0,$PA);
			 		 
print  DSC "# Local Group volume (< (10 Mpc)^3):\n";
print  DSC "# Milky Way\n";
print  DSC "Galaxy \"Milky Way\"\n";
print  DSC "{\n";
print  DSC "        Type  \"$tp\"\n";
print  DSC "        CustomTemplate \"MilkyWay.png\"\n";  
printf DSC "        RA        %10.6f\n",$alpha;
printf DSC "        Dec       %10.6f\n",$delta;
printf DSC "        Distance  %10.4g\n",$dist;
printf DSC "        Radius    %10.4g\n",50000;
printf DSC "        AbsMag    %10.4g\n",$Mabs;
printf DSC "        Axis    [%8.4f %8.4f %8.4f]\n",@v;
printf DSC "        Angle    %8.4f\n",$angle;
print  DSC "        InfoURL  \"http\:\/\/www.seds.org\/messier\/more\/mw.html\"\n";  
print  DSC "}\n\n";

if ($lgout){
	$dkpc = $dist/($pc2ly * 1e3);
	$rar  = "17 45 37.2";
	$decr = "-28 56 10"; 
	printf "%17s %15s; %10s; %9s; %5s; %8.3f;  %6.2f\n",$name,$altname,$rar,$decr,$tp,$dkpc,$Mabs;
}

$count++;

#
# read in the averaged distances from the NED1D distance data base
# http://nedwww.ipac.caltech.edu/level5/NED1D/
#
open(NED1D," < catalogs/ned1d.txt")|| die "Can not read ned1d.txt\n";
$name_old = "";
$ct = 1;
$n_ned1d = 0;
while (<NED1D>){
	next if (/^Contents/);
	next if (/^\s+/);
	next if(/^Galaxy/);
	next if(/^\s*\(kpc\)/);
	next if(/^\s*\(1\)/);
	($name,$dist) = split (/\s\s+/);
	($name1,$namealt) = split(/,/,$name);
	if ($namealt){
    	$name = $name1;    
	}

	$dist = &clean($dist);
	$name = &clean($name);
	
    # adapt syntax of galaxy names to RC3/NGC/IC conventions	
	$name = &standardize($name);
	
	# only consider Messier, NGC and IC galaxies in NED1D data base for now	
	
	next unless ($name =~ /^M\s\d+|^NGC|^IC/);    

    # calculate the distance averages for all entries with the same $name
	if ($name eq $name_old){
		$ct++;
		$d = (($ct-1) * $d + $dist)/$ct;
	} else {
	    $n_ned1d++;
	    $d = $dist;		
		$ct = 1;	
	}
	# convert kpc to ly
    $distance_ned1d{$name} = $d * $pc2ly * 1e3;
    #print "$name  $distance_ned1d{$name}\n" if ($name ne $name_old);
	#print "$name       $distance_ned1d{$name}\n" if ($name =~/^M\s\d+/ && $name ne $name_old);
	$name_old = $name;
}
close(NED1D);

# +++++++++++++++++++++++++++++++++++++++++++++++++++
# read in revised SBF-survey data IV for fast lookup
# +++++++++++++++++++++++++++++++++++++++++++++++++++

open(SBF," < catalogs/sbf.txt")|| die "Can not read sbf.txt\n";
$n_sbf=0;
while (<SBF>) {
    next if (/^Pref_name/);
    $name = &ss(1,8);
    $name =~ s/\s+//g;
    $name = &standardize($name);

	# only consider NGC and IC galaxies in NED1D data base for now	
	
	next unless ($name =~ /^NGC|^IC/);    
    
    #
    # SBF-distances [ly]
    # ------------------
    #
    $deltam   = &ss(77,81);
    $dm{$name} = $deltam;
    $dm{$name} =~ s/ //g;
    $sbf_distance{$name} = 10**(($deltam+5)/5)*$pc2ly;
    #print "$name  $sbf_distance{$name}\n";
    $n_sbf++;
}
close (SBF);

# ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
# read in Tully-Fisher (T-F) & Dn-sigma distance data for fast lookup
# ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

# read in first GINnumber ($gin), name and BT and B-V
open(TFE," < catalogs/TF/egalf1.dat") || die "Can not read egalf1.dat\n";
while (<TFE>) {
    $gin = &ss(1,5);
    $gin =~ s/ //g;
    $name = &standardize (&ss(7,16));

	# only consider NGC and IC galaxies in NED1D data base for now	
	
	next unless ($name =~ /^NGC|^IC/);    
    
    $name_f1{$gin} = $name;
    #print "$name\n";
}
close (TFE);

# next enter the corresponding T-F distances for elliptical galaxies, 
# match via $gin with egalf1.dat

open(TF," < catalogs/TF/egalf2.dat") || die "Can not read egalf2.dat\n";
$n_tf_e=0;
while (<TF>) {
    $gin = &ss(1,4);
    $gin =~ s/ //g;;
    $name = $name_f1{$gin} if ($name_f1{$gin});
    $distance_TFE = &ss(35,41);
    $distance_TFE = &clean($distance_TFE);
    $distance_TFE{$name} = $distance_TFE/$H0 * 1.0e6 * $pc2ly; # [ly]
    # $v_CMB_TFE{$name} = &ss(32,38) + &ss(40,46);
    #printf "%5d %15s   %10.4g\n",$gin,$name, $distance_TFE{$name};
    $n_tf_e++;
}
close(TF);

# next enter the corresponding T-F distances for spiral  galaxies

open(TF," < catalogs/TF/tf-spiral.dat") || die "Can not read tf-spiral.dat\n";
$n_tf_s=0;
while (<TF>) {
    $pgc = &ss(6,10);
    $pgc =~ s/ //g;
    $name = &standardize(&ss(12,20));

	# only consider NGC and IC galaxies in NED1D data base for now	
	
	next unless ($name =~ /^NGC|^IC/);    
    
    #$name_TFS{$pgc} = $name;
    $distance_TF = &ss(82,86);
    $distance_TF =~ s/ //g;
    $distance_TFS{$name} = $distance_TF/$H0 * 1.0e6*$pc2ly; # [ly]
    $logr = &ss(66,70);
    $logr =~ s/ //g;
    #printf "PGC %6d %15s   %10.4g\n",$pgc, $name, $distance_TFS{$name};
    $n_tf_s++;
}
close(TF);

# ++++++++++++++++++++++++++++++++++++++
# read in data on 200 brightest galaxies
# ++++++++++++++++++++++++++++++++++++++

$n_b200 = 0;
open( B200," < catalogs/200.txt") || die "Can not read 200.txt\n";
while (<B200>) {
    next if (/^\s+/);
    $name = &ss(1,8);
    $name = &cmpname ($name) if ($name =~ /^N|^I/);;
    $name =~ s/\s*$//g;
    $messier = &ss(10,13);
    $messier =~ s/ //g;
    if ($messier){
    	$messier =~ s/M(\d+)/M $1/;
    	$name = $messier;
    }
	# only consider Messier, NGC and IC galaxies in NED1D data base for now	
	
    next unless ($name =~ /^M\s\d+|^NGC|^IC/);    
    
    $distance_B200{$name} = &ss(74,79);
    $distance_B200{$name} =~ s/ //g;
    $distance_B200{$name} = 1e6 * $distance_B200{$name};
    $method_B200{$name} = &ss(83,90);
    $method_B200{$name} =~ s/\s+$//g;
    #print "$name   $distance_B200{$name} $method_B200{$name}\n";
    $n_b200++;
}
close(B200);


# +++++++++++++++++++++++++++++++
# read in redshift data from zcat
# +++++++++++++++++++++++++++++++

$n_zcat = 0;
open( ZCAT," < catalogs/zcat.txt") || die "Can not read zcat.txt\n";
while (<ZCAT>) {
    next if (/^\s+/);
    $name = &clean(&ss(1,11));
    $name = &standardize ($name); 

	# only consider NGC and IC galaxies in NED1D data base for now	
	
    next unless ($name =~ /^NGC|^IC/);    
    
    $cz = &ss(32,36);
    $cz =~ s/ //g;
	# skip if redshift z is missing    
    next if (! $cz);
    $z = $cz/$c;
	# distance inclusive relativistic cosmological  corrections    
    #printf "%10s  %10.3f  %10.3f\n", $name, $z,  $distance_zcat{$name};
    $distance_zcat{$name} = &dcmr($Omega_matter,$Omega_lambda,$z);
    $n_zcat++;
}
close(ZCAT);

# +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
# Read in missing redshift data from NED batch-job for fast lookup
# +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

open(NED," < catalogs/nedz8.txt")|| die "Can not read nedz.txt\n";
$n_ned = 0;
while (<NED>){
	    if (/\(\s+\d\)\sG/){
    	$name = &standardize(&ss(19,28));
    	next if ($name =~ /NE*$/); # drop the NGC|IC xxx NED 1/2 objects
		$z = &ss(75,80);
		$z =~ s/ //g;   
    	if ($name =~ /^NGC 4518/){$z = 0.021868;}
    	if ($name =~ /^IC 1947/){$z = 0.03851;}
    	if ($name =~ /^IC 3658/){$z = 0.00015;}
		#printf "%10s %10.4f\n",$name,$z if ($z);    
    	if ($z && $z > 0){
    		$distance_ned{$name} = &dcmr($Omega_matter,$Omega_lambda,$z);
	        #printf "%10d   %15s %10.4f\n",$n_ned, $name,$z; 
	        $n_ned++;
	    }    
    } 
}

# +++++++++++++++++++++++++++++++++++++++++++++++++
# Read in revised RC3 data rc3.txt for fast lookup
# +++++++++++++++++++++++++++++++++++++++++++++++++

open(RC3," < catalogs/rc3.txt")|| die "Can not read rc3.txt\n";
$n_rc3 = 0;
while (<RC3>) {
    $pgc = &ss(106,116);
    $pgc =~ s/^PGC\s*0*(\d+)\s*$/$1/g;

    #
    # Galaxy name in RC3 as tag
    #--------------------------
    #
    $n1 = &standardize(&ss(63,74));
    
    # These data are used BOTH for 
    #  -- $PA and $Type values for 'catalog of neighboring galaxies' 
    #  -- distances via Hubble law for NGC/IC catalog
    # Hence evaluate all alternative names for now
        
    # Hierarchy: UGC , ESO , MCG , UGCA, CGCG (Zwicky et al.)
    # Read out 2 alternative names:
    
    $altname =  &standardize(&ss(75,89));
    $altname2 = &standardize(&ss(91,104));
    
    # RC3 does not list Messier names! Hence, must translate NGC to Messier names for the 5 M-galaxies used in "catalog of neighboring galaxies"     
    
    $n1 =~ s/NGC 224/M 31/;
    $n1 =~ s/NGC 598/M 33/;
    $n1 =~ s/NGC 3031/M 81/;
    $n1 =~ s/NGC 3034/M 82/;
    $n1 =~ s/NGC 5457/M 101/;
    
    # Read out position angle for the "catalog of neighboring galaxies" below
    
    $PA{$n1} = &ss(186,188); 
	$PA{$n1} =~ s/ //g;
	# assign to 0 if undefined
	$PA{$n1} = 0 if (! $PA{$n1});
    $PA{$altname} = $PA{$n1};
    $PA{$altname2} = $PA{$n1};
    
    # Read out morphological types for the "catalog of neighboring galaxies" below
    
    $type = &ss(118,124);
    $type =~ s/ //g;
    $Type{$n1} = $type;
    $Type{$altname} = $type;
    $Type{$altname2} = $type;
    
    if(0){
    	if ($type){
    		$tp = &RC3type($type);
    		printf "%15s %15s %15s %10s %10s %10s\n",$pgc, $n1, $altname, $altname2, $type, $tp;
    	}
    }    

    #
    # RC3 distances, [ly]
    #--------------------
    # from Hubble's law, d ~ v_recession/H0,
    # if velocity >~ +80 km/sec (positive!)
    # use weighted mean recession velocity corrected to the reference frame
    # defined by the 3K microwave background radiation (CMB), units [km/sec]
    
    $vrad = &ss(359,363);
    $vrad =~ s/ //g;
    if ($vrad) {
        $n_rc3++ if ($vrad > 80.0);
    	$rc3_distance{$n1} = $vrad/$H0 * 1e6 * $pc2ly;
    	$v_CMB_RC3{$n1} = $vrad;
    }
}
close (RC3);

# +++++++++++++++++++++++++++++++++++++++++++++++++++
# Read in data from "Catalog of Neighboring Galaxies"
# +++++++++++++++++++++++++++++++++++++++++++++++++++

# names, major diameters, inclinations, absolute B-band magnitudes

open(LOCALV4," < catalogs/neighborgals4.txt")|| die "Can not read neighborgals4.txt\n";
$n_local = 0;
$j = 0;
while (<LOCALV4>){
		
	$names = &ss(1,15);
	$names = &clean($names);
	if ($names =~/,/){
		($name,$name2) = split(/,/,$names);
	} else {
		$name = $names;
	}		
	$name = &standardize($name);
    
    # it was forgotten to list main IC name first for DDO 226    
    $name =~ s/^DDO 226/IC 1574/;

# skip all NGC/IC and Messier objects in 'catalog of neighboring galaxies'	
# this drops 114 objects from the catalog
	
	#if ($name =~ /^M\s\d+|^NGC|^IC/){print "$j  $name\n";$j++;}
	next if ($name =~ /^M\s\d+|^NGC|^IC/); 
    next if ($name =~ /^Milky Way/);
#
# Major diameters  [ly]
# (corrected for galaxy inclination and the galactic extinction)
#
	$D = &ss(17,21);
	$D =~ s/ //g;
	next if (!$D);
	$radius{$name} = 0.5 * $D * $pc2ly * 1e3;
	#print "$name $radius{$name}\n";
#
# Inclinations from face-on [deg]
#	
	$i{$name} = &ss(23,24);
       
#
# Absolute B-band magnitudes
#	
	$BMag{$name} = &ss(35,40);
	$BMag{$name} =~ s/ //g;

}
close (LOCALV4);

# names, altnames, J2000 coordinates, distances, position angles and types from RC3

open(LOCALV," < catalogs/neighborgals.txt")|| die "Can not read neighborgals.txt\n";
$n_local = 0;
$j=0;
$name2="";
while (<LOCALV>){
	$names = &ss(1,15);
	$names = &clean($names);
	if ($names =~/,/){
		($name1,$name2) = split(/,/,$names);
		$name2 = &standardize($name2);
	} else {
		$name1 = $names;
    	$name2 = "";
	}		

	$name1 = &standardize($name1);
	#print "$name1\n" if ($name1 =~/^[A-Z]+[a-z]+\s*[a-zA-Z]*\s*$/);
    
    # it was forgotten to list main IC name first for DDO 226    
    $name1 =~ s/^DDO 226/IC 1574/;
	
	# skip all NGC/IC and Messier objects in 'catalog of neighboring galaxies'	
	next if ($name1 =~ /^M\s\d+|^NGC|^IC/); 
    next if ($name1 =~ /^Milky Way/);
	
	#$DD = &ss(38,42);
	#$DD =~ s/ //g;	
	next if (!$radius{$name1});
#
# Read out J2000 coordinates
#
# RA, DEC fields:
	$ra = &clean(&ss(17,26));
	$dec= &clean(&ss(28,36));

# convert to hours, degrees, respectively
  	
  	$alpha = &ra2h ($ra);
  	$delta = &dec2deg ($dec);

#
# Implement morphological Type (RC2/RC3)
#
	$T = &ss(61,62);
	$T =~ s/ //g;
	if ($Type{$name1}){
		$tp = &RC3type($Type{$name1});
		#print "$j $name1 $Type{$name1} $tp\n";
		#$j++;
	} elsif ($Type{$name2} && $name2){
		$tp = &RC3type($Type{$name2});
		#print "$j $name2 $Type{$name2} $tp\n";
		#$j++;
	} else {	
		$tp = $HType{$T};
	}
	if ($name1 =~ /^And\s+II\b/) {$tp = "E3";}
	if ($name1 =~ /^And\s+I\b/)  {$tp = "E0";}
	if ($name1 =~ /^Antlia\b/)   {$tp = "E3";}
	if ($name1 =~ /^And\s+III\b/){$tp = "E6";}
    if ($name1 =~ /^Antlia\b/)   {$tp = "E3";}	
	if ($name1 =~ /^Cetus\b/)    {$tp = "E4";}
	if ($name1 =~ /^Sex dSph\b/) {$tp = "E4";}
	if ($name1 =~ /^Tucana\b/)   {$tp = "E5";}
	if ($name1 =~ /^And\s+V\b/)  {$tp = "E0";}
	if ($name1 =~ /^UMin\b/)     {$tp = "E5";}
	if ($name1 =~ /^Draco\b/)    {$tp = "E3";}
	if ($name1 =~ /^Sag dSph\b/) {$tp = "E6";}
	if ($name1 =~ /^Cas\s+dSph\b/){$tp = "E3";}
	if ($name1 =~ /^Peg\s+dSph\b/){$tp = "E3";}
#
# distance [ly]
#
	$dist_local = &ss(88,92)* $pc2ly * 1e6;
    #$distance_local{$name1} = $dist_local;
    #printf "%15s  %15.3f\n", $name1,$dist_local;
# method used
	$meth = &clean(&ss(95,98));

#
# Implement PA, degrees E of N, from RC3, else PA = 0
# check both names 
	if ($PA{$name1}){
		$pa = &clean($PA{$name1});
		#print "$j $name1 $pa\n";
		#$j++;
	
	} elsif ($PA{$name2} && $name2){
	 	$pa = &clean($PA{$name2});
		#print "$j $name2 $pa\n";
		#$j++;
	} elsif ($name1 =~ /^Sag dSph\b/){
	
	    $pa = 90.0;	
	} else {
		$pa = 0.0;
	}	 	

#
# Orientation (Axis, Angle)
#
	&orientation($alpha,$delta,$tp,$i{$name1},$pa);
	
#
# recall radius and BMag from other file
#

	$Radius = $radius{$name1};
	$Mabs = $BMag{$name1};


# NED conform nomenclature:
# ellipticities from http://www.ast.cam.ac.uk/~mike/local_members.html
	$name1 =~ s/(^Tucana|^Draco|^Antlia|^Carina|^Orion|^Phoenix|^Cetus)/$1 Dwarf/;
	$name1 =~ s/^SagDIG/Sagittarius DIG/;
	$name1 =~ s/UMin/UrsaMinor Dwarf/;
	$name1 =~ s/ESO 349-31/Sculptor DIG/;
	$name2 =~ s/SDIG/ESO 349-31/;
	$name1 =~ s/^Sculptor\s*$/Sculptor dSph/;
	$name2 =  "ESO 351-30" if ($name1 =~/Sculptor dSph/);
	$name1 =~ s/^Sex\b/Sextans/;
	$name1 =~ s/^Sag\b/Sagittarius/;
	$name1 =~ s/^Cas\s+dSph\b/And VII/;
	$name2 = "Cassiopeia dSph" if ($name1 =~/And VII/);
	$name1 =~ s/^DDO 210/Aquarius Dwarf/;
	$name2 =  "DDO 210" if ($name1 =~/Aquarius Dwarf/);
	$name1 =~ s/^Peg\s+dSph/And VI/;
	$name2 = "Pegasus dSph" if ($name1 =~/And VI\b/);
	$name1 =~ s/^Fornax/Fornax dSph/;
	$name1 =~ s/^Pegasus/Pegasus DIG/;
	 
	$na = "";
	if ($name2){$na = ":".$name2;}
 	$name1 =~ s/ $//g;

	if($lgout){	  
    	$dkpc = $dist_local /($pc2ly * 1e3);
		if ($dkpc < 1400){	
			printf "%17s %15s; %10s; %9s; %5s; %8.3f;  %6.2f\n",$name1,$name2,$ra,$dec,$tp,$dkpc,$Mabs;	
		}	
	}	
#
# add 333 galaxies of the local group volume, V < (10 Mpc)^3,
# (without Messier, NGC and IC galaxies)
	print  DSC "# Local Group volume (< (10 Mpc)^3):\n";
	print  DSC "Galaxy \"$name1$na\"\n";
	print  DSC "{\n";
	print  DSC "        Type  \"$tp\"\n";
	printf DSC "        RA        %10.4f\n",$alpha;
	printf DSC "        Dec       %10.4f\n",$delta;
	printf DSC "        Distance  %10.4g",$dist_local;
	print  DSC " # method: $meth\n";
	printf DSC "        Radius    %10.4g\n",$Radius;
	printf DSC "        AbsMag    %10.4g\n",$Mabs;
	printf DSC "        Axis    [%8.4f %8.4f %8.4f]\n",@v;
	printf DSC "        Angle    %8.4f\n",$angle;
	print  DSC "        InfoURL  \"http\:\/\/simbad.u-strasbg.fr\/sim-id.pl\?Ident=$name1\"\n";  
	print  DSC "}\n\n";
	$count++;
	$n_local++;
}	
close (LOCALV);
if(!$lgout){	
	print "\n Catalog Input:\n---------------\n";
	printf "%5d Galaxies in Local Group Volume (d < 10 Mpc)\n",$n_local;
	printf "%5d Averaged distances from NED-1D Distance Data Base\n",$n_ned1d;
	printf "%5d Surface Brightness Fluctuation (SBF) distance data\n",$n_sbf;
	printf "%5d Tully-Fisher elliptical galaxy distance data\n",$n_tf_e;
	printf "%5d Tully-Fisher spiral galaxy distance data\n",$n_tf_s;
	printf "%5d ZCAT redshift data\n",$n_zcat;
	printf "%5d RC3 galaxy distance data\n",$n_rc3;
	printf "%5d NED redshift data (batch)\n",$n_ned;
}

$ngcic_flag = 0;        # =1 for NGC, =2 for IC
$c_sbf = 0;
$c_hubble = 0;
$c_tf = 0;
$c_B200 = 0;
$c_zcat = 0;
$c_ned1d = 0;
$c_ned = 0;

# ++++++++++++++++++++++++++++++++++++++++++++++++++++++
# Read out data from revised NGC/IC catalog (Steinecke), 
# merge with above distance data
# +++++++++++++++++++++++++++++++++++++++++++++++++++++
$name = "";
open(NGCIC,"<catalogs/revngcic.txt")|| die "Can not read revngcic.txt\n";
while (<NGCIC>) {

# skip header and identify NGC or IC catalogs
	if (/^NGC/) {
  		$ngcic_flag = 1;
    	next;
  	} elsif (/^\ IC/) {
    	$ngcic_flag = 2;
    	next;
  	} elsif (/^-/) {
    	next;
  	}

# status ( reading out the S-column)
  	$status = &clean(&ss(11,12));

# drop all but galaxies S=1 (drop also  parts of galaxies S=6)
  	next if (!$status);
  	next if ($status != 1);

# skip extension lines
  	next if (&ss(9,10) =~ /2|10/);

    $pgc = &ss(89,94);
    $pgc =~ s/ //g;
    $pgc =~ s/0*(\d+)/$1/g;
#
# J2000 coordinates
#
  	$ra =  &clean(&ss(21,30));
  	$dec = &clean(&ss(32,40));

# Convert coordinates into hours (ra) and degrees (dec):

  	$alpha = &ra2h ($ra);
  	$delta = &dec2deg ($dec);
#
# Galaxy type, translation to simplified Hubble classes
#
  	$Type = &clean(&ss(79,87));
    $HType = &type($Type);

# uncomment as a check on "punch-through" NON-simple Hubble types    
#   next if ($HType =~ /^S[0a-c]$/);
#	next if ($HType =~ /^SB[a-c]$/);
#	next if ($HType =~ /^E[0-7]$/);
#	next if ($HType =~ /^Irr$/);

#   print STDOUT "$name     $Type    $HType\n";

#
# Galaxy names
#

# NGC number
  	
  	$basename = &clean(&ss(1,5));
#  	$basename =~ s/ //g;
#
# Read alternative galaxy names, too
#
  	$ID1 = &ss(97,111);
#    $ID1 =~ s/ //g;  	
  	$ID2 = &ss(113,127);
 	$ID2 =~ s/ //g;
 	$ID3 = &ss(129,143);
  	$ID3 =~ s/ //g;
  	
  	if    ($ngcic_flag == 1) {$basename = "NGC ".$basename;} 
  	elsif ($ngcic_flag == 2) {$basename =  "IC ".$basename;} 
  	else                     {next;}
   	
   	$partflag = &ss(6,6);
  	$partflag =~ s/ //g;
# connect the different components (C-column) of an object with a dash  	
  	
  	if ($partflag){
  		$name = $basename."-".$partflag;
  		#$basename = &standardize($ID1); # so NED (batch) will find it!
   	} else {
   		$name = $basename;
   	}

# NGC 292 = SMC will be taken care of separately
  	next if($name =~ /NGC 292/);
  	
  	$altname1 = "";
  	if ($ID1) {
#
# Exchange NGC number by Messier designation from Steinicke's catalog
# find all 40 galaxies from Messier catalog! (checked)
#
  		if ( $ID1 =~ /(^M\s\d{1,3})/) {
    		$an1 =  $name;
      		$name = $1;
      		$basename = $name;
  		} else {
    		$an1 = &standardize($ID1);
  		}
    	if ($an1) {
    		$altname1 = ":".$an1 ;
    	}
  	}
  	$altname2 = "";
  	$an2 = "";
  	if ($ID2) {
    	$an2      = &standardize($ID2);
      	$altname2 = ":".$an2;
  	}
  	$altname3 = "";
  	if ($ID3) {
    	$an3      = &standardize($ID3);
    	$an3 = "DDO 226" if ($an3 eq "UGCA 9"); 
    	$altname3 = ":".$an3;
  	}
  	
  	@nm = ();
  	if ($basename) {
		#$name =~ s/^(NGC|IC)\ (\d+)[ABCD-]1*$/$1 $2/; 
		#$name =~ s/^(?:NGC|IC)\ \d+[ABCD-][234]$//;
		#$basename =~ s/-[12]$//;
  		#print "$name     $basename" if ($basename);
  		#print "          $an1\n" if ($an1);
  		#print "\n" if (!$an1);
  	    push(@nm,$basename);# if ($basename);
  	}
  	if ($an1)  {push(@nm,$an1) };
  	if ($an2)  {push(@nm,$an2) };
  	if ($an3)  {push(@nm,$an3) };
#
# Max/min extensions $D/$d [arcmin] and  inclination $i [degrees]
# ---------------------------------------------------------------
#
# use $D [arcmin] from Steinicke's data
# for all lines, $D entry well-defined
	$D = &ss(62,67);
  	$D =~ s/ //g;
  	$d = &ss(69,73);
  	if ($d =~ /[0-9]/) {
    	$i = &inclination($d/$D);
    # if $d lacking, equate min size $d to to max size $D.
  	} else {
    	$d = $D;
    	$i = 3.0;
  	}
#
# Apparent blue magnitude $Bmag
# ------------------------------------------------
#
  	$Bmag = "";                   # $Bmag undefined
  	if (&ss(42,47) =~ /[0-9]/) {
    	$Bmag = &ss(42,47);
    	$Bmag =~ s/ //g;
  	}
  
# Distance [ly] from various methods
#-----------------------------------
#
  	$distance = "";
  	$dist_flag = "";
  	# loop over alternate names
  	foreach $n (@nm) {
    #
    # first, use the distance-averages from the accurate NED-1D 
    # Extragalactic Distance Database
    # 
    #
    	if ($distance_ned1d{$n}) {
    	    #print "$n\n";
      		$distance = $distance_ned1d{$n};
      		$dist_flag = 6;
      		last;    
    #
    # next, exploit SBF-distance data (IV)
    # they are very accurate
    #
    	} elsif ($sbf_distance{$n}) {
      		$distance = $sbf_distance{$n};
      		$dist_flag = 1;
      		#print "$n\n";
      		last;
    #
    # next add in available Tully-Fischer distances
    # for elliptical galaxies
    #
    	} elsif ($distance_TFE{$n}) {
      		$distance = $distance_TFE{$n};
      		$dist_flag = 2;
      		#printf "%10s %10.2g %5s %10.2f\n",$n,$distance,$HType,$Bmag;
      		last;
    #
    # next add in available Tully-Fischer distances
    # for spiral galaxies
    #
    
    # match via PGC numbers
    	} elsif ($distance_TFS{$n}) {
      		$distance = $distance_TFS{$n};
      		#printf "%10s %10.2g %5s %10.2f\n",$n,$distance,$HType,$Bmag;
           	$dist_flag = 2;
    	
    #
    # Next, use distances from "200 brightest galaxies" compilation
    #
    #
   		} elsif ($distance_B200{$n}) {
    		$distance = $distance_B200{$n};
    		$dist_flag = 4;
    		$method{4} = "  # method: $method_B200{$n}\n";
    		#printf "%10s %10s %10.2g\n", $name,$method_B200{$name},$distance_B200{$name};
    #
    # next exploit Hubble's law, distance ~ v_recession/H0,
    # if radial velocity >~ 80 [km/sec] (positive!),
    # use weighted mean recession velocity corrected to
    # the reference frame defined by the 3K microwave background
    # radiation (CMB)
    #
    # recession velocities from RC3_rev catalog
   		} elsif ($rc3_distance{$n}) {
			if ($v_CMB_RC3{$n} > 80.0) {
    			$distance = $rc3_distance{$n};
    			#print "$pgc $n   $distance  $v_CMB_RC3{$n}\n";
    			$dist_flag = 3;  			
  			}  
  
  	#
  	# Next, use distances from zcat redshift catalog
  	#  
        } elsif ($distance_zcat{$n}) {
  			if ($distance_zcat{$n} > 0){  
    			$distance = $distance_zcat{$n} ;
    			#print "$n  $distance\n";
    			$dist_flag = 5;
  			}  
  		} elsif ($distance_ned{$n}) {
    			$distance = $distance_ned{$n} ;
    			#print "$n  $distance\n";        
    			$dist_flag = 8;
  	    }
  	}

  #
  # Individual corrections from various recent sources
  #
  	if($name =~ /M\ 51/) {
    	$distance = $sbf_distance{"NGC 5195"} - 0.5e6;
    	$dist_flag = 4;
    	$method{4} = "  # method: $method_B200{$name}\n";
  	}
  	if($name =~ /NGC\ 1569/) {
    	$distance = 2.375 * $pc2ly * 1e6;
    	$dist_flag = 7;
    	$method{7} = "  # method: P\n";
  	}
  	if($name =~ /M\ 90/) {
    	$distance = $distance_B200{"M 90"};
    	$dist_flag = 4;
    	$method{4} = "  # method: $method_B200{$name}\n";
  	}
  	if($name =~ /NGC\ 4438/) {
    	$distance = $rc3_distance{"NGC 4435"};
    	$dist_flag = 7;
    	$method{7} = "  # method: M. Machacek et al. 2003, ApJ\n";
  	}
  
  	#
  	# last resort for distances: (<=> TF for eta = eta_peak)
  	# Simple estimate using /peak/ absolute mags from (absolute) brightness
  	# distributions of SBF-galaxy survey, distinguishing "E" and "S"-type 
  	# galaxies.
  	#
  	
  	# if(!$distance){printf "%10s  %10.3f %4s\n",$name,$Bmag,$HType;}
  	#next if (!$distance);
  	
  	if(! $distance){ 
        
    	$M_abs = $M_abs_ell if ($HType =~ /^E|^Irr/);
    	$M_abs = $M_abs_spir if ($HType =~ /^S/);
    	$distance = 10**(($Bmag - $M_abs * (1 + 0.0 * (1 - 2 * rand())) + 5)/5) * $pc2ly;
    	$dist_flag = 0;
        #print "$j     $basename\n" if ($basename);
        $j++;	
    	#printf "%10s  Distance = %8.4g ly  HType =%4s\n",$name,$distance,$HType;
  	}

  	# Radius [ly]
  	#-------------
  	#
  	$radius = 0.5*deg2rad($D/60.0)*$distance;
  	#
  	# Position angle
  	#---------------
  	#
  	# use values from Steinicke's catalog
  	$PA = &ss(75,77);
  	$PA =~ s/ //g;
  	# assign to 0 if undefined
  	$PA = 0 if (! $PA);
  	#printf "%10s %10.2f %5s\n",$name,$PA,$HType if ($HType eq "Sc");

  	#
  	# Orientation (Axis, Angle)
  	#---------------------------
  	#
  	&orientation($alpha,$delta,$HType,$i,$PA);
  	#
  	# absBMag (<== Bmag)
  	#--------------------
  	# use the distance from the galaxy boundary (rather than the center)
  	# matches with Celestia code.
    #printf "%10s %15.4g %10d\n",$name,$distance,$dist_flag if ($name =~ /^M\s\d+/|$name =~/^NGC\s5196/);
  	$Mabs = $Bmag - 5*log(($distance - $radius)/(10*$pc2ly))/log(10);
    #$DM = $Bmag - $Mabs;
    #printf "%10.3f  %10.3f\n",$Bmag, $DM if ($dist_flag == 2);  
  	# count the various distance methods used
  	$c_sbf++    if ($dist_flag == 1);
  	$c_tf++     if ($dist_flag == 2);
  	$c_hubble++ if ($dist_flag == 3);
  	$c_B200++   if ($dist_flag == 4);
  	$c_zcat++   if ($dist_flag == 5);
  	$c_ned1d++  if ($dist_flag == 6);
  	$c_ned++    if ($dist_flag == 8);
	    
	if($lgout){	  
    	$dkpc = $distance /($pc2ly * 1e3);
		if ($dkpc < 1400){	
		printf "%17s %15s; %10s; %9s; %5s; %8.3f;  %6.2f\n",$name,$an1,$ra,$dec,$HType,$dkpc,$Mabs;	
		}	
	}	
	
	print  DSC "Galaxy \"$name$altname1$altname2$altname3\"\n";
  	print  DSC "{\n";
  	print  DSC "        Type  \"$HType\"\n";
	printf DSC "        RA        %10.4f\n",$alpha;
  	printf DSC "        Dec       %10.4f\n",$delta;
  	printf DSC "        Distance  %10.4g",$distance;
  	print  DSC "$method{$dist_flag}";
  	printf DSC "        Radius    %10.4g\n",$radius;
  	printf DSC "        AbsMag    %10.4g\n",$Mabs;
  	printf DSC "        Axis    [%8.4f %8.4f %8.4f]\n",@v;
  	printf DSC "        Angle    %8.4f\n",$angle;
  	print  DSC "        InfoURL \"http\:\/\/simbad.u-strasbg.fr\/sim-id.pl\?Ident=$name\"\n";
  	print  DSC "}\n\n";
  	
  	$count++;
}

# Summary output of used distance data
$sum = $n_local + $c_ned1d + $c_sbf + $c_tf + $c_hubble + $c_B200 + $c_zcat + $c_ned;
print  "\nSummary Output:\n---------------\n";
printf "%5d Galaxies in Total\n",$count;
printf "%5d with good distance data\n\n",$sum;
printf "%5d with distances from 'catalog of neighboring galaxies'\n",$n_local;
printf "%5d with NED-1D averaged distances\n",$c_ned1d;
printf "%5d with Surface Brightness Fluctuation (SBF) distances\n",$c_sbf;	
printf "%5d with Tully-Fisher distances\n",$c_tf;	
printf "%5d with B200 distances\n",$c_B200;	
printf "%5d with distances from Hubble Law (v_CMB, RC3)\n",$c_hubble;
printf "%5d with distances from Hubble Law (ZCAT).\n",$c_zcat;
printf "%5d with distances from Hubble Law (NED, batch).\n",$c_ned;

# like substr($_,first,last), but one-based.
sub ss {
  	substr ($_, $_[0]-1, $_[1]-$_[0]+1);
}
sub clean {
# squeeze out superfluous spaces
my($string) = @_;
$string =~ s/^\s*//;
$string =~ s/\s*$//;
$string =~ s/\s+/ /g;
$string;
}
sub ra2h {
  	my($coord) = @_;
  	my $h =  substr($coord,0,2);
  	my $min =  substr($coord,3,2);
  	my $sec = substr($coord,6,3);
  	$h+$min/60+$sec/3600;
}
sub dec2deg {
	my($coord) = @_;
  	my $sign = substr($coord,0,1);
  	my $deg  = substr($coord,1,2);
  	my $min  = substr($coord,4,2);
  	my $sec  = substr($coord,7,3);
  	if ($sign eq "+") {
    	$s =1;
  	} else {
    	$s = -1;
  	}
  	$s*($deg+$min/60+$sec/3600);
}
sub ra2h_rc3 {
    my($h,$min,$sec) = @_;
    $h+$min/60+$sec/3600;
}
sub dec2deg_rc3 {
    my($sign,$deg,$min,$sec) = @_;
    if ($sign eq "+"){
    	$s =1;
    } else {
    	$s = -1;
    }
    $s*($deg+$min/60+$sec/3600);
}
sub standardize {
  	# eliminate prenulls and bring the ned1d galaxy names in sync with RC3
  	# and NGC/ICrevised
  	my( $name ) = @_;
    $name =~ s/ //g;
	$name =~ s/\bI*G//g;	
	$name =~ s/\b0+//g;
	$name =~ s/([A-Z]+)0+([1-9]+)/$1 $2/g;
    $name =~ s/(^[A-Z]+)(\-*[1-9].*)/$1 $2/;    
    $name =~ s/^E\b/ESO/;
    $name =~ s/^U\b/UGC/;
    $name =~ s/^UA\b/UGCA/;
    $name =~ s/^D\b/DDO/;
    $name =~ s/^N\b/NGC/;
    $name =~ s/^I\b/IC/;
    $name =~ s/^ANDIV/And IV/;
    $name =~ s/(^[A-Z][a-ce-z]+)(dSph)/$1 $2/;
    $name =~ s/(^[A-Z][a-z]+G*)(I*[VAB]*)/$1 $2/ unless ($name =~ /SagDIG|dSph/);
    $name =~ s/^ARP/Arp/;
    $name =~ s/(^[A-Za-z]+)([1-9]+)/$1 $2/;
    $name =~ s/LGS -3/LGS 3/;
    $name =~ s/=\w+//;
    $name =~ s/\[V\]//;
    #$name =~ s/^Z (\d+)/ZWG $1/;
    $name =~ s/^C (\d+)_0*(\d+)/CGCG $1-$2/;
    $name =~ s/^P (\d+)/PGC $1/;
    $name =~ s/^\[KK98\](\d+)/KK $1/;     
    $name =~ s/Dwarf//;
    $name =~ s/-dE1//;
    $name =~ s/\/39$//;
    $name =~ s/\&$//;
    $name =~ s/^NPM\s1G(.*)/NPM1G\ $1/;
    
    return $name;
}
sub cmpname {
  	my( $name ) = @_;
  	my $num ="";
  	if ($name =~ /(^[A-Z]+)\s*([0-9\-_AB]+\+*\.*\d*)/) {
    	my $let = $1;
    	$num = $2;
    	$num =~ s/^\-//;
    	$let." ".$num;
  	}
}
sub RC3type {
# Reduce the RC3 morphology scheme to simple Hubble classes	
	my($iType) = @_;
    return "" if (length($iType) != 7); 
    $oType = substr($iType,1,length($iType) - 3);
    $oType =~ s/^[IP].*$/Irr/;
    $oType =~ s/^L.*$/S0/;    
    $oType =~ s/^RING.*$/SBa/;
    $oType =~ s/[AX]//;
    $oType =~ s/^SB.*0/SBa/;
    $oType =~ s/^E.*(\d).*$/E$1/;
    $oType =~ s/\+*\?*\.*//g;
    $oType =~ s/^(SB*).*[89]/Irr/;    
    $oType =~ s/^(SB*).*[567]$/$1c/;
    $oType =~ s/^(SB*).*[34]$/$1b/;
    $oType =~ s/^(SB*).*[12]$/$1a/;
    $oType =~ s/^S$/S0/;
    $oType =~ s/^E$/E0/;
    return $oType;
}	
sub type {
	my($Type) = @_;
  	if ($Type =~ /S[\+\?0abcdm]?|^[ES]$|SB[?0abcdm]?|d?E[\+\?0-7]?|Irr|IB?m?|C|RING|Ring|GxyP|P|D|^R/) {
    	$Type =~ s/^E\/SB0/S0/;
    	$Type =~ s/^(\w)\+C/$1/;
    	$Type =~ s/\/P?R?G?D?\b//;
    	$Type =~ s/(\w+\??)\sB?R?M?/$1/;
    	$Type =~ s/^d//;
    	$Type =~ s/(\w+)\d\-(\w)/$1$2/;
    	$Type =~ s/^E\-?\/?S0/S0/;
    	$Type =~ s/^S(B?)[abc]([abc])/S$1$2/;
    	$Type =~ s/SBR?\??$/SBa/;
    	$Type =~ s/SB0/S0/;
    	$Type =~ s/^E\??\ ?B?R?$/E0/;
    	$Type =~ s/^S\??$/S0/;
    	$Type =~ s/^Sd$/Irr/;
    	$Type =~ s/^S(B?)c?d/S$1c/;
    	$Type =~ s/^IB?m?$/Irr/;
    	$Type =~ s/^Irr B$/Irr/;
    	$Type =~ s/^SB?d?m/Irr/;
    	$Type =~ s/^C/E0/;
    	$Type =~ s/^P\??/Irr/;
    	$Type =~ s/GxyP/Irr/;
    	$Type =~ s/^Ring.*[AB]?$/S0/;
    	$Type =~ s/^SB?cm/Irr/;
    	$Type =~ s/S\+S/Sa/;
    	$Type =~ s/E\+E\??/E0/;
    	$Type =~ s/^[cd]?D\ ?R?/E0/;
    	$Type =~ s/E\+\*?C?\??S?/E0/;
    	$Type =~ s/^R\w\d$/Irr/;
    	$Type =~ s/^R/S0/;
    	# new for celestia-1.4.1
    	$Type =~ s/S0\+S0$/S0/;
    	$Type =~ s/S[0c]\?$/S0/;
    	$Type =~ s/Spec$/Irr/;
    	$Type =~ s/c?E0\w+$/E0/;
    	$Type =~ s/[34]S$/Sc/;
    	$Type =~ s/5C$/Irr/;
    	$Type =~ s/E0\+\*3/E0/;
    	$Type =~ s/SB\+C\?/SBa/;
    }	
	return $Type;
}
sub inclination {
  	my( $r ) = @_;
  	# compute inclination angle $i [degrees] from $d/$D extension ratio
  	my $wu = ($r**2 - 0.2**2)/(1.0 - 0.2**2);
  	if ($wu > 1.0e-8) {
    	3.0 + 180/pi*acos(sqrt($wu));
  	} else {
    	90.0;
  	}
}

sub orientation {
  	my($ra, $dec, $Type, $i, $PA) = @_;
  	my $decrot = Math::Quaternion::rotation(deg2rad(90 - $dec),1,0,0);
  	my $rarot  = Math::Quaternion::rotation(deg2rad(90 - $ra * 15),0,1,0);
  	my $radecrot = $decrot * $rarot;
  	my $incrot = Math::Quaternion::rotation(deg2rad($i),0,0,1);
	my $pa =  Math::Quaternion::rotation(deg2rad($PA),0,1,0);
  	my $eclipticsrot = Math::Quaternion::rotation(deg2rad($epsilon),1,0,0);
  	#
  	if ($Type =~ /^E|^Milky/) {
    	$rot = $pa * $radecrot * $eclipticsrot;
	} else {
    	$rot = $incrot * $pa *  $radecrot * $eclipticsrot;
  	}
	if ($rot->rotation_angle > pi){
		$rot = $rot->negate;
	}
	$rot   = $rot->normalize;	
  	$angle = $rot->rotation_angle * 180/pi;
	@v     = $rot->rotation_axis;	
}

sub dcmr {
# calculate the comoving distance appropriate for Hubble's law, 
# given $z, $Omega_matter=$Wm, $Omega_lambda=$Wlam.
    my($Wm, $Wlam, $z) = @_;
    my $Wr = 0.4165/($H0 * $H0); # 3 massless neutrinos to Omega (radiation)
    my $Wk = 1 - $Wm -$Wr -$Wlam; # flat universe, Omega (total) = 1
    my $az = 1/($z + 1);
    my $numpoints = 50; # support points for numerical integration
    # do integral over $a=1/(1+$z) from $az to 1 in $numpoints steps, 
    # midpoint rule
  	$dcmr = 0;
  	#print "$Wm $Wlam $Wk  $Wr $az\n";
  	for ($i = 0; $i < $numpoints; $i++) {
    my $a = $az + (1 - $az) * ($i + 0.5)/$numpoints;
    my $adot = sqrt($Wk + ($Wm/$a) + ($Wr/($a * $a)) + ($Wlam * $a * $a));
    $dcmr = $dcmr + 1/($a * $adot);
	}
	return $dcmr = $pc2ly * 1e6 * $c/$H0 * (1 - $az) * $dcmr/$numpoints;
}