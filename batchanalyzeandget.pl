#!/usr/bin/perl 
use warnings;
use strict;

# does the batch analysis and stat collection.
# takes as input 
# the partial program number
# and file suffixes (in "-suffix" form)
# and chip numbers

# default will parse the no-suffix case.
my $pptime = shift @ARGV;
my $chips = shift @ARGV;
my @suffixes = @ARGV;

my $analyzecommand = '..\..\analyze-slc-partialprogram.pl';
my $rawpath = '..\..\programtime\slc\raw';
my $rawfilename = "partialprogram-$pptime";
my $rawextension = ".hex";
my $parsedpath = '..\..\programtime\slc\parsed';
my $parsedfilename = "partialprogram-$pptime";
my $parsedextension = ".txt";

my $getstatscommand = '..\..\getstats-slc-pp.pl';

foreach my $chipnum (1..$chips) {
	foreach my $suffix (@suffixes) {

		my $currentrawfile = "$rawpath" . "\\chip$chipnum\\" . "$rawfilename" . "$suffix" . "$rawextension";
		my $currentparsedfile = "$parsedpath" . "\\chip$chipnum\\" . "$parsedfilename" . "$suffix" . "$parsedextension";
	
		unless (-e $currentparsedfile) {
		
			print($analyzecommand . " $currentrawfile > $currentparsedfile");
			print "\n\n";
			system($analyzecommand . " $currentrawfile > $currentparsedfile");
		
		}
		
		# if (-e $currentparsedfile) {
			# print($getstatscommand . " $currentparsedfile");
			# print "\n\n";
			# system($getstatscommand . " $currentparsedfile");
		# }
		
	}
}