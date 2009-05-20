//***************************************************************************************
//*  Copyright: National ICT Australia,  2007, 2008					*
//*  Developed at the Networks and Pervasive Computing program				*
//*  Author(s): Athanassios Boulis, Dimosthenis Pediaditakis				*
//*  This file is distributed under the terms in the attached LICENSE file.		*
//*  If you do not find this file, copies can be found by writing to:			*
//*											*
//*      NICTA, Locked Bag 9013, Alexandria, NSW 1435, Australia			*
//*      Attention:  License Inquiry.							*
//*											*
//***************************************************************************************



<?php 
$statisticsOutputFile = "OverallResults.txt";
$fullResultsFile = "FullDetailedResults_perRun.txt";
$totalRuns = 50;
$totalNodes = 9;
$totalValidLinks = $totalNodes*($totalNodes-1);

print("==== Processing Results ====\n");


$theCommand = "cat ".$fullResultsFile;
$strResult  = `$theCommand`;
$allRuns = split("Preparing for Run #[0-9][0-9]*...", $strResult);
#$allRuns = split("run [0-9][0-9]* \"SN\"", $strResult);


$allResults = array();
for($currRun=1; $currRun<count($allRuns);  $currRun++)
	$allResults[$currRun] = 0;


$currRunID = 0;
$fistrNullRun = true;
foreach($allRuns as $run)
{
	if(!$fistrNullRun)
	{
		$totalNodesReceived = 0;
	
		$currRunID++;
	
		$lines = explode("\n", $run);
		
		foreach($lines as $currLine)
		{
			if( (strstr($currLine, "Node [")) && (strstr($currLine, "] Value")))
			{
				if(!strstr($currLine, "] Value: 0"))
				{
					$totalNodesReceived++;
				}
			}
		}
		
		$allResults[$currRunID] = $totalNodesReceived / $totalNodes;
	}
	
	$fistrNullRun = false;
}

$deletePreviousResultFile = "rm -f $statisticsOutputFile";
system($deletePreviousResultFile);

$fh = fopen($statisticsOutputFile, 'w') or die("can't create file");

fwrite($fh, "\n\n#######################  Simulation Statistics  #######################  \n\n");

$sumPropagations = 0;
$sumPropagationsNo_01 = 0;
$countPropagationsNo_01 = 0;
$sumPropagationsNo_012 = 0;
$countPropagationsNo_012 = 0;
for($currRun=1; $currRun<=$totalRuns;  $currRun++)
{
	if( ($currRun != 1) && ($currRun != 12) && ($currRun != 17) && ($currRun != 22) && ($currRun != 25) && ($currRun != 27) && ($currRun != 38) && ($currRun != 41) && ($currRun != 46) && ($currRun != 48) )
	{
 		$countPropagationsNo_01++;	
		$sumPropagationsNo_01 += $allResults[$currRun];
		
		if( ($currRun != 4) && ($currRun != 5) && ($currRun != 6) && ($currRun != 9) && ($currRun != 15) && ($currRun != 28) && ($currRun != 40) )
		{
			$countPropagationsNo_012++;
			$sumPropagationsNo_012 += $allResults[$currRun];
		}
	}
	
	$sumPropagations += $allResults[$currRun];

	$stringData = "";
	$stringData .=  "In simulation [run $currRun] ".($allResults[$currRun]*100)."% of the nodes got the value\n\n";

	fwrite($fh, $stringData);
}

$stringData = "";
$stringData .=  "\n\n\n=========  Averaged Results over all Runs ==========\n\n";
$stringData .=  "ALL SEEDS: \t\t".($sumPropagations*100/$totalRuns)."% of the nodes got the value\n\n";
$stringData .=  "NO 0/9, 1/9 SEEDS: \t".($sumPropagationsNo_01*100/$countPropagationsNo_01)."% of the nodes got the value\n\n";
$stringData .=  "NO 0/9, 1/9, 2/9 SEEDS: ".($sumPropagationsNo_012*100/$countPropagationsNo_012)."% of the nodes got the value\n\n";
fwrite($fh, $stringData);

fclose($fh);

?>