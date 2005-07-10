#!/usr/bin/perl -w
#
# Author: Fridger.Schrempp@desy.de
#
use Math::Trig;
use Math::Quaternion qw(slerp);

open(DSC, ">deepsky.dsc") || die "Can not create deepsky.dsc\n";
#
# Constants
#----------
#
$H0 = 72.0;	     # today's Hubble constant [km/sec/Mpc], WMAP 2005
$pc2ly = 3.26167;		# parsec-to-lightyear conversion
$epsilon =23.4392911;		# ecliptic angle
$M_abs_ell = -20.133; # absolute (Blue) magnitude for elliptical galaxies                            # from peak of SBF-survey distribution <=== Maple
$M_abs_spir = -19.07; # absolute (Blue) magnitude for spiral galaxies                                # from peak of SBF-survey distribution <=== Maple
#
# Boiler plate
#-------------
#
($ver = "Revision: 1.00 ") =~ s/\$//g;
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
print DSC "# Augmented by\n"; 
print DSC "# The SBF Survey of Galaxy Distances. IV.\n"; 
print DSC "# SBF Magnitudes, Colors, and Distances, \n";
print DSC "# J.L. Tonry et al., Astrophys J 546, 681 (2001)\n";
print DSC "#\n";
print DSC "# Using today's Hubble constant = $H0 [km/sec/Mpc]  (WMAP 2005)\n";
print DSC "#\n";
print DSC "# Adapted for Celestia with Perl script: $me $ver\n";
print DSC "# Processed $year-$mon-$mday $wday $yday $isdst $hour:$min:$sec UTC\n";
print DSC "#\n";
print DSC "# by Dr. Fridger Schrempp, fridger.schrempp\@desy.de\n";
print DSC "# ------------------------------------------------------ \n";
print DSC "\n\n";
#
# read in TF + Dn-sigma distance data for fast lookup
#
open(TF," < catalogs/TF/tf-spiral.dat") || die "Can not read tf-spiral.dat\n";
while (<TF>) {

  $name = &ss(12,20);
  $name =~ s/^NG?C?0*(\w+)\[?V?\]?\s*/NGC\ $1/g;
  $name =~ s/^IC?0*(\w+)\[?V?\]?\s*/IC\ $1/g;
  $name =~ s/^UG?C?0*(\w+)\s*/UGC\ $1/g;
  $name =~ s/^E?0*(\d+-\w+)\s*/ESO\ $1/g;
  $name =~ s/^[ZC]0*(\d{1,3})_?0*(\w+)\s*/ZWG\ $1\.$2/g;
  next if ($name =~ /^A.*/);	# drop anonymous galaxies
  $distance_TF{$name} = &ss(82,86);
  $distance_TF{$name} =~ s/ //g;
  $distance_TF{$name} = $distance_TF{$name}/$H0 * 1.0e6*$pc2ly; # [ly]
  $mag_TF{$name} = &ss(45,49);
  $mag_TF{$name} =~ s/ //g;
  $v_CMB_TF{$name} = &ss(106,110);
  $logr = &ss(66,70);
  $logr =~ s/ //g;
  $r_TF{$name} = 10**(-$logr);  # r = 1-epsilon = d/D 
  #if ($name =~/^NGC|^IC/){
  #printf "%15s   %10.4g    %10.2f    %10.2f\n",$name, $distance_TF{$name}, $mag_TF{$name}, $r_TF{$name};
  #}
}
close(TF);
open(TF," < catalogs/TF/tf-ellipt.dat") || die "Can not read tf-ellipt.dat\n";
#$n=0;
while (<TF>) {
  $name = &ss(1,9);
  # get rid of extra spaces
  $name =~ s/^\s*//;
  $name =~ s/\s*$//;
  $name =~ s/\s+/ /g; 
  $name =~ s/^N\s(\d+)\s*/NGC\ $1/g;
  $name =~ s/^I\s(\d+)\s*/IC\ $1/g;
  $name =~ s/^U\s(\d+)\s*/UGC\ $1/g;
  $name =~ s/^E0*(\d+-\w+)\s*/ESO\ $1/g;
  $name =~ s/^[ZC]0*(\d{1,3})_?0*(\w+)\s*/ZWG\ $1\.$2/g;
  $distance_TF{$name} = &ss(40,46);
  $distance_TF{$name} =~ s/ //g;
  $distance_TF{$name} = $distance_TF{$name}/$H0 * 1.0e6*$pc2ly; # [ly]
  $v_CMB_TF{$name} = &ss(32,38) + &ss(40,46);
  $r_TF{$name} = "";
  #if ($name =~/^NGC|^IC/){
  #printf "%15s   %10.4g    %10.2f\n",$name, $distance_TF{$name}, $v_CMB_TF{$name};
  #$n++;
  #}
}
close(TF);
#print "$n\n";
#
# read in revised SBF-survey data IV for fast lookup
#
open(SBF," < catalogs/sbf.txt")|| die "Can not read sbf.txt\n";

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
  $v_CMB_SBF{$name} = &ss(29,32);
}
close (SBF);
#
# read in revised RC3 data rc3.txt for fast lookup
#

open(RC3," < catalogs/rc3.txt")|| die "Can not read rc3.txt\n";
while (<RC3>) {

  #
  # Galaxy name in RC3 as tag
  #--------------------------
  #  $line = $_;
  $n1 = &ss(63,74);
  $altname = &ss(75,89);

  # squeeze out all spaces
  $n1 =~ s/ //g;
  $altname =~ s/ //g;
  $altname =~ s/UGC(\d+)/UGC\ $1/g;
  $altname =~ s/ESO(\d+-\w+)/ESO\ $1/g;
  $altname =~ s/CGCG0*(\d{1,3})-0*(\w+)/ZWG\ $1\.$2/g;
  $altname =~ s/MCG-?(\w+)/MCG\ $1/g;
  if ($n1 =~ /NGC/) {
    $name = $n1;
    # compactify name
    $name = &cmpname ($name);  
  } elsif ($n1 =~ /LMC/) {
    $name = $n1;
  } elsif ($n1 =~ /SMC/) {
    $name = "NGC 292";
  } elsif ($n1 =~ /IC/) {
    $name = $n1;
    # compactify name
    $name = &cmpname ($name);  
  } elsif (! $n1) {
    # empty string?
    $name = $altname;
  }
  # squeeze out all spaces and compactify galaxy name 

  # drop galaxies without name

  next if (! $name);
  #print "$name\n";

  # apparent RC3 magnitudes
  #--------------------------
  #
  # BT
  $Bmag = "";
  # if total Bmag (BT) exists, use it 
  if (&ss(190,194) =~ /[0-9]/) {
    $Bmag = &ss(190,194);
    # if photographic (blue) mag exists, then use that instead
  } elsif (&ss(201,205) =~ /[0-9]/) { 
    $Bmag = &ss(201,205);
  }

  # (B-V)T

  # if (B-V)T exists, use it
  if (&ss(253,256) =~ /[0-9]/) {
    $BmVmagT = &ss(253,256);
    # last resort, assume blue mag = visual mag + 1.0
  } else {
    $BmVmagT = 1.0;
  }

  # Vmag (VT)
  $Vmag_RC3{$name} = $Bmag - $BmVmagT if ($Bmag);
  $Bmag_RC3{$name} = $Vmag_RC3{$name} if (! $Bmag);
  #
  # RC3 distances, [ly]
  #--------------------
  # from Hubble's law, d ~ v_recession/H0, 
  # if velocity >~ 1000 km/sec (positive!) 
  # use weighted mean recession velocity corrected to the reference frame defined
  # by the 3K microwave background radiation
  # units [km/sec]
  $vrad = &ss(359,363);
  if ($vrad =~ /[0-9]/) {
    $rc3_distance{$name} = $vrad/$H0 * 1.0e6*$pc2ly; 
    $v_CMB_RC3{$name} = $vrad;
  }
}
close (RC3);

$ngcic_flag = 0;		# =1 for NGC =2 for IC
$count =0;
$c_sbf = 0;
$c_hubble = 0;
$c_tf = 0;

open(NGCIC,"<catalogs/revngcic.txt")|| die "Can not read revngcic.txt\n";
while (<NGCIC>) {

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

  #
  # Exchange NGC number by Messier designation from Steinicke's catalog
  # ---------------------------------------------------------------------
  #  find all 43 galaxies from Messier catalog! (checked)
  #
  $messier = "";
  $ID1 = &ss(97,111);
  if ( $ID1 =~ /(^M\s\d{1,3})/) {
    $messier = $1;
    #   print "$messier\n";
  }
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
    $HType =~ s/^E\/SB0/SBa/;
    $HType =~ s/^(\w)\+C/$1/;
    $HType =~ s/\/P?R?G?D?\b//;
    $HType =~ s/(\w+\??)\sB?R?M?/$1/;
    $HType =~ s/^d//;
    $HType =~ s/(\w+)\d\-(\w)/$1$2/;
    $HType =~ s/^E\-?\/?S0/S0/;
    $HType =~ s/^S(B?)[abc]([abc])/S$1$2/;
    $HType =~ s/SB[\?0]/SBa/;
    $HType =~ s/^E\??\ ?B?R?$/E0/;
    $HType =~ s/^S\??$/S0/;
    $HType =~ s/^S(B?)c?d/S$1c/;
    $HType =~ s/^IB?m?$/Irr/;
    $HType =~ s/^Irr B$/Irr/;
    $HType =~ s/^SB?d?m/Irr/;
    $HType =~ s/^Sd$/Sc/;
    $HType =~ s/^C/E0/;
    $HType =~ s/^P\??/Irr/;
    $HType =~ s/GxyP/Irr/;
    $HType =~ s/^Ring.*[AB]?$/S0/;
    $HType =~ s/^SBcm/Irr/;
    $HType =~ s/S\+S/Sa/;
    $HType =~ s/E\+E\??/E0/;
    $HType =~ s/^[cd]?D\ ?R?/Irr/;
    $HType =~ s/E\+\*?C?\??S?/E0/;
    $HType =~ s/^R\w\d$/Irr/;
    $HType =~ s/^R/S0/;
    #   print STDOUT "$Type $HType\n";
  } 
  #
  # Alternative galaxy names
  #-------------------------
  # 
  $ID1 = &ss(97,111);
  $ID2 = &ss(113,127);
  $ID3 = &ss(129,143);
  $ID1 =~ s/ //g;
  $ID2 =~ s/ //g; 
  $ID3 =~ s/ //g;
  $altname1 = "";
  if ($ID1) {
    $an1      = &cmpname($ID1);
    $altname1 = " / ".$an1;
  }
  $altname2 = "";
  if ($ID2) {
    $an2      = &cmpname($ID2);
    $altname2 = " / ".$an2;
  }
  $altname3 = "";
  if ($ID3) {
    $an3      = &cmpname($ID3);
    $altname3 = " / ".$an3;
  }


  # array with galaxy naming alternatives from Steinicke's catalog
  @names = ($name,$an1,$an2,$an3);

  # Apparent visual magnitude: $Vmag
  #------------------------------------
  #
  $Vmag = "";			# $Vmag undefined

  # preference to value from Steinicke's catalog
  if (&ss(49,54) =~ /[0-9]/) {
    $Vmag = &ss(49,54);
  } elsif (! $Vmag) {
    # use RC3 catalog value if existing
    foreach $n (@names) {
      if ($Vmag_RC3{$n}) {
	$Vmag = $Vmag_RC3{$n};
	last;
      }
    }
    # last resort: reasonable default guess, to have $Vmag defined...
  } else {
    $Vmag = 13.565;		# average value over data base
  }

  # Distance [ly], apparent blue magnitude, $Bmag and inclination $i (TF) 
  #------------------------------------------------------------------
  #
  $Bmag = "";			# $Bmag undefined to start
  #
  # first, try SBF-distance data (IV)
  $bingo = 0;
  foreach $n (@names) {
    if ($sbf_distance{$n}) {
      $distance = $sbf_distance{$n};
      $dist_flag = 1;    
      $bingo = 1;
      $nn = $n;
      last;
    }
  } 
  # next try to exploit available Tully-Fischer distances
  if (! $bingo) {
    foreach $n (@names) {
      if ($distance_TF{$n}) {
	$distance = $distance_TF{$n};
	$dist_flag = 2;    
	$bingo = 1;
	$nn = $n;
	last;
      }
    }
  }

  # next exploit Hubble's law, d ~ v_recession/H0, 
  # if radial velocity >~ 1000 [km/sec] (positive!) 
  # use weighted mean recession velocity corrected to the reference frame defined
  # by the 3K microwave background radiation (CMB)
  #
  # Use RC3 data
  if (! $bingo) {
    foreach $n (@names) {
      if ($rc3_distance{$n} && $v_CMB_RC3{$n} > 1000.0) {
	$distance = $rc3_distance{$n};
	$dist_flag = 3;
	$bingo = 1;
	$nn = $n;
	last;
      }
    }
  }
  # Try to exploit available Tully-Fischer blue mags 
  if ($dist_flag == 2 && $mag_TF{$nn}) {
    $Bmag =  $mag_TF{$nn};  
    # next try RC3 data
  } elsif ($dist_flag == 3 && $Bmag_RC3{$nn}) {    
    $Bmag = $Bmag_RC3{$nn}; 
    # try next Steinicke's catalog 
  } elsif (&ss(42,47) =~ /[0-9]/) {
    $Bmag = &ss(42,47);
    # last resort: reasonable default guess in terms of $Vmag...
  } else {
    $Bmag = 14.2122;		# average value over data base
  }
  # last resort for distances: (<=> TF for eta = eta_peak)
  # Simple estimate using /peak/ absolute mags from (absolute)brightness
  # distributions of SBF-galaxy survey, distinguishing "E" and "S"-type galaxies.
  if (! $bingo) {
    $M_abs = $M_abs_ell if ($HType =~ /E|Irr/);
    $M_abs = $M_abs_spir if ($HType =~ /S/);
    $distance = 10**(($Bmag - $M_abs+5)/5)*$pc2ly;
    $dist_flag = 0;    
    #    printf "%10s        Distance = %8.4g ly    HType =%4s\n",$name,$distance,$HType;
  }
  # Surface brightness 
  #  $SB = &ss(56,60);

  #
  # max/min extensions [arcmin]
  # ----------------------------
  #
  # use $D [arcmin] from Steinicke's data   
  $D = &ss(62,67);
  $D =~ s/ //g;
  # for all lines, $D entry well-defined 
  $d = &ss(69,73);
  #
  # Radius [ly], ellipticity eps and inclination [degrees]
  #----------------------------------------------------
  #  r= d/D = 1 - eps
  #
  $radius = 0.5*deg2rad($D/60.0)*$distance;

  # Try to exploit available Tully-Fischer ellipticity, if relevant. 
  if ($dist_flag == 2 && $r_TF{$nn}) {
    $i = &inclination($r_TF{$nn}) ;
    # use Steinicke's catalog for $d [arcmin]
  } elsif ($d =~ /[0-9]/) {
    $i = &inclination($d/$D); 
    # last resort if $d lacking: equate min size $d to to max size.
  } else {
    $d = $D;
    $i = 3.0; 
  }
  #print "$i\n";
  #}
  # 
  # Position angle
  #---------------
  #
  $PA = &ss(75,77);
  $PA =~ s/ //g;
  # assign to 0 if undefined
  $PA = 0 if (! $PA);

  #
  # Print Absolute magnitudes of SBF-galaxies
  # to check on their universality. Correlate with $HType
  #
  #  if ($dm{$name}){
  #    $mvabs = -$dm{$name}+$Vmag;
  #    $mbabs = -$dm{$name}+$Bmag;
  #    print "$mvabs    $mbabs   $HType\n" if ($HType =~/E/);
  #  }

  #
  # Orientation (Axis, Angle)
  #---------------------------
  #
  &orientation($alpha,$delta,$HType,$i,$PA);
  #
  # absBMag
  #--------
  #
  $Mabs = $Vmag -5*log($distance/(10*$pc2ly))/log(10);
  #
  # Bmag cutoff for reasons of space!
  #-------------------------------------
  #    
  next if($Bmag > 12);
      
  if ($messier) {
    print  DSC "# $messier / $name $altname1 $altname2 $altname3\n";
    print  DSC "Galaxy \"$messier\"\n";
  } else {
    print  DSC "# $name $altname1 $altname2 $altname3\n";
    print  DSC "Galaxy \"$name\"\n";
  }
  print  DSC "{\n";
#  print  DSC "#\n";
#  printf DSC "# Inclination    = %6.2f deg's   Position Angle = %6.2f
#  deg's\n",$i,$PA;
#  printf DSC "#   app. Vmag    = %6.2f              app. Bmag =
#  %6.2f\n",$Vmag,$Bmag;

#  if ($dist_flag == 0) {
#    printf DSC "# Distance from assuming  universal  absBMag = %8.2f\n",$M_abs;
#  }
#  if ($dist_flag == 1) { 
#    $c_sbf++;
#    printf DSC "# Distance from SBF-survey. (Cf.   v_rad_CMB = %8.2f
#  km/sec)\n",$v_CMB_SBF{$nn};
#  } elsif ($dist_flag == 2) {
#    $c_tf++;
#    printf DSC "# Tully-Fischer Distance.   (Cf.   v_rad_CMB = %8.2f
#  km/sec)\n",$v_CMB_TF{$nn};
#  } elsif ($dist_flag == 3) {
#    $c_hubble++;      
#    printf DSC "# Distance from Hubble law, using: v_rad_CMB = %8.2f
#  km/sec\n",$v_CMB_RC3{$nn};
#  }
#  print  DSC "#\n";
  print  DSC "        Type  \"$HType\"\n";
  print  DSC "        InfoURL  \"http\:\/\/simbad.u-strasbg.fr\/sim-id.pl\?Ident=$name\"\n";
  printf DSC "        RA        %10.4f\n",$alpha;
  printf DSC "        Dec       %10.4f\n",$delta;
  printf DSC "        Distance  %10.4g\n",$distance;
  printf DSC "        Radius    %10.4g\n",$radius;
  printf DSC "        absMag    %10.4g\n",$Mabs;
  printf DSC "        Axis    [%8.4f %8.4f %8.4f]\n",@v;
  printf DSC "        Angle    %8.4f\n",$angle;
  print  DSC "}\n\n";


  $count++;
} 
print STDOUT "$count   $c_sbf $c_tf $c_hubble\n";

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
sub cmpname {
  my( $name ) = @_;
  my $num ="";
  if ($name =~ /([A-Z]+)([0-9\-_AB]+\.*\d*)/) {
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
