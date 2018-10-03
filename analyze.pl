use v5.14;
use Chart::Gnuplot;
use constant use_ftrace => 0;

my @y;

for(<>) {
	my @fields = split ' ';

	if (!!use_ftrace) {
		if ($fields[4] =~ /mmc_start_req_cmd/ and ($fields[6] eq "CMD24" or $fields[6] eq "CMD25")) {
			my $addr = hex ((split '=', $fields[7])[1]);
			my $nblocks = $fields[6] ne "CMD25" ? 1 : int ((split '=', $fields[10])[1]);
			push @y, $addr;
		};
	} else {
		# $ blkparse -f"%d %n %S %C\n" ... <device>
		last if (@fields != 3);
		if ($fields[0] =~ /W/) {
			my $addr = $fields[2];
			my $nblocks = $fields[1];
			push @y, $addr;
		}
	}
}

my @x = ( 0 .. @y-1 );

# Create chart object and specify the properties of the chart
my $chart = Chart::Gnuplot->new(
    terminal => "wxt",
    title  => "Simple testing",
    xlabel => "My x-axis label",
    ylabel => "My y-axis label",
);
 
# Create dataset object and specify the properties of the dataset
my $dataSet = Chart::Gnuplot::DataSet->new(
    xdata => \@x,
    ydata => \@y,
    title => "Plotting a line from Perl arrays",
    style => "points",
);
 
# Plot the data set on the chart
$chart->plot2d($dataSet);
