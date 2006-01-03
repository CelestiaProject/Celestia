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
($ver = "Revision: 1.20 ") =~ s/\$//g;
($me = $0) =~ s/.*\///;
($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst) = gmtime;
$year += 1900;
$mon += 1;
print DSC "# Revised NGC and IC Catalog, Wolfgang Steinicke, April 5, 2005\n";
print DSC "# http://www.ngcic.org/steinicke/default.htm\n";
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
print DSC "# Using today's Hubble constant = 72 [km/sec/Mpc]  (WMAP 2005)\n";
print DSC "#\n";
print DSC "# Abreviations for various distance methods used:\n";
print DSC "# S= SBF (Surface Brightness Fluctuations), T-F= Tully-Fischer\n";
print DSC "# V = rad. velocity in CMB frame & Hubble law \n";
print DSC "# C=Cepheids, P=photometric, N(G)=planetary nebula (globular cluster) luminosity function\n";
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
$H0 = 72.0;      # today's Hubble constant [km/sec/Mpc], WMAP 2005
$pc2ly = 3.26167;       # parsec-to-lightyear conversion
$epsilon =23.4392911;       # ecliptic angle
$M_abs_ell = -20.133; # absolute (Blue) magnitude for elliptical galaxies
# from peak of SBF-survey distribution <=== Maple
$M_abs_spir = -19.07; # absolute (Blue) magnitude for spiral galaxies
# from peak of SBF-survey distribution <=== Maple
#
# Constant hashes and arrays
# --------------------------
#
# hash with distance methods used, key = $dist_flag
%method =  ("0" => "  # distance uncertain!\n",
            "1" => "  # method: S\n",
            "2" => "  # method: T-F\n",
            "3" => "  # method: V\n",
            "4" => "  # method: B200\n");

# Hubble types of nearby galaxies
%HType = ("LMC"      => "Irr",
          "SMC"      => "Irr",
          "Sculptor" => "E0",
          "Fornax"   => "E4",
          "WLM"      => "Irr",
          "Leo I"    => "E3");

# array with nearby galaxies
@local = ("LMC","SMC","Sculptor","Fornax","WLM","Leo I");

#
# read in revised SBF-survey data IV for fast lookup
#
open(SBF," < catalogs/sbf.txt")|| die "Can not read sbf.txt\n";
$n_sbf=0;
while (<SBF>) {
    next if (/^Pref_name/);
    next if (/^E\d+/ || /^U\d+/);
    $name = &ss(1,6);
    $name =~ s/^N0*(\w+)\s*/NGC\ $1/g;
    $name =~ s/^I0*(\w+)\s*/IC\ $1/g;
    #
    # SBF-distances [ly]
    # ------------------
    #
    $deltam   = &ss(77,81);
    $dm{$name} = $deltam;
    $dm{$name} =~ s/ //g;
    $sbf_distance{$name} = 10**(($deltam+5)/5)*$pc2ly;
    #$v_CMB_SBF{$name} = &ss(29,32);
    $n_sbf++;
}
close (SBF);
print "\n";
printf "%5d SBF galaxy distance data\n",$n_sbf;
#
# read in TF + Dn-sigma distance data for fast lookup
#
# read in first GINnumber ($gin), name and BT and B-V
open(TFE," < catalogs/TF/egalf1.dat") || die "Can not read egalf1.dat\n";
while (<TFE>) {
    $gin = &ss(1,5);
    $gin =~ s/ //g;
    $name = &ss(7,16);
    # get rid of extra spaces
    $name =~ s/^\s*//;
    $name =~ s/\s*$//;
    $name =~ s/\s+/ /g;
    $name =~ s/^N\s(\d+)\s*/NGC\ $1/g;
    $name =~ s/^I\s(\d+)\s*/IC\ $1/g;
    $name =~ s/^U\s(\d+)\s*/UGC\ $1/g;
    $name =~ s/^E0*(\d+)-G0*(\d+)\s*/ESO\ $1-$2/g;
    $name =~ s/^[ZC]0*(\d{1,3})_?0*(\w+)\s*/ZWG\ $1\.$2/g;
    $name =~ s/^A0*(\d+)-S*(\d+)/A\ $1-$2/g;
    $name =~ s/^DC(\d+)/DC\ $1/g;
    $name_f1{$gin} = $name;
}
close (TFE);
# next enter the corresponding TF distances
open(TF," < catalogs/TF/egalf2.dat") || die "Can not read egalf2.dat\n";
$n_tf_e=0;
while (<TF>) {
    $gin = &ss(1,4);
    $gin =~ s/ //g;
    $name = $name_f1{$gin};
    $distance_TFE = &ss(35,41);
    $distance_TFE =~ s/ //g;
    $distance_TFE{$name} = $distance_TFE/$H0 * 1.0e6*$pc2ly; # [ly]
    # $v_CMB_TFE{$name} = &ss(32,38) + &ss(40,46);
    #printf "%5d %15s   %10.4g\n",$gin,$name, $distance_TFE{$name};
    $n_tf_e++;
}
close(TF);
printf "%5d TF elliptical galaxy distance data\n",$n_tf_e;

open(TF," < catalogs/TF/tf-spiral.dat") || die "Can not read tf-spiral.dat\n";
$n_tf_s=0;
while (<TF>) {
    $pgc = &ss(6,10);
    $pgc =~ s/ //g;
    $name = &ss(12,20);
#printf "%15s",$name;
   $name =~ s/^NG?C?0*(\w+)\[?V?\]?\s*/NGC\ $1/g;
    $name =~ s/^IC?0*(\w+)\[?V?\]?\s*/IC\ $1/g;
    $name =~ s/^UG?C?0*(\w+)\s*/UGC\ $1/g;
    $name =~ s/^E?0*(\d+)-G?0*(\d+)\s*/ESO\ $1-$2/g;
    $name =~ s/^[ZC]0*(\d{1,3})_?0?(\w+)\s*/ZWG\ $1\.$2/g;
    $name =~ s/^A0*(\d+)-S*(\d+)/A\ $1-$2/g;
    $name_TFS{$pgc} = $name;
    $distance_TF = &ss(82,86);
    $distance_TF =~ s/ //g;
    $distance_TFS{$name} = $distance_TF/$H0 * 1.0e6*$pc2ly; # [ly]
    $logr = &ss(66,70);
    $logr =~ s/ //g;
    #printf "PGC %6d %15s   %10.4g\n",$pgc, $name, $distance_TFS{$name};
    $n_tf_s++;
}
close(TF);
printf "%5d TF spiral galaxy distance data\n",$n_tf_s;
#
# read in data on 200 brightest galaxies
#
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
    $distance_B200{$name} = &ss(74,79);
    $distance_B200{$name} =~ s/ //g;
    $distance_B200{$name} = 1e6*$distance_B200{$name};
    $method_B200{$name} = &ss(83,90);
    $method_B200{$name} =~ s/\s+$//g;
    #print "$name   $distance_B200{$name} $method_B200{$name}\n";
}
close(B200);
#
# read in revised RC3 data rc3.txt for fast lookup
#
open(RC3," < catalogs/rc3.txt")|| die "Can not read rc3.txt\n";
$n_rc3 = 0;
while (<RC3>) {
    $pgc = &ss(106,116);
    $pgc =~ s/^PGC\s*0*(\d+)\s*$/$1/g;

    #
    # Galaxy name in RC3 as tag
    #--------------------------
    #
    $n1 = &ss(63,74);
    $altname = &ss(75,89);
    # squeeze out all spaces
    $n1 =~ s/ //g;
    $altname =~ s/ //g;
    $altname =~ s/UGC(\d+)/UGC\ $1/g;
    $altname =~ s/ESO(\d+-\w+)/ESO\ $1/g;
    $altname =~ s/CGCG0*(\d{1,3})-0*(\w+)/ZWG\ $1\.$2/g;
    $altname =~ s/MCG-?(\w+)/MCG\ $1/g;
    $name = "";
    if ($n1 =~ /NGC|IC|^A/) {
    $name = $n1;
    # compactify name
    $name = &cmpname ($name);
    } elsif ($n1 =~ /LMC|SMC|Sculptor|Fornax|WLM/) {
    $name = $n1;
    $altname = "NGC 292" if ( $name =~ /SMC/);
    } elsif ($n1 =~ /Leo(\w+)\b/) {
    $name = "Leo $1";
    } elsif (!$n1) {
    # empty string?
    $name = $altname;
    }
    #print "$name    $altname\n";
    # drop galaxies without name
    next if (! $name);
    $name_RC3{$pgc} = $name;

    if ($name =~ /LMC|SMC|Sculptor|Fornax|WLM|Leo\ I\b/) {

    $altname1{$name} = $altname;

    #
    # J2000 coordinates
    #-------------------
    #
    # RA
    $rh = &ss(1,2);
    $rm = &ss(3,4);
    $rs = &ss(5,8);

    # convert to hours

    $alpha{$name} = &ra2h_rc3 ($rh,$rm,$rs);

    # DEC
    $dd = &ss(11,12);
    $dm = &ss(13,14);
    $ds = 0;
    $ds = &ss(15,16) if (&ss(15,16) =~ /[0-9]/);
    $dsign = &ss(10,10);

    # convert to degrees

    $delta{$name} = &dec2deg_rc3 ($dsign,$dd,$dm,$ds);
    #
    # maj, min size  [arcmins]
    #-------------------------
    #
    $log10D25 = &ss(152,155);
    $log10D25 =~ s/ //g;
    $D = (10**$log10D25)/10; # units -> [0.1 arcmins]!
    $d = "";
    $d = $D / (10 ** &ss(162,165)) if (&ss(162,165) =~ /[0-9]/);
    #

    # Radius [ly] and inclination [degrees]
    #-----------------------------------------
    #

    $radius{$name} = 0.5*deg2rad($D/60)*$distance_B200{$name};
    if ($d){
        $i{$name} = &inclination($d/$D);
    } else {
        # equate min size $d to to max size, if $d lacking
        $d = $D;
        $i{$name} = 3.0;
    }
    #
    # PA, degrees E of N
    #---------------
    #
    $PA{$name} = &ss(186,188);
    $PA{$name} =~ s/ //g;
    # assign to 0 if undefined
    $PA{$name} = 0 if (! $PA{$name});
        #
        # apparent RC3 magnitudes
        #--------------------------
        #
        $Bmag_RC3{$name} = &ss(190,194);
        $Bmag_RC3{$name} =~ s/ //g;

    }

    #
    # RC3 distances, [ly]
    #--------------------
    # from Hubble's law, d ~ v_recession/H0,
    # if velocity >~ 1000 km/sec (positive!)
    # use weighted mean recession velocity corrected to the reference frame
    # defined by the 3K microwave background radiation, units [km/sec]
    $vrad = &ss(359,363);
    if ($vrad =~ /[0-9]/) {
        $n_rc3++ if ($vrad > 80.0);
    $rc3_distance{$name} = $vrad/$H0 * 1.0e6*$pc2ly;
    $v_CMB_RC3{$name} = $vrad;
    }
}
close (RC3);
printf "%5d RC3 galaxy distance data\n",$n_rc3;

$ngcic_flag = 0;        # =1 for NGC =2 for IC
$count =0;
$c_sbf = 0;
$c_hubble = 0;
$c_tf = 0;
$c_B200 = 0;

open(NGCIC,"<catalogs/revngcic.txt")|| die "Can not read revngcic.txt\n";
while (<NGCIC>) {
    $pgc = &ss(89,94);
    $pgc =~ s/ //g;
    $pgc =~ s/0*(\d+)/$1/g;
  #
  # Galaxy name
  #------------
  #
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
  # skip extension lines
  next if (&ss(9,10) =~ /2|10/);

  # NGC number including C(omponent)-column
  $name = &ss(1,6);
  # get rid of extra spaces
  $name =~ s/^\s*//;
  $name =~ s/\s*$//;
  $name =~ s/\s+/ /g;
  # connect the different components (C-column) of an object with a dash
  $name =~ s/\s(\d)$/-$1/g;
  if ($ngcic_flag == 1) {
    $name = "NGC ".$name;
  } elsif ($ngcic_flag == 2) {
    $name = "IC ".$name;
  } else {
    next;
  }

  # status ( reading out the S-column)
  $status = &ss(11,12);
  $status =~ s/^\s*//;
  # drop all but galaxies S=1 (drop also  parts of galaxies S=6)
  # print "$name\n" if ($status =~ /6/);
  next if ($status =~ /[234567890]/);

  # NGC 292 = SMC will be taken care of separately
  next if($name =~ /NGC 292/);

  #
  # J2000 coordinates
  #-------------------
  #
  $ra = &ss(21,30);
  $dec = &ss(32,40);

  # Convert coordinates into hours (ra) and degrees (dec):

  $alpha = &ra2h ($ra);
  $delta = &dec2deg ($dec);
  #
  #
  # Galaxy type, translation to simplified Hubble classes
  #------------------------------------------------------
  #
  $Type = &ss(79,87);
  $Type =~ s/^\s*//;
  $Type =~ s/\s*$//;
  $Type =~ s/\s+/ /g;
  if ($Type =~ /S[\+\?0abcdm]?|^[ES]$|SB[?0abcdm]?|d?E[\+\?0-7]?|Irr|IB?m?|C|RING|Ring|GxyP|P|D|^R/) {
    $HType = $Type;
    $HType =~ s/^E\/SB0/S0/;
    $HType =~ s/^(\w)\+C/$1/;
    $HType =~ s/\/P?R?G?D?\b//;
    $HType =~ s/(\w+\??)\sB?R?M?/$1/;
    $HType =~ s/^d//;
    $HType =~ s/(\w+)\d\-(\w)/$1$2/;
    $HType =~ s/^E\-?\/?S0/S0/;
    $HType =~ s/^S(B?)[abc]([abc])/S$1$2/;
    $HType =~ s/SBR?\??$/SBa/;
    $HType =~ s/SB0/S0/;
    $HType =~ s/^E\??\ ?B?R?$/E0/;
    $HType =~ s/^S\??$/S0/;
    $HType =~ s/^Sd$/Irr/;
    $HType =~ s/^S(B?)c?d/S$1c/;
    $HType =~ s/^IB?m?$/Irr/;
    $HType =~ s/^Irr B$/Irr/;
    $HType =~ s/^SB?d?m/Irr/;
    $HType =~ s/^C/E0/;
    $HType =~ s/^P\??/Irr/;
    $HType =~ s/GxyP/Irr/;
    $HType =~ s/^Ring.*[AB]?$/S0/;
    $HType =~ s/^SB?cm/Irr/;
    $HType =~ s/S\+S/Sa/;
    $HType =~ s/E\+E\??/E0/;
    $HType =~ s/^[cd]?D\ ?R?/E0/;
    $HType =~ s/E\+\*?C?\??S?/E0/;
    $HType =~ s/^R\w\d$/Irr/;
    $HType =~ s/^R/S0/;
    # new for celestia-1.4.1
    $HType =~ s/S0\+S0$/S0/;
    $HType =~ s/S[0c]\?$/S0/;
    $HType =~ s/Spec$/Irr/;
    $HType =~ s/c?E0\w+$/E0/;
    $HType =~ s/[34]S$/Sc/;
    $HType =~ s/5C$/Irr/;
    $HType =~ s/E0\+\*3/E0/;
    $HType =~ s/SB\+C\?/SBa/;

# uncomment as a check on "punch-through" NON-simple Hubble types    
#   next if ($HType =~ /^S[0a-c]$/);
#	next if ($HType =~ /^SB[a-c]$/);
#	next if ($HType =~ /^E[0-7]$/);
#	next if ($HType =~ /^Irr$/);

#   print STDOUT "$name     $Type    $HType\n";
  }
  #
  # Alternative galaxy names
  #-------------------------
  #
  $ID1 = &ss(97,111);
  $ID2 = &ss(113,127);
  $ID3 = &ss(129,143);

  $altname1 = "";
  if ($ID1) {

  #
  # Exchange NGC number by Messier designation from Steinicke's catalog
  # find all 40 galaxies from Messier catalog! (checked)
  #
  if ( $ID1 =~ /(^M\s\d{1,3})/) {
      $an1 =  $name;
      $name = $1;
  #   print "$name\n";
  } else {
    $ID1 =~ s/ //g;
    $an1      = &cmpname($ID1);
  }
    if ($an1) {
    $altname1 = ":".$an1 ;
    }
  }
  $ID2 =~ s/ //g;
  $altname2 = "";
  if ($ID2) {
    $an2      = &cmpname($ID2);
    $altname2 = ":".$an2;
  }
  $ID3 =~ s/ //g;
  $altname3 = "";
  if ($ID3) {
    $an3      = &cmpname($ID3);
    $altname3 = ":".$an3;
  }
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
  foreach $n ($name,$an1,$an2,$an3) {
    #
    # first, exploit SBF-distance data (IV)
    # they are very accurate
    #
    if ($sbf_distance{$n}) {
      $distance = $sbf_distance{$n};
      $dist_flag = 1;
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
    }
  }
  if(!$distance) {
    #
    # next add in available Tully-Fischer distances
    # for spiral galaxies
    #
    # match via PGC numbers
    if ($name_TFS{$pgc}) {
      $n_TF = $name_TFS{$pgc};
      $distance = $distance_TFS{$n_TF};
      $dist_flag = 2;
      #
      # next exploit Hubble's law, distance ~ v_recession/H0,
      # if radial velocity >~ 80 [km/sec] (positive!),
      # use weighted mean recession velocity corrected to
      # the reference frame defined by the 3K microwave background
      # radiation (CMB)
      #
      # recession velocities from RC3_rev catalog

    } elsif ($name_RC3{$pgc} && $v_CMB_RC3{$name_RC3{$pgc}}) {
      $n_RC3 = $name_RC3{$pgc};
      if ($v_CMB_RC3{$n_RC3} > 80.0) {
    $distance = $rc3_distance{$n_RC3};
    $dist_flag = 3;
        #printf "%10s %10.2g %10.2f %10.2f\n",$name,$distance,$Bmag;
      }
    }
  }
  #
  # impose magnitude cut for Celestia default distribution, which
  # keeps the number of galaxies < 1000.
  #

# next if($Bmag > 12.5);
  #
  # Next, use distances from "200 brightest galaxies" compilation
  #
  if (!$distance && $distance_B200{$name}) {
    $distance = $distance_B200{$name};
    $dist_flag = 4;
    $method{4} = "  # method: $method_B200{$name}\n";
        #printf "%10s %10s %10.2g\n", $name,$method_B200{$name},$distance_B200{$name};
  }
  #
  # last resort for distances: (<=> TF for eta = eta_peak)
  # Simple estimate using /peak/ absolute mags from (absolute) brightness
  # distributions of SBF-galaxy survey, distinguishing "E" and "S"-type galaxies.
  #
  if(! $distance){
        #print "$name $Bmag\n";
    $M_abs = $M_abs_ell if ($HType =~ /E|Irr/);
    $M_abs = $M_abs_spir if ($HType =~ /S/);
    $distance = 10**(($Bmag - $M_abs+5)/5)*$pc2ly;
    $dist_flag = 0;
    #printf "%10s  Distance = %8.4g ly  HType =%4s\n",$name,$distance,$HType;
  }
  if($name =~ /M\ 51/) {
    $distance = $sbf_distance{"NGC 5195"} - 0.5e6;
    $dist_flag = 4;
    $method{4} = "  # method: $method_B200{$name}\n";
  }
  #printf "%10s %10.2g %10.2f\n",$name,$distance,$Bmag;
  #
  # Surface brightness
  #  $SB = &ss(56,60);
  #

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
  # Print Absolute magnitudes of SBF-galaxies
  # to check on their universality. Correlate with $HType
  #
  #  if ($dm{$name}){
  #    $mbabs = -$dm{$name}+$Bmag;
  #    print "$mbabs   $HType\n" if ($HType =~/E/);
  #  }

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
  $Mabs = $Bmag -5*log(($distance - $radius)/(10*$pc2ly))/log(10);

  # count the various distance methods used
  $c_sbf++    if ($dist_flag == 1);
  $c_tf++     if ($dist_flag == 2);
  $c_hubble++ if ($dist_flag == 3);
  $c_B200++   if ($dist_flag == 4);
#

#print  DSC "# $name $altname1 $altname2 $altname3\n";
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

if (1){
#
# add Milky Way
#
print  DSC "# Milky Way\n";
print  DSC "Galaxy \"Milky Way\"\n";
print  DSC "{\n";
print  DSC "        Type  \"SBc\"\n";
printf DSC "        RA        %10.4f\n",17.75;
printf DSC "        Dec       %10.4f\n",-28.93;
printf DSC "        Distance  %10.4g\n",2.772e4;
printf DSC "        Radius    %10.4g\n",50000;
printf DSC "        AbsMag    %10.4g\n",-20.5;
printf DSC "        Axis    [%8.4f %8.4f %8.4f]\n",0.866,0.491,0.091;
printf DSC "        Angle    %8.4f\n",176;
print  DSC "}\n\n";

$count++;

#
# add close-by galaxies from RC3_rev:
#


foreach $name (@local) {
  $Mabs = $Bmag_RC3{$name} -5*log($distance_B200{$name}/(10*$pc2ly))/log(10);
  #print "$name  $Bmag_RC3{$name} $distance_B200{$name}\n";
  #
  # Orientation (Axis, Angle)
  #---------------------------
  #
  &orientation($alpha{$name},$delta{$name},$HType{$name},$i{$name},$PA{$name});

#print  DSC "# $name / $altname1{$name}\n";
    print  DSC "Galaxy \"$name:$altname1{$name}\"\n";
  print  DSC "{\n";
  print  DSC "        Type  \"$HType{$name}\"\n";
  print  DSC "        InfoURL  \"http\:\/\/simbad.u-strasbg.fr\/sim-id.pl\?Ident=$name\"\n";
  printf DSC "        RA        %10.4f\n",$alpha{$name};
  printf DSC "        Dec       %10.4f\n",$delta{$name};
  printf DSC "        Distance  %10.4g",$distance_B200{$name};
  print  DSC "  # method: $method_B200{$name}\n";
  printf DSC "        Radius    %10.4g\n",$radius{$name};
  printf DSC "        AbsMag    %10.4g\n",$Mabs;
  printf DSC "        Axis    [%8.4f %8.4f %8.4f]\n",@v;
  printf DSC "        Angle    %8.4f\n",$angle;
  print  DSC "}\n\n";


  $count++;
  $c_B200++;
}
print STDOUT "$count   $c_sbf $c_tf $c_hubble $c_B200\n";
}
# like substr($_,first,last), but one-based.
sub ss {
  substr ($_, $_[0]-1, $_[1]-$_[0]+1);
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

sub cmpname {
  my( $name ) = @_;
  my $num ="";
  if ($name =~ /([A-Z]+)\s*([0-9\-_AB]+\+*\.*\d*)/) {
    my $let = $1;
    $num = $2;
    $num =~ s/^\-//;
    $let." ".$num;
  }
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
  my $decrot = Math::Quaternion::rotation(deg2rad(90-$dec),1,0,0);
  my $rarot  = Math::Quaternion::rotation(deg2rad(90-$ra*15),0,1,0);
  my $radecrot = $decrot*$rarot;
  my $incrot = Math::Quaternion::rotation(deg2rad($i),0,0,1);
  my $pa =  Math::Quaternion::rotation(deg2rad($PA),0,1,0);
  my $eclipticsrot = Math::Quaternion::rotation(deg2rad($epsilon),1,0,0);
  #
  if ($Type =~ /E/) {
      $rot = $pa*$radecrot*$eclipticsrot;
  } else {
    $rot = $incrot*$pa*$radecrot*$eclipticsrot;
  }

  $angle=$rot->rotation_angle*180/pi;
  @v=$rot->rotation_axis;
}
