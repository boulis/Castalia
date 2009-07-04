# *******************************************************************************
# *  Copyright: National ICT Australia,  2007, 2008, 2009			*
# *  Developed at the Networks and Pervasive Computing program  		*
# *  Author(s): Athanassios Boulis, Dimosthenis Pediaditakis			*
# *  This file is distributed under the terms in the attached LICENSE file.	*
# *  If you do not find this file, copies can be found by writing to:		*
# *										*
# *      NICTA, Locked Bag 9013, Alexandria, NSW 1435, Australia		*
# *      Attention:  License Inquiry.						*
# *										*
# *******************************************************************************/


In the Root directory (Castalia/) there are the following contents :

bin           --> Contains the produced executable of Castalia (CastaliaBin)

config        --> Contains the configuration files needed for building the Makefile files

Simulations   --> For each implemented application module in Castalia, there is a test simulation
		  that allows the user to run experiments with his own parameter settings.
		  Inside the Castalia/Simulations directory there are four subdirectories each one
		  holding the simulation files of the respective application.

src           --> Here resides the source code of Castalia (*.cc, *.h, *.msg, *.ned files)
                  The directory's structure corresponds to the structure of the 
                  compound modules and submodules that Castalia has. Each compound module 
                  resides in a separate forlder. This folder contains as many subfolders 
                  as the number of the used sub-modules by this compound module (and so forth).

Tests         --> This directory contains files for automated testing. A script runs many
                  chosen simulation scenarios and compares the results with previously
                  saved data. 

Makefile      --> This file is created after the invocation of the makemake script. In general,
		  a Makefile is created/generated for every directory inside the src folder.

makemake      --> The script that the user uses to generate the appropriate Makefiles. It uses 
                  OMNeT's opp_makemake tool. It also uses the configuration files inside the 
		  ./config directory.

nedfiles.lst  --> This file is produced automatically after the run of makemake and contains
		  all the ned files of the source of Castalia with their relative paths. 
                  It is used inside the omnet.ini file to include all the modules of the 
                  Castalia simulator. All the listed modules are loaded in the runtime dynamically.

Readme.txt    --> This file.

VERSION	      --> This file hold the current version/build of Castalia


    ===============  READ THE INSTALATION GUIDE AND THE USER'S MANUAL  ==============
