#!/usr/bin/perl 
#
# Author: Fridger.Schrempp@desy.de
#
#use Math::Libm ':all';
use Math::Trig;
use Math::Quaternion qw(slerp);
#
open(DSC, ">globulars.dsc") || die "Can not create globulars.dsc\n";
#
# Boiler plate
#-------------
#
($ver = "Revision: 1.0.0 ") =~ s/\$//g;
($me = $0) =~ s/.*\///;
($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst) = gmtime;
$year += 1900;
$mon += 1;
print DSC "# \"Catalog of Parameters for 150 Milky Way Globular Clusters\",\n";  
print DSC "#\n";
print DSC "# 2003 Update, by William E. Harris.\n";
print DSC "# http://physwww.physics.mcmaster.ca/~harris/mwgc.dat\n";
print DSC "# Bibliography: http://physwww.mcmaster.ca/%7Eharris/mwgc.ref\n";
print DSC "#\n";
print DSC "# Supplemented by diameters <=> 25mu isophote from\n";
print DSC "# Brian A. Skiff/Lowell Observatory, NGC/IC project,\n";
print DSC "# http://www.ngcic.org/data_archive/gc.txt.\n";
print DSC "#\n";
print DSC "# Supplemented by diameters <=> 25mu isophote from\n";
print DSC "# the SEDS 2007 catalog (H. Frommert)\n";
print DSC "# http://www.seds.org/~spider/spider/MWGC/mwgc.html.\n";
print DSC "#\n";
print DSC "# Supplemented by a cross-index for alternate identifiers\n"; 
print DSC "# generated from and compatible with the SIMBAD world data base,\n"; 
print DSC "# using the batch interface\n";
print DSC "# http://simbad.u-strasbg.fr/simbad/sim-fscript.\n";
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
$pc2ly = 3.26167; # parsec-to-lightyear conversion
$epsilon = 23.4392911; # ecliptic angle	

%format =  ("0" => "        Radius          %10.4g  # unknown!! DSS2-red estimate used\n",
            "1" => "        Radius      	%10.4g  # [ly], mu25 Isophote\n",		            
			"2" => "        CoreRadius      %10.4g  # unknown!! Data base average used\n",
            "3" => "        CoreRadius  	%10.4g  # [arcmin]\n",
            "4" => "        KingConcentration     %4.2f  # unknown!! Data base average used\n",
            "5" => "        KingConcentration     %4.2f  # c = log10(r_t/r_c)\n",
            "6" => "        AbsMag      	%10.4g  # unknown!! Data base average used\n",
            "7" => "        AbsMag      	%10.4g  # [V mags]\n"
            );
#
# Data base averages of globular parameters			
# -----------------------------------------
#
$AvM_Vt       = -6.86;
$M_Vt_dimmest = -1.60;
$Avc  	      =  1.47;
$Avr_c 	      =  0.83;
$AvRadius     = 40.32;								

$count = 0;
#
# read in mu25 isophote diameters from data base  
# by Bryan Skiff/Lowell Observatory; http://www.ngcic.org/data_archive/gc.txt
# 
#
open(MU25," < gcBSkiff99.txt")|| die "Can not read gcBSkiff99.txt\n";
$n_skiff = 1;
while (<MU25>){
	next if (/^#/);
	next if (/^\s+/);
	($id1,$name1) = split(/=/,&ss(1,20));
	$id1 = &clean($id1);
	$name1 = &clean($name1);	
	$diameter = &clean(&ss(46,50)); 	# units arcmin

	if ($diameter){
	$diameter =~ s/\~//g;
    $Radius_mu25{$id1} = 0.5 * $diameter;
    #print "$n_skiff   $id1  $name1   $Radius_mu25{$id1}\n";
    $n_skiff++;
	} 
}
close(MU25);

open(SIMBAD," < simbad.txt")|| die "Can not read simbad.txt\n";
$n_simbad = 1;
while (<SIMBAD>){

	# eliminate SIMBAD acronyms that are exotic or too long
	# all remaining acronyms should also work with SIMBAD

	if (/^Identifiers:(.*$)/){	
		$altname[$n_simbad] = &clean($1); 									
		$altname[$n_simbad] =~ s/:2E 0021.8-7221:1RXS J002404.6-720456:1ES 0021-72.3:1E 0021.8-7221//g;
		$altname[$n_simbad] =~ s/:\[FS2003\] 0013:\[BM83\] X0021-723//g;
		$altname[$n_simbad] =~ s/SGC 035336-4945.6:Cl AM 1:NAME E1 CLUSTER:NAME E1:NAME E 1://g;
		$altname[$n_simbad] =~ s/NAME HERCULES GLOBULAR CLUSTER://g;	
		$altname[$n_simbad] =~ s/:2E 82//g;
		$altname[$n_simbad] =~ s/:\s*://g;
		$altname[$n_simbad] =~ s/:$//g;
		$altname[$n_simbad] =~ s/\bCl\b\s//g;
		$altname[$n_simbad] =~ s/\bNAME\b/Name/g;
		$altname[$n_simbad] =~ s/Cl\* NGC 5897 FFB 3037://g;
		$altname[$n_simbad] =~ s/[V2001] NGC 1904 X01://g;
		$altname[$n_simbad] =~ s/Name PYXIS CLUSTER:Name PYX GLOBULAR CL://g;			
		$altname[$n_simbad] =~ s/ome Cen//g;
		$altname[$n_simbad] =~ s/ksi/xi/g;			
		$altname[$n_simbad] =~ s/\bSTAR CLUSTER\b/star cluster/g;
		$altname[$n_simbad] =~ s/\bCLUSTER\b/cluster/g;
		$altname[$n_simbad] =~ s/\*\s//g;
		$altname[$n_simbad] =~ s/^Ruprecht 106://g;
		$altname[$n_simbad] =~ s/Name CL ESO 452\-SC 11://g;
		$altname[$n_simbad] =~ s/2MASX J\d+[\+\-]\d+://g;
		$altname[$n_simbad] =~ s/Ton 2/Ton 2:Pismis 26:VDBH 236:Tonantzintla 2:GCl 71:C 1732-385/;
		$altname[$n_simbad] =~ s/Name ERIDANUS star cluster://g;
		$altname[$n_simbad] =~ s/1E 0522\.1\-2433:\[V2001\] NGC 1904 X01://g;
		$altname[$n_simbad] =~ s/Name E3 cluster://g;
		$altname[$n_simbad] =~ s/MCG[0-9\+\-]+://g;
		$altname[$n_simbad] =~ s/CSI[0-9\+\-\s?]+://g;
		$altname[$n_simbad] =~ s/Z [0-9\.\+?\-?]+://g;
		$altname[$n_simbad] =~ s/1E [0-9\.\-]+://g;
		$altname[$n_simbad] =~ s/RX J1835.7-3259://g;
		$altname[$n_simbad] =~ s/\[FG2000\] J173324\.56\-332319\.8://g;
		$altname[$n_simbad] =~ s/:\[FG2000\] Terzan 5 C//g;		
		$altname[$n_simbad] =~ s/Name SEXTANS C://g;
		$altname[$n_simbad] =~ s/:\[BDS2003\] 103//g;			
		#print "$altname[$n_simbad]\n";		    
		
		$n_simbad++;
	}			
}
close(SIMBAD);
#
# read in some more recent mu25 isophote diameters from the SEDS data base  
# (Hartmut Frommert), version 2007; 
# http://www.seds.org/~spider/spider/MWGC/mwgc.html
# 
open(SEDS," < gcHFrommert07.txt")|| die "Can not read gcHFrommert07.txt\n";
$n_seds = 1;
while (<SEDS>){
	next if (/^#/);
	next if (/^\s+/);
	($id2,$name2) = split(/\,/,&ss(1,20));
	if ($name2 =~ /NGC/){
		$hold = $name2;
		$name2 = $id2;
		$id2 = $hold;
	}			
	$id2 = &clean($id2);
	$id2 =~ s/ESO452-SC11/ESO 452-SC11/g;
	$name2 = &clean($name2);	
	$diameterSEDS = &clean(&ss(70,75)); 	# units arcmin
	if ($diameterSEDS){
    $Radius_mu25_SEDS{$id2} = 0.5 * $diameterSEDS;
    #print "$n_seds   $id2 $name2  $Radius_mu25{$id2}\n";
    
	$n_seds++;
	} 
}
$nglob = 150;
open(HARRIS,"<catalog2003.dat")|| die "Can not read catalog2003.dat\n";
$k=1;
while (<HARRIS>) {

	$out=&clean($_);
	next if ($out =~ /^$/);
	next if ($out =~ /^#/);
	# read out values from the first data chunk
	if($k<=$nglob){
		$id[$k]=&clean(&ss(1,10));
		$name=&clean(&ss(11,23));
		if($name){$name[$k]=$name.":"};
		#
		# J2000 coordinates
		#
  		$ra =  &clean(&ss(24,34));
  		$dec = &clean(&ss(36,45));

		# Convert coordinates into hours (ra) and degrees (dec):

  		$alpha[$k] = &ra2h ($ra);
  		$delta[$k] = &dec2deg ($dec);
		$R_Sun[$k] = &clean(&ss(63,68)) * $pc2ly * 1e3;	
		#print "$k   $id[$k]     $name[$k]\n";
		$k++;
		
		# next, data from the second data chunk	
	} elsif (($k>$nglob) && ($k<=2*$nglob)){
		$kk = $k - $nglob;	
		if (&clean(&ss(1,10)) == $id[$kk]){		
			$M_Vt[$kk] = &clean(&ss(36,42));		
			if ($M_Vt[$kk] eq ""){$M_Vt[$kk] = "??";}		
			# print "$id[$kk]   $M_Vt[$kk]\n";		
			$v_hb[$kk] = &clean(&ss(18,23));
			#print "$name[$k-$nglob]$id[$k-$nglob] $v_hb[$k-$nglob]\n";
		}
    	$k++;
		# finally, from the third data chunk	
	} else {
		$kk = $k - 2 * $nglob;		
		if (&clean(&ss(1,10)) == $id[$kk]){
			$SpT[$kk] = &clean(&ss(19,23));
			if ($SpT[$kk] eq ""){$SpT[$kk] = "??";}
			$r_c[$kk] = &clean(&ss(55,59));
			$r_t[$kk] = &clean(&ss(65,70));
			$mu_V[$kk] = &clean(&ss(84,89));
			#print "$name[$kk]$id[$kk] $Radius[$kk] $v_hb[$kk]\n";
		}
    	$k++;	
	}
}
$M_min = -1000;
$avM = 0;
$avc = 0;
$avr_c = 0;
$avR_mu25 = 0;

for ($k=1; $k<=150;$k++){
	
	# various syntax mods to achieve name consistency
    # with auxiliary data <MU25> and <SEEDS>
	
	$id[$k] =~ s/2MS-GC0(?)/2MASS-GC0$1/g;
	$id[$k] =~ s/ESO-SC06/ESO 280-SC06/g;		
	$id[$k] =~ s/^1636-283/ESO 452-SC11/g;    

	$name[$k] =~ s/2MASS-GC0\d://g;
	$name[$k] =~ s/ESO280-SC06://g;
	$name[$k] =~ s/ESO452-SC11://g;
    $name[$k] =~ s/AvdB://g;
	 
	$avM += $M_Vt[$k] if ($M_Vt[$k] ne "??");	
	$M_min = $M_Vt[$k] if (($M_Vt[$k] ne "??") && ($M_Vt[$k] >  $M_min));  
	$avr_c += $r_c[$kk] if ($r_c[$kk] ne "??");
				
	# calculate King concentration c    
	if ($r_c[$k] && $r_t[$k]){
		$c = log($r_t[$k] / $r_c[$k])/log(10);
	}  else {
		$c   = "??";
		$r_c[$k] = "??" if (!$r_c[$k]);			
	}		
	$avc += $c if ($c ne "??");
	
	if ($Radius_mu25{$id[$k]}){
		$R_mu25 = $Radius_mu25{$id[$k]};
		#print "1  $k $id[$k]  $c  $R_mu25\n";
	} elsif ($Radius_mu25_SEDS{$id[$k]}){
		$R_mu25 = $Radius_mu25_SEDS{$id[$k]};
	   	#print "2  $k $id[$k]  $c  $R_mu25\n";
	} elsif (($c ne "??") && $mu_V[$k]) {
    	$R_mu25q = 1.0/((1.0 - 1.0/sqrt(1.0 + 10**(2*$c)))*10**(0.2 * $mu_V[$k] - 5.0) + 1.0/sqrt(1.0 + 10**(2*$c)))**2 - 1.0;	
		if ($R_mu25q <= 0) {
			$R_mu25 = "??";
		} else {
			$R_mu25 = sqrt($R_mu25q);
		}
		#print "3  $k $id[$k]  $c  $R_mu25\n";
	} else {
		$R_mu25 = "??";
		#print "4  $k $id[$k]  $c  $R_mu25\n";
	}
		
    $avR_mu25 += deg2rad($R_mu25/60.0) * $R_Sun[$k] if ($R_mu25 ne "??");		
			
		#print "3  $k $id[$k]  $c  $R_mu25\n";	
		
		
	#
  	# Orientation (Axis, Angle)
  	#---------------------------
  	#
    &orientation($alpha[$k],$delta[$k],"E0",0,0);
			
    # further syntax adjustments for compatibility with SIMBAD		
	$id[$k]   =~ s/^Rup\b/Ruprecht/;
	$id[$k]   =~ s/^BH\b/VDBH/;
	$id[$k]   =~ s/^1636-283/C 1636-283/;	
	$id[$k]   =~ s/^HP\b/Haute-Provence/;
	$id[$k]   =~ s/^Terzan1(\d)/Terzan 1$1/;
    $name[$k] =~ s/^HP\b/Haute-Provence/;
	$name[$k] =~ s/^BH\b/VDBH/;	
	$name[$k] =~ s/Pismis 26/Ton 2/g;
	$id[$k]   =~ s/Ton 2/Pismis 26/g;
	$id[$k] =~ s/Arp 2/Cl Arp 2/g;
	if ($id[$k] =~/Cl Arp 2/){$altname[$k] = "GCl 112:C 1925-304";}	    
	if ($id[$k] =~/Djorg 1/){$altname[$k] = "";}	
	$name[$k] =~ s/E 1/AM 1/g;
	$id[$k]   =~ s/AM 1/Name E1/g;
	$id[$k]   =~ s/E 3/Name E 3/g;
    $id[$k]   =~ s/^UKS 1\b/Name UKS 1/g;
	
	#
	# finalize the alternate id strings from SIMBAD
	#
		
	# are $id[$k] and $name[$k] substrings of the SIMBAD $altname[$k] string?
	# if YES, delete these in $altname[$k].
	
	$ind = index($altname[$k],$id[$k]);		
	if ($ind >= 0){
		$altname[$k] =~ s/$id[$k]://g;
	}	
	#print "$k  $id[$k]    $altname[$k]            $ind\n";		   	
	$ind = index($altname[$k],$name[$k]);
	if ($ind >= 0){
		$altname[$k] =~ s/$name[$k]//g;
	} 
	#print "$k  $name[$k]    $altname[$k]            $ind\n";
		
	# further mods AFTER the substring tests were done
	# adjust relative locations of acronyms within the string	
	if ($name[$k] =~/Ton 1/){$altname[$k]    = &flipnm(0,1,$altname[$k]);}	
	if ($name[$k] =~/47 Tuc/){$altname[$k]   = &flipnm(0,4,$altname[$k]);}
	if ($id[$k]   =~/Eridanus/){$altname[$k] = &flipnm(0,1,$altname[$k]);}
	if ($id[$k]   =~/Eridanus/){$altname[$k] = &flipnm(1,2,$altname[$k]);}
	if ($id[$k]   =~/Pal 3/){$altname[$k]    = &flipnm(0,2,$altname[$k]);}
		
          
	$id[$k]       =~ s/Name E 3/Name E3/g;
	$altname[$k]  =~ s/Name E3/Name E 3/g;	
	$altname[$k]  =~ s/Arp 1/Cl Arp 1/g;	
	#	
	# more exotic catalog entries to be discarded	
	#	
	$altname[$k]  =~ s/:FAUST \d+//g;	
	$altname[$k]  =~ s/\[\w+\d+\w?\] \w*\s?\d+\+?\-?\d*://g;
	$altname[$k]  =~ s/:AM 0353-494//g;
	$altname[$k]  =~ s/:GCRV \d+\s?\w?//g;
	$altname[$k]  =~ s/:CP?D\-\d+\s\d+//g;
	$altname[$k]  =~ s/:\s?$//g;
	if ($id[$k]   =~/Pal 5/){$altname[$k] = &flipnm(0,1,$altname[$k]);}
	$altname[$k]  =~ s/:EQ 0423-21//g;
	$altname[$k]  =~ s/:BD\-\d+\s\d+//g;	
	#
	# Ready for printout!
	#
	$Radius = deg2rad($R_mu25/60.0) * $R_Sun[$k];	
    
    #
    # Terzan 11 = Terzan 12 (SIMBAD), <> Terzan 5 (Harris), avoid Terzan 12 (SIMBAD)
    # => Frommert's mu25 radius for Terzan 12 should be that of Terzan 11 (Harris) 
    # Apparently, from King profiles:
    # mu25 radius for Terzan 5 too small by ~ factor of two;
    # mu25 radii for Terzan 4, 9, 10 too small by ~ factor of three;
    # Note: mu25 radius estimates are uncertain!    
    #
    
    if ($id[$k] =~ /Terzan 12/)
    {
        $id[$k]      =~ s/Terzan 12/Terzan 11/g;
        $altname[$k] =~ s/Terzan 11/Terzan 12/g;
        $Radius      =  deg2rad(1.2/60.0) * $R_Sun[$k];        
    }
    
    if ($name[$k] =~ /Terzan 11/)
    {
        $name[$k]    =~ s/Terzan 11://g;
        $Radius      =  2 * $Radius;
    }
    
    if ($id[$k] =~ /Terzan 4/ || $id[$k] =~ /Terzan 9/ || $id[$k] =~ /Terzan 10/)
    {
        $Radius      =  3 * $Radius;
    }    
    #
    	
    if ($altname[$k] eq ""){
		print  DSC "Globular \"$name[$k]$id[$k]\"\n";
	} else {
		print  DSC "Globular \"$name[$k]$id[$k]:$altname[$k]\"\n";	
	}
	print  DSC "{\n";
#	print  DSC "        SpectralType  	      \"$SpT[$k]\"\n";
	printf DSC "        RA          	%10.4f  # [hours]\n",$alpha[$k]; 
	printf DSC "        Dec         	%10.4f  # [degrees]\n",$delta[$k]; 
	printf DSC "        Distance    	%10.4g  # [ly]\n",$R_Sun[$k]; 
	if ($R_mu25 eq "??"){
		printf DSC $format{0},$AvRadius;
	} else {
		printf DSC $format{1},$Radius;		
	}	
	if ($r_c[$k] eq "??"){
		printf DSC $format{2},$Avr_c;
	} else {
		printf DSC $format{3},$r_c[$k];
	}	
	if ($c eq "??"){
		printf DSC $format{4},$Avc;		
	} else {
		printf DSC $format{5},$c;			
	}
	if ($M_Vt[$k] eq "??"){
		printf DSC $format{6},$AvM_Vt;		
	} else {
		printf DSC $format{7},$M_Vt[$k];			
	}			
	printf DSC "        Axis       	[%8.4g %8.4g %8.4g]\n",@v;
	printf DSC "        Angle       	%10.4g  # [degrees]\n",$angle;
	print  DSC "        InfoURL  \"http\:\/\/simbad.u-strasbg.fr\/sim-id.pl\?Ident=$id[$k]\"\n";  
	print  DSC "}\n\n";
    $count++;
}
$avM /= $count;	
$avc /= $count; 
$avr_c /= $count;
$avR_mu25 /= $count;

printf "\n\nNumber of globulars         : %10d\n\n",$count;
printf "Average absolute magnitude  : %10.2f\n", $avM;
printf "Dimmest absolute magnitude  : %10.2f\n", $M_min;
printf "Average King concentration  : %10.2f\n",$avc;
printf "Average core radius [arcmin]: %10.2f\n",$avr_c;
printf "Average mu25 radius [ly]    : %10.4g\n\n",$avR_mu25;

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
sub flipnm {
my($n,$m,$namestring) = @_;
my(@tmp) = split(":",$namestring);
my($hold) = @tmp[$n];
@tmp[$n] = @tmp[$m];
@tmp[$m] = $hold;
join(":",@tmp);
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
