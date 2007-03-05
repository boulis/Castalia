<?php 
$statisticsOutputFile = "OverallResults.txt";
$fullResultsFile = "FullDetailedResults.txt";
$totalRuns = 30;

print("==== Processing Results ====\n");


$allResults = array();
for($i=0; $i<=8; $i++)
{
	for($j=0; $j<=8; $j++)
	{

		$theCommand = "cat ".$fullResultsFile."  | grep \"".$i."<--".$j."\"";
		$strResult  = `$theCommand`;
		
		$lines = explode("\n", $strResult);
		
		$k=0;
		foreach($lines as $currLine)
		{	
echo ".";
			if(ereg("(\[[0-9]<--[0-9]\]\t -->\t )([0-9][0-9]*)(\t out of \t[0-9][0-9]*)", $currLine, $regs))
				$allResults[$i][$j][++$k] = $regs[2];
		}
		$allResults[$i][$j][0] = $k;
	}
}



$deletePreviousResultFile = "rm -f $statisticsOutputFile";
system($deletePreviousResultFile);

$fh = fopen($statisticsOutputFile, 'w') or die("can't create file");

fwrite($fh, "\n\n#######################  Simulation Statistics  #######################  \n\n");
fwrite($fh, "----  Over $totalRuns runs  ----\n\n");

for($i=0; $i<=8; $i++)
{
	$stringData = "\n---------------------------------------------------------";
	$stringData .= "\n** [Node $i] as a Receiver **";
	$stringData .= "\n---------------------------------------------------------\n";

	for($j=0; $j<=8; $j++)
	{
		if($i != $j)
		{
			$tmpTotal = 0;
			for($k=1; $k<$allResults[$i][$j][0]; $k++)
			{
				$tmpTotal += $allResults[$i][$j][$k];
			}
			
			if($tmpTotal != 0)
			{
				$stringData .= "Received from Node[$j](average over all runs):  ".$tmpTotal/$totalRuns." packets out of 500\n";
	
				$stringData .= "When received something from Node[$j](average over the times that received something): ".$tmpTotal/$allResults[$i][$j][0]." packets out of 500\n\n";
			}
		}
	}
	
	fwrite($fh, $stringData);
}



fwrite($fh, "\n\n\n\n\n===========  omnetpp.ini Parameters =============    \n\n");
$copyOmnetIni = "cat omnetpp.ini";
$OmnetIniFraction = `$copyOmnetIni`;
fwrite($fh,$OmnetIniFraction);

fclose($fh);

//print_r($allResults);

?>