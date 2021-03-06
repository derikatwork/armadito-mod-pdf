package ObjectAnalysis;

use strict;

use lib::conf::Config;

my $DEBUG = "no";


# This function analyzes uri (for example detect a path traversal pattern in URI object.)
# TODO to improve
sub URI_analysis{

	my $obj_ref = shift;
	
	
	#print "Warning :: URI_analysis :: $obj_ref->{ref}\n";
	
	
	my $test = "../../../myPath";
	#my $test = "..\..\..\myPath";
	
	
	# Path traversal detection
	#if($obj_ref->{uri} =~ /([\.\.\/|\.\.\\])+/){
	#if($test =~ /(\.\.\/)+/){
	if($obj_ref->{uri} =~ /(\.\.\/)+/){
	#if($obj_ref->{uri} =~ /(\.\.\/|\.\.\\)+/){
		print "Warning :: URI_analysis :: Found path traversal in $obj_ref->{ref} URI :: $obj_ref->{uri}\n";
		
		if(exists($main::TESTS_CAT_2{"Malicious URI"})){
			$main::TESTS_CAT_2{"Malicious URI"} ++;
		}else{
			$main::TESTS_CAT_2{"Malicious URI"} =1;
		}
		
	}
	
	
	# potential dangerous pattern :: system32
	if($obj_ref->{uri} =~ /(system32)+/){
		print "Warning :: URI_analysis :: Found potentially dangerous pattern in $obj_ref->{ref} URI :: $obj_ref->{uri}\n";
		#$main::%TESTS_CAT_2{"Malicious URI"} ++;
		
		if(exists($main::TESTS_CAT_2{"Malicious URI"})){
			$main::TESTS_CAT_2{"Malicious URI"} ++;
		}else{
			$main::TESTS_CAT_2{"Malicious URI"} =1;
		}
	}
	
	

}



# The basic analysis consists to parse the content of object and detect all potential dangerous patterns.
# Returns "none" - "high" - "medium" - or "low"
sub DangerousKeywordsResearch{

	# 
	#$TESTS_CAT_2{"Dangerous Pattern High"} ;
	#$TESTS_CAT_2{"Dangerous Pattern Medium"};
	#$TESTS_CAT_2{"Dangerous Pattern Low"};

	my ($obj_ref,$content) = @_;
	
	if(!$content){
		#print "Error :: DangerousKeywordsResearch :: empty content\n";
		return "none";
	}
	
	
	
	# keywords (HIGH) :: HeapSpray - heap - spray - hack - shellcode - shell - Execute - exe - exploit - pointers - memory - exportDataObject -app.LaunchURL -byteToChar - system32  - payload
	if( $content =~ /(HeapSpray|heap|spray|hack|shellcode|shell|Execute|pointers|byteToChar|system32|payload|console)/si ){
		#$TESTS_CAT_2{"Dangerous Pattern High"} ++;
		print "Dangerous Pattern \(High\) found :: $1 :: in $obj_ref->{ref} \n";
		return "High";
	}
	
	# Unicode detection
	my @rep_unicode = ($content =~ /(\%u[a-f0-9]{4})/gi);
	my $count = @rep_unicode;
	print "unicode string = $count :: @rep_unicode\n" if ($count > 0);
	
	if($count > 10){
		print "Warning :: DangerousKeywordsResearch :: Found unicode strings :: @rep_unicode\n";
		return "High";
	}
	
	# TODO combinaison between unicode and medium

	
	# Javascript keywords (MEDIUM) :: substring - toSring - split - eval - String.replace - unescape - exportDataObject - StringfromChar - util.print
	if( $content =~ /(toString|substring|split|eval|addToolButton|String\.replace|unescape|exportDataObject|StringfromChar|util\.print)/si ){
		#$TESTS_CAT_2{"Dangerous Pattern Medium"} ++;
		print "Dangerous Pattern \(Medium\) found :: $1 :: in $obj_ref->{ref} \n";
		return "Medium";
	}
	
	
	# javascript keywords :: 
	# 
	# 
	# NOP detection "90"
	# 
	# %u... like   %u4141%u4141%u63a5%u4a80%u0000
	
	
	
	# TODO Look for JavaScript in XFA block Ex: <script name="ADBE::FileAttachment" contentType="application/x-javascript" ></script>
	
	return "none";
}


# This function detect the wide repetition of an unknown pattern
# Test1 files	:: unknown pattern repetition
# 618b5fcf762bc7397a22e568753858c9
# 6254e7e17d9796028bdc56ba81022617
# 6bffa8f1f0155a554fcdca6a1839576e
# 8e88d64028093d2ef6a633c83ee28e44
# b400e8d3635f91176e1d56a38e6aa590
# c8c39082dfca15d5ded02ca050a96112
# de8bcc90ecd0049a1ab4e5a5087359b4
# fa2ddb10d9184dba0f90c88b7786f6ec
sub Unknown_Pattern_Repetition_Detection{


	my $result = 0;
	my $objcontent = shift;
	my %h; # hash table containing the results.
	my $cpt=5; # number of characteres repetition to detect
	
	my $timeout = 5;
	my $nb_rep_max = 200;

	if(!$objcontent){
		return 0;
	}
	
	my $start_time = time - $^T;

	# Remove a white characters for a better processing
	$objcontent =~ s/\s//g;

	# split into array
	my @a =split('',$objcontent);
	
	for (my $i = 0 ; $i<= $#a-$cpt ; $i++){
	
		my $pat;
		
		# generate pattern according to number of caracter
		for (my $y=0 ; $y<$cpt ; $y++){
			$pat .= $a[$i+$y];
		}
		
		# if the pattern is already in the table
		if(!exists($h{"$pat"})){

			my $count = 0;
			# count the number of repetition in the content
			my @rep = ($objcontent =~ /\Q$pat/g);
			$count = @rep;
			#print "pat = $pat :: count = $count :: rep = $#repp \n";
			$h{"$pat"} = $count;
			
			if($count > $Config::MAX_REP_DETECTION){
				print "FOUND = $pat => $count\n\n" unless $DEBUG eq "yes";
				$result ++;
				return $result;
			}
			
		}
		
		my $time_elapsed = time - $^T;
		if($time_elapsed-$start_time > $Config::ANALYSIS_TIMEOUT ){
		
			print "TIME_EXCEEDED\n";
			return -1;
		}

	}
	
	
	# print
	my $sum=0;
	my $nb =0;
	while ((my $key, my $value) = each %h)  {
		$sum+= $value;
		$nb ++;
		#print "$key => $value\n";
	}
	
	if($nb==0){
		return 0;
	}
	
	# Calcul de l'ecart-type
	my $moyenne =0 ;# moyenne
	my $var =0; # variance
	my $et = 0; # ecart type
	
	
	$moyenne = $sum/$nb;
	
	
	print "100% => $sum :: cpt =>  $cpt :: m => $moyenne \n" unless $DEBUG eq "no";

	while ((my $key, my $value) = each %h)  {
		
		my $pourcent = ($value*100)/$sum;
		$var += ($value-$moyenne)*($value-$moyenne);
	}

	
	$var = $var/$nb;
	$et = sqrt($var);
	
	print "moyenne = $moyenne :: nb = $nb :: variance = $var :: ecartype = $et\n" unless $DEBUG eq "no";

	while ((my $key, my $value) = each %h)  {

		if($value > 2*$et  && $value > $nb_rep_max){
			print "FOUND = $key => $value :: \n\n" unless $DEBUG eq "yes";
			$result ++ ;
		}	
	}
	
	#print "END\n\n";
	return $result;

}


sub Unknown_Pattern_Repetition_Detection__{


	my $result = 0;
	my @found;
	my $objcontent = shift;
	my %h; # hash table containing the results.
	my $cpt=5; # number of characteres repetition to detect
	my $rep; # The number of repetition to reach to trigger an alert

	if(!$objcontent){
		return;
	}

	# Remove a white characters for a better processing
	$objcontent =~ s/\s//g;

	# split into array
	my @a =split('',$objcontent);

	for (my $i = 0 ; $i<= $#a-$cpt ; $i++){

		#my $pat = $a[$i].$a[$i+1];
		my $pat;

		# generate pattern according to number of caracter
		for (my $y=0 ; $y<$cpt ; $y++){
			$pat .= $a[$i+$y];
		}

		# if the pattern is already in the table
		if(exists($h{"$pat"})){
			next;
		}

		for (my $j = $i+$cpt ; $j<= $#a-$cpt ; $j++){

			my $pat2;
			# generate pattern according to number of caracter
			for (my $y=0 ; $y<$cpt ; $y++){
				$pat2 .= $a[$j+$y];
			}

			if($pat eq $pat2 && $i!=$j){


				# add in repetition hash table
				if(exists($h{"$pat"})){ # If the pattern as already been detected
					# add in offset array
					# search if the offset is already in the array
					my $in=0;
					my @tmp=@{$h{"$pat"}};
					foreach(@tmp){
						if($_ == $j){
							$in = 1;
						}
					}
					
					push($h{"$pat"}, $j) unless $in == 1;
				}else{
					my @tmp_array;
					push @tmp_array, $i;
					push @tmp_array, $j;
					$h{"$pat"}= \@tmp_array;
				}

			}
		}
	}



	my $sum=0;
	my $nb =0;
	while ((my $key, my $value) = each %h)  {

		my @arr= @{$value};
		$sum+= $#arr+1;
		$nb ++;		
	}

	# Calcul de l'ecart-type
	
	my $moyenne =0 ;# moyenne
	my $var =0; # variance
	my $et = 0; # ecart type


	if($nb > 0){
		$moyenne = $sum/$nb;
	}
	print "100% => $sum :: cpt =>  $cpt :: m => $moyenne \n" unless $DEBUG eq "no";

	while ((my $key, my $value) = each %h)  {
		my @arr= @{$value};
		my $rep = $#arr+1;
		my $pourcent = ($rep*100)/$sum;
		#print "$key => $rep ::: $pourcent %\n\n";

		$var += ($rep-$moyenne)*($rep-$moyenne);
		
	}

	if($nb > 0){
		$var = $var/$nb;
		$et = sqrt($var);
	}
	
	
	print "moyenne = $moyenne :: nb = $nb :: variance = $var :: ecartype = $et\n" unless $DEBUG eq "no";

	while ((my $key, my $value) = each %h)  {

		my @arr= @{$value};
		my $rep = $#arr+1;

		if($rep > 2*$et  && $rep > 30){
			print "FOUND = $key => $rep\n\n" unless $DEBUG eq "yes";
			$result ++ ;
		}	
	}
	
	
	#if($result > 0){
	#	$TESTS_CAT_2{"Pattern Repetition"} = "DETECTED";
	#}
	
	return $result;
	

}




# This function detect a shellcode or suite of hexa insertion
# Test2  files	:: shellcode or hexa insertion
# 5c08ea688165940008949a86805ff1d0
# 5f27adfa55628ea4674348351e241be8
# 73b0e8c5a7e5814c723295313ce0262d
# 75c1ae242d07bb738a5d9a9766c2a7de
# 7bcb4c9c35e01bd985f74aec66c19876
# 84d860a4c9e8d2baec983ef35789449a
# ab3f72df228715e6265cb222c586254e
# b823473c7206d64fa3ce20c4669b707d
# d785f43c523bf36d1678da84fa84617f
# edab6ed2809f739b67667e8fed689992
sub Shellcode_Detection{

	my $objcontent = shift;
	my $res = 0;
	my @found;
	
	if(!$objcontent){
		return 0;
	}

	# Remove white space for a better processing
	$objcontent =~ s/\s//g;

	
	# Shellcode detection // ou repetition de chiffres, separated by an element (,_\-...)

	# 73b0e8c5a7e5814c723295313ce0262d
	# 5f27adfa55628ea4674348351e241be8
	# 5c08ea688165940008949a86805ff1d0
	# 73b0e8c5a7e5814c723295313ce0262d
	# 7bcb4c9c35e01bd985f74aec66c19876
	# d785f43c523bf36d1678da84fa84617f
	# 75c1ae242d07bb738a5d9a9766c2a7de
	# ab3f72df228715e6265cb222c586254e
	# b823473c7206d64fa3ce20c4669b707d
	if( $objcontent =~ /(([\d]{1,2}[\/,%\$@^_]{1,2}){100})/ig){
		print "\n\n:::TEST 2:::\n" unless $DEBUG eq "no";
		print "DANGEROUS PATTERN 1 FOUND !!\n" unless $DEBUG eq "no";
		$res ++;
		push @found, $1;
		#print "$1\n";
		
		# TODO look for "split" pattern (or medium dangerous pattern)
		
		
	}


	if( $objcontent =~ /(([\d]{1,}[\/,%\$@^_-]{1,2}){100})/ig){
		print "\n\n:::TEST 2:::\n" unless $DEBUG eq "no";
		print "DANGEROUS PATTERN 1.1 FOUND !!\n" unless $DEBUG eq "no";
		$res ++;
		push @found, $1;
		print "$1\n" unless $DEBUG eq "no";

		# TODO look for "split" pattern (or medium dangerous pattern)
		
	}

	#pat = 9804c-9686c7351c-7254c27757c-27643c18532c-18500c32447c-32352c28309c-28201c10773c-10724c12582c-12521c
	# 84d860a4c9e8d2baec983ef35789449a
	#if( $objcontent =~ /([\dABCDEF\-]{100})/ig){
	if( $objcontent =~ /(([\dABCDEF]{2,}[-]){100})/ig){
		print "\n\n:::TEST 2:::\n" unless $DEBUG eq "no";
		print "DANGEROUS PATTERN 2 FOUND !!\n" unless $DEBUG eq "no";
		$res ++;
		push @found, $1;
		print "$1\n" unless $DEBUG eq "no";	
	}

	# edab6ed2809f739b67667e8fed689992
	#if( $objcontent =~ /([\d\/A-z,]{100})/ig){

	#if($res eq "true"){
	#	$TESTS_CAT_2{"Shellcode"} = "DETECTED";
	#}
	

	return $res;

}



1;
