
This folder contains an automated test script for Castalia simulations

Key files are:

- runTests - the main script
- targets.txt - file with a list of simulations to be tested (refers to folders within 
		the Simulations directory of Castalia
- exceptions.txt - file with a list of substrings. Any line in Castalia output 
		containing any of those substrings will be ignored when comparing results
		from running a simulation with those stored in repository
		
The main script can be executed in two modes:

1) Snapshot mode:
./runTests snapshot

Will go through all simulations from targets.txt file, saving corresponding omnetpp.ini file
in repository, together with any other parameter files which are referenced from within it.
Once input files are saved, Castalia will be executed and its output will also be stored in
repository. This will create a snapshot of each simulation - a static copy of input and 
output - which can be later used for reference.


2) Test mode:
./runTests 
or
./runTests run

Will go through all simulations from targets.txt, execute Castalia using saved omnetpp.ini 
file as input and compare the results with the copy in repository. If any difference is
found, the test is considered as a fail and the output of 'diff' command is printed 
		
		    