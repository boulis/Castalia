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


// The hardcoded connectivity map is derived from a real experiment.
// The following variable holds the ID of that experiment.
// Will need that ID in order to compare the results of the simulator
// with the real experiment under identical connectivities between nodes
$experimentID = 14;

print("==== Processing Results ====\n");


$theCommand = "cat ".$fullResultsFile;
$strResult  = `$theCommand`;
$allRuns = split("Preparing for Run #[0-9][0-9]*...", $strResult);
#$allRuns = split("run [0-9][0-9]* \"SN\"", $strResult);


$allResults = array();
for($currRun=1; $currRun<count($allRuns);  $currRun++)
	$allResults[$currRun] = 0;



// Data that come from a real wireles sensor node deployment of 9 nodes
// $experiment[i][j] = k
// i: the ID of the experiment
// j: the ID of the node
// k: can be 1(true) of (0) false and represents wether or not the node j received a value 
//    at experiment with ID i

//Experiment 01 (Central Case)
$experiment[1][0] = 1;
$experiment[1][1] = 1;
$experiment[1][2] = 1;
$experiment[1][3] = 1;
$experiment[1][4] = 1;
$experiment[1][5] = 1;
$experiment[1][6] = 1;
$experiment[1][7] = 1;
$experiment[1][8] = 1;
//Experiment 02
$experiment[2][0] = 1;
$experiment[2][1] = 0;
$experiment[2][2] = 0;
$experiment[2][3] = 1;
$experiment[2][4] = 1;
$experiment[2][5] = 0;
$experiment[2][6] = 1;
$experiment[2][7] = 1;
$experiment[2][8] = 0;
//Experiment 03
$experiment[3][0] = 0;
$experiment[3][1] = 0;
$experiment[3][2] = 0;
$experiment[3][3] = 1;
$experiment[3][4] = 1;
$experiment[3][5] = 0;
$experiment[3][6] = 0;
$experiment[3][7] = 0;
$experiment[3][8] = 0;
//Experiment 05
$experiment[5][0] = 1;
$experiment[5][1] = 1;
$experiment[5][2] = 0;
$experiment[5][3] = 1;
$experiment[5][4] = 1;
$experiment[5][5] = 0;
$experiment[5][6] = 1;
$experiment[5][7] = 1;
$experiment[5][8] = 1;
//Experiment 06
$experiment[6][0] = 1;
$experiment[6][1] = 1;
$experiment[6][2] = 0;
$experiment[6][3] = 1;
$experiment[6][4] = 1;
$experiment[6][5] = 0;
$experiment[6][6] = 1;
$experiment[6][7] = 1;
$experiment[6][8] = 1;
//Experiment 07
$experiment[7][0] = 1;
$experiment[7][1] = 1;
$experiment[7][2] = 0;
$experiment[7][3] = 1;
$experiment[7][4] = 1;
$experiment[7][5] = 1;
$experiment[7][6] = 1;
$experiment[7][7] = 1;
$experiment[7][8] = 1;
//Experiment 08
$experiment[8][0] = 0;
$experiment[8][1] = 0;
$experiment[8][2] = 0;
$experiment[8][3] = 1;
$experiment[8][4] = 0;
$experiment[8][5] = 0;
$experiment[8][6] = 0;
$experiment[8][7] = 0;
$experiment[8][8] = 0;
//Experiment 09
$experiment[9][0] = 0;
$experiment[9][1] = 0;
$experiment[9][2] = 0;
$experiment[9][3] = 1;
$experiment[9][4] = 1;
$experiment[9][5] = 0;
$experiment[9][6] = 0;
$experiment[9][7] = 0;
$experiment[9][8] = 0;
//Experiment 10
$experiment[10][0] = 1;
$experiment[10][1] = 1;
$experiment[10][2] = 1;
$experiment[10][3] = 1;
$experiment[10][4] = 1;
$experiment[10][5] = 1;
$experiment[10][6] = 1;
$experiment[10][7] = 1;
$experiment[10][8] = 1;
//Experiment 11
$experiment[11][0] = 1;
$experiment[11][1] = 0;
$experiment[11][2] = 0;
$experiment[11][3] = 1;
$experiment[11][4] = 1;
$experiment[11][5] = 0;
$experiment[11][6] = 1;
$experiment[11][7] = 1;
$experiment[11][8] = 0;
//Experiment 12
$experiment[12][0] = 1;
$experiment[12][1] = 1;
$experiment[12][2] = 0;
$experiment[12][3] = 1;
$experiment[12][4] = 1;
$experiment[12][5] = 0;
$experiment[12][6] = 1;
$experiment[12][7] = 1;
$experiment[12][8] = 0;
//Experiment 14
$experiment[14][0] = 0;
$experiment[14][1] = 1;
$experiment[14][2] = 1;
$experiment[14][3] = 1;
$experiment[14][4] = 1;
$experiment[14][5] = 1;
$experiment[14][6] = 1;
$experiment[14][7] = 1;
$experiment[14][8] = 1;

$simulation = array();

$currRunID = 0;

$fistrNullRun = true;

foreach($allRuns as $run)
{
	if(!$fistrNullRun)
	{
		$totalNodesReceived = 0;
	
		$currRunID++;

		for($i=0; $i<=$totalNodes-1; $i++)
			$simulation[$currRunID][$i] = 0;

		$lines = explode("\n", $run);
		
		foreach($lines as $currLine)
		{
	
			if( (strstr($currLine, "Node [")) && (strstr($currLine, "] Value")))
			{
				if(!strstr($currLine, "] Value: 0"))
				{
					if(ereg("(Node \[)([0-9][0-9]*)(\] Value: [0-9][0-9]*[\.]?[0-9]*)", $currLine, $regs))
						$simulation[$currRunID][$regs[2]] = 1;
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
for($currRun=1; $currRun<=$totalRuns;  $currRun++)
{
	$sumPropagations += $allResults[$currRun];

	$stringData = "";
	$stringData .=  "In simulation [run $currRun] ".($allResults[$currRun]*100)."% of the nodes got the value\n";
	
	$strArray =  $simulation[$currRun][8]." ".$simulation[$currRun][5]." ".$simulation[$currRun][2]."\n";
	$strArray .= $simulation[$currRun][7]." ".$simulation[$currRun][4]." ".$simulation[$currRun][1]."\n";
	$strArray .= $simulation[$currRun][6]." ".$simulation[$currRun][3]." ".$simulation[$currRun][0]."\n\n\n";

	fwrite($fh, $stringData.$strArray);
}



$simulationAvg = array();
for($i=0; $i<=8; $i++)
{
	$sumSim = 0;
	for($currRun=1; $currRun<=$totalRuns;  $currRun++)
	{
		$sumSim += $simulation[$currRun][$i];
	}
	$simulationAvg[$i] = $sumSim/$totalRuns;
}

$stringData = "";
$stringData .=  "\n\n\n=========  Averaged Results over all Runs ==========\n";
$stringData .=  ($sumPropagations*100/$totalRuns)."% of the nodes got the value\n";
$strArray =  $simulationAvg[8]." \t".$simulationAvg[5]." \t".$simulationAvg[2]."\n";
$strArray .= $simulationAvg[7]." \t".$simulationAvg[4]." \t".$simulationAvg[1]."\n";
$strArray .= $simulationAvg[6]." \t".$simulationAvg[3]." \t".$simulationAvg[0]."\n\n\n";
fwrite($fh, $stringData.$strArray);



$stringData = "";
$sumExperiment = 0;
$avgExperiment = 0;
for($i=0; $i<=8; $i++)
{
	$sumExperiment += $experiment[$experimentID][$i];
}
$avgExperiment = $sumExperiment/$totalNodes*100;
$stringData .=  "\n=========  Real Experiment ==========\n";
$stringData .=  "In real experiment ".$avgExperiment."% of the nodes got the value\n";
$strArray =  $experiment[$experimentID][8]." ".$experiment[$experimentID][5]." ".$experiment[$experimentID][2]."\n";
$strArray .= $experiment[$experimentID][7]." ".$experiment[$experimentID][4]." ".$experiment[$experimentID][1]."\n";
$strArray .= $experiment[$experimentID][6]." ".$experiment[$experimentID][3]." ".$experiment[$experimentID][0]."\n\n\n";
fwrite($fh, $stringData.$strArray);



$networkErrorAvg = $avgExperiment - ($sumPropagations*100/$totalRuns);
$networkErrorAvg = ($networkErrorAvg < 0)?-$networkErrorAvg:$networkErrorAvg;
$stringData = "";
$stringData .=  "\n=========  Averaged Errors over all Runs ==========\n";
$stringData .=  "The network error is ".$networkErrorAvg."% \n";
$stringData .=  "The nodal error array is: \n";

$nodalErr = array();
for($i=0; $i<=8; $i++)
{
	$theDIff = $simulationAvg[$i] - $experiment[$experimentID][$i];
	$nodalErr[$i] = ($theDIff < 0)?-$theDIff:$theDIff;
}
$strArray =  $nodalErr[8]." \t".$nodalErr[5]." \t".$nodalErr[2]."\n";
$strArray .= $nodalErr[7]." \t".$nodalErr[4]." \t".$nodalErr[1]."\n";
$strArray .= $nodalErr[6]." \t".$nodalErr[3]." \t".$nodalErr[0]."\n\n\n";
fwrite($fh, $stringData.$strArray);


fclose($fh);

?>