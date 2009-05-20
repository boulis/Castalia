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
$allRuns = explode("Preparing for Run", $strResult);


$allResults = array();
for($i=0; $i<=$totalNodes-1; $i++) //each receiver
	for($j=0; $j<=$totalNodes-1; $j++) //each sender
		for($currRun=1; $currRun<count($allRuns);  $currRun++)
			$allResults[$currRun][$i][$j] = 0;


$currRunID = -1;
$totalPackets = 0;
foreach($allRuns as $run)
{
	$currRunID++;

	$lines = explode("\n", $run);
	
	foreach($lines as $currLine)
	{
		if(strstr($currLine, "<--"))
		{
			if(ereg("(\[)([0-9])(<--)([0-9])(\]\t -->\t )([0-9][0-9]*)(\t out of \t)([0-9][0-9]*)", $currLine, $regs))
			{	
				$allResults[$currRunID][$regs[2]][$regs[4]] = $regs[6];
				$totalPackets = $regs[8]; //this line is enough to be executed only once
			}
		}
	}
}



//======== CALCULATE THE DEAD LINKS  ===========
$deadLinks = array();
for($i=0; $i<=$totalNodes-1; $i++) //each receiver
	for($j=0; $j<=$totalNodes-1; $j++) //each sender
		$deadLinks[$i][$j] = 0;

for($i=0; $i<=$totalNodes-1; $i++) //each receiver
{
	for($j=0; $j<=$totalNodes-1; $j++) //each sender
	{
		if($i != $j)
		{
			$linkIsDead = true;
			for($currRun=1; $currRun<count($allRuns); $currRun++)
			{
				if($allResults[$currRun][$i][$j] != 0)
					$linkIsDead = false;
			}
			
			if($linkIsDead)
				$deadLinks[$i][$j] = 1;
		}
	}
}


//==========================================================================================
//===================== COMPUTE THE HISTOGRAM =============================================
//=========================================================================================
$histogram = array();
for($currRun=1; $currRun<count($allRuns);  $currRun++)
{
	$histogram[$currRun]["0-15%"] = 0;
	$histogram[$currRun]["15-30%"] = 0;
	$histogram[$currRun]["30-50%"] = 0;
	$histogram[$currRun]["50-60%"] = 0;
	$histogram[$currRun]["60-70%"] = 0;
	$histogram[$currRun]["70-80%"] = 0;
	$histogram[$currRun]["80-85%"] = 0;
	$histogram[$currRun]["85-90%"] = 0;
	$histogram[$currRun]["90-95%"] = 0;
	$histogram[$currRun]["95-100%"] = 0;
}

for($currRun=1; $currRun<count($allRuns);  $currRun++)
{
	for($i=0; $i<=$totalNodes-1; $i++) //each receiver
	{
		for($j=0; $j<=$totalNodes-1; $j++) //each sender
		{
			if($i != $j)
			{
				$currLinkQuality = $allResults[$currRun][$i][$j]/$totalPackets;
				
				if(($currLinkQuality < 0.15))
				{	
					if($deadLinks[$i][$j] != 1)
						$histogram[$currRun]["0-15%"]++;
				}
				else
				if($currLinkQuality < 0.30)
					$histogram[$currRun]["15-30%"]++;
				else
				if($currLinkQuality < 0.5)
					$histogram[$currRun]["30-50%"]++;
				else
				if($currLinkQuality < 0.6)
					$histogram[$currRun]["50-60%"]++;				
				else
				if($currLinkQuality < 0.7)
					$histogram[$currRun]["60-70%"]++;
				else
				if($currLinkQuality < 0.8)
					$histogram[$currRun]["70-80%"]++;
				else
				if($currLinkQuality < 0.85)
					$histogram[$currRun]["80-85%"]++;
				else
				if($currLinkQuality < 0.9)
					$histogram[$currRun]["85-90%"]++;
				else
				if($currLinkQuality < 0.95)
					$histogram[$currRun]["90-95%"]++;
				else
					$histogram[$currRun]["95-100%"]++;
			}
		}
	}
}


$deletePreviousResultFile = "rm -f $statisticsOutputFile";
system($deletePreviousResultFile);

$fh = fopen($statisticsOutputFile, 'w') or die("can't create file");

fwrite($fh, "\n\n#######################  Simulation Statistics  #######################  \n\n");

for($currRun=1; $currRun<count($allRuns);  $currRun++)
{
	fwrite($fh, "----  Histogram of simulation Run $currRun  ----\n");
	$stringData = "";

	$stringData .=  "[0-15%]   ==> ".$histogram[$currRun]["0-15%"]/$totalValidLinks."\n";
	$stringData .=  "[15-30%]   ==> ".$histogram[$currRun]["15-30%"]/$totalValidLinks."\n";
	$stringData .=  "[30-50%]  ==> ".$histogram[$currRun]["30-50%"]/$totalValidLinks."\n";
	$stringData .=  "[50-60%]  ==> ".$histogram[$currRun]["50-60%"]/$totalValidLinks."\n";
	$stringData .=  "[60-70%]  ==> ".$histogram[$currRun]["60-70%"]/$totalValidLinks."\n";
	$stringData .=  "[70-80%]  ==> ".$histogram[$currRun]["70-80%"]/$totalValidLinks."\n";
	$stringData .=  "[80-85%]  ==> ".$histogram[$currRun]["80-85%"]/$totalValidLinks."\n";
	$stringData .=  "[85-90%]  ==> ".$histogram[$currRun]["85-90%"]/$totalValidLinks."\n";
	$stringData .=  "[90-95%]  ==> ".$histogram[$currRun]["90-95%"]/$totalValidLinks."\n";
	$stringData .=  "[95-100%] ==> ".$histogram[$currRun]["95-100%"]/$totalValidLinks."\n\n\n";

	fwrite($fh, $stringData);
}


$class["A"] = 0;
$class["B"] = 0;
$class["C"] = 0;
$class["D"] = 0;
$class["E"] = 0;
$class["F"] = 0;
$class["G"] = 0;
$class["H"] = 0;
$class["I"] = 0;
$class["j"] = 0;

for($currRun=1; $currRun<count($allRuns);  $currRun++)
{
	$class["A"] += $histogram[$currRun]["0-15%"];
	$class["B"] += $histogram[$currRun]["15-30%"];
	$class["C"] += $histogram[$currRun]["30-50%"];
	$class["D"] += $histogram[$currRun]["50-60%"];
	$class["E"] += $histogram[$currRun]["60-70%"];
	$class["F"] += $histogram[$currRun]["70-80%"];
	$class["G"] += $histogram[$currRun]["80-85%"];
	$class["H"] += $histogram[$currRun]["85-90%"];
	$class["I"] += $histogram[$currRun]["90-95%"];
	$class["J"] += $histogram[$currRun]["95-100%"];
}
fwrite($fh, "----  Averaged Histogram over $totalRuns runs  ----\n");
$stringData = "";

$stringData .=  "[0-15%]   ==> ".($class["A"]/(count($allRuns)-1))/$totalValidLinks."\n";
$stringData .=  "[15-30%]  ==> ".($class["B"]/(count($allRuns)-1))/$totalValidLinks."\n";
$stringData .=  "[30-50%]  ==> ".($class["C"]/(count($allRuns)-1))/$totalValidLinks."\n";
$stringData .=  "[50-60%]  ==> ".($class["D"]/(count($allRuns)-1))/$totalValidLinks."\n";
$stringData .=  "[60-70%]  ==> ".($class["E"]/(count($allRuns)-1))/$totalValidLinks."\n";
$stringData .=  "[70-80%]  ==> ".($class["F"]/(count($allRuns)-1))/$totalValidLinks."\n";
$stringData .=  "[80-85%]  ==> ".($class["G"]/(count($allRuns)-1))/$totalValidLinks."\n";
$stringData .=  "[85-90%]  ==> ".($class["H"]/(count($allRuns)-1))/$totalValidLinks."\n";
$stringData .=  "[90-95%]  ==> ".($class["I"]/(count($allRuns)-1))/$totalValidLinks."\n";
$stringData .=  "[95-100%] ==> ".($class["J"]/(count($allRuns)-1))/$totalValidLinks."\n\n\n";

fwrite($fh, $stringData);






//=========================================================================================
//===================== COMPUTE THE BIDIRECTIONAL LINKS ===================================
//=========================================================================================
$bidiLinks = array();
for($i=0; $i<=$totalNodes-1; $i++) //each receiver
	for($j=0; $j<=$totalNodes-1; $j++) //each sender
		for($currRun=1; $currRun<count($allRuns);  $currRun++)
			$bidiLinks[$currRun][$i][$j] = -1;//initial value

for($currRun=1; $currRun<count($allRuns);  $currRun++)
	for($i=0; $i<=$totalNodes-1; $i++) //each unique receiver-sender pair
		for($j=$i; $j<=$totalNodes-1; $j++)
		{
			if($i != $j)
			{
				//$min = ($allResults[$currRun][$i][$j] < $allResults[$currRun][$j][$i])?$allResults[$currRun][$i][$j]:$allResults[$currRun][$j][$i];
				$diff = $allResults[$currRun][$i][$j] - $allResults[$currRun][$j][$i];
				$abs_diff = ($diff < 0)?-$diff:$diff;
				
				$theValue = 0;
				
				if($abs_diff == 0)
				{	
					if( ($allResults[$currRun][$i][$j] == 0) || ($allResults[$currRun][$j][$i] == 0) )
						$theValue = -2; //disconnected both directions
				}
				else
					$theValue = $abs_diff;
				
				$bidiLinks[$currRun][$i][$j] = $theValue;
				$bidiLinks[$currRun][$j][$i] = $theValue;
			}
		}


$overallSymmetricLinks = array();
for($currRun=1; $currRun<count($allRuns);  $currRun++)
{
	$overallSymmetricLinks[$currRun]["BothDisconnected"] = 0;
	$overallSymmetricLinks[$currRun]["AverageDiff"] = 0;
}

for($currRun=1; $currRun<count($allRuns);  $currRun++)
{
	fwrite($fh, "----  Link symmetricity of simulation Run $currRun  ----\n");
	$stringData = "";
	$tmpSum = 0;
	$sumCounter = 0;	
	$bidiDeadLinks = 0;

	for($i=0; $i<=$totalNodes-1; $i++) //each unique receiver-sender pair
		for($j=$i; $j<=$totalNodes-1; $j++)
		{
			if($i != $j)
			{
				if($bidiLinks[$currRun][$i][$j] == -1)
					$stringData .= "[$i--$j]   ==> SAME NODE\n";
				else
				if($bidiLinks[$currRun][$i][$j] == -2)
				{	
					$stringData .= "[$i--$j]   ==> Disconnected both ways\n";
					$bidiDeadLinks++;
				}
				else
				{	
					$stringData .=  "[$i--$j]   ==> ".$bidiLinks[$currRun][$i][$j]."\n";
					$tmpSum += $bidiLinks[$currRun][$i][$j];
					$sumCounter++;
				}
			}
		}
	
	$averageDifference = ($tmpSum/$sumCounter);
	if($sumCounter != 0)
		$stringData .=  "\nAverage difference of the non symmetric links: ".$averageDifference." out of ".$totalPackets."  (".$averageDifference*100/$totalPackets." %)";
	else
		$stringData .=  "\nAverage difference of the non symmetric links: All nodes are both-ways disconnected";
	$stringData .=  "\nNumber of both-ways dead links: ".$bidiDeadLinks." * 2 = ".($bidiDeadLinks*2)." out of $totalValidLinks (".($bidiDeadLinks*2*100/$totalValidLinks)." %)\n\n\n\n";
	
	$overallSymmetricLinks[$currRun]["BothDisconnected"] = $bidiDeadLinks*2;

	if($sumCounter != 0)
		$overallSymmetricLinks[$currRun]["AverageDiff"] = ($tmpSum/$sumCounter);
	else
		$overallSymmetricLinks[$currRun]["AverageDiff"] = 0;

	fwrite($fh, $stringData);
}

$avgBothDisconnected = 0;
$avgAverageDiff = 0;
for($currRun=1; $currRun<count($allRuns);  $currRun++)
{
	$avgBothDisconnected += $overallSymmetricLinks[$currRun]["BothDisconnected"];
	$avgAverageDiff += $overallSymmetricLinks[$currRun]["AverageDiff"];
}
$avgBothDisconnected /= $totalRuns;
$avgAverageDiff /= $totalRuns;

fwrite($fh, "----  Averaged link symmetricity after simulation over $totalRuns runs  ----\n");
$stringData = "";
$stringData .=  "\nAverage difference of the non symmetric links: ".$avgAverageDiff." packets out of ".$totalPackets."(".($avgAverageDiff*100/$totalPackets)."%)";
$stringData .=  "\nNumber of both-ways dead links: ".$avgBothDisconnected."(".($avgBothDisconnected*100/$totalValidLinks)."%)";

fwrite($fh, $stringData);

fclose($fh);

?>