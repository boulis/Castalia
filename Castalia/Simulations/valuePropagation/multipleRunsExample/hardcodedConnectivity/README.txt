# ***************************************************************************************
# *  Copyright: National ICT Australia,  2007, 2008					*
# *  Developed at the Networks and Pervasive Computing program				*
# *  Author(s): Athanassios Boulis, Dimosthenis Pediaditakis				*
# *  This file is distributed under the terms in the attached LICENSE file.		*
# *  If you do not find this file, copies can be found by writing to:			*
# *											*
# *      NICTA, Locked Bag 9013, Alexandria, NSW 1435, Australia			*
# *      Attention:  License Inquiry.							*
# *											*
# **************************************************************************************/




1) Check the relative path to the nedfiles.lst found in omnetpp.ini:
     preload-ned-files = *.ned @../../../../nedfiles.lst
   Make sure it points to $(CASTALIA_HOME)/nedfiles.lst 

2) Check the castaliaHome_RelativePath variable found in the runMultiValProp script.
   Make sure it points to $(CASTALIA_HOME)

3) Modify the following variables in produce_statistics_hardcoded.php file accrding to your simulation settings:
     $totalRuns = 50;
     $totalNodes = 9;
   Note that the php script requires all runs to have the same number of nodes.

4) Run:   ./runMultiValProp

5) Check the  file: Castalia-Primary-Output.txt for the generated statistics

NOTES: 
       a)  "runMultiValProp" produces the overall results (over all runs) using a script written in PHP (produce_statistics_hardcoded.php)
           In order this script to run the user should have installed php4-cli or php5-cli in order to run PHP scripts from  a shell
       b)  The full-detailed results per run are written to a file called "FullDetailedResults_perRun.txt".
           Note that this file is overwritten every time the user runs the "runMultiValProp" script.
