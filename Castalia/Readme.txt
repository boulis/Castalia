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



*** Directory Strucure ***

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

Makefile      --> This file is created after the invocation of the makemake script. In general,
		  a Makefile is created/generated for every directory inside the src folder.

makemake      --> The script that the user uses to generate the appropriate Makefiles. It uses 
                  OMNeT's opp_makemake tool. It also uses the configuration files inside the 
		  ./config directory.

nedfiles.lst  --> This file is produced automatically after the run of makemake and contains
		  all the ned files of the source of Castalia with their relative paths. 
                  It is used inside the omnet.ini file to include all the modules of the Castalia simulator. 
                  All the listed modules are loaded in the runtime dynamically.

Readme.txt    --> This file.

VERSION	      --> This file hold the current version/build of Castalia




*** How to build Castalia ***
The source files of Castalia have a directory structure that follows the logical structure
its modules. Due to the file structure complexity, it would be dufficult for the user to 
create a custom script to build the simulator everytime its sources are modified.
Insteado fthis, the user modifies one configuration file and runs the standard build script
of Castalia. The procedure to build Castalia is quite similar to that of the INET framework
for OMNeT++.
The steps to build Castalia from its sources are:

1) Add your new Modules (simple or compound) by creating *.ned, *.cc, *.h files under Castalia/src
   Note: If you haven't added/created any new source file (you just modified the existing code) you can
   jump to "STEP 8)"

2) If you create a new module then try to place its sources in such a place (in a new directory under Castalia/src)
   so that the module structure is reflected on the directory structure. For example if you create a new MAC 
   protocol (named myMAC), place the ned file and the sources inside Castalia/src/Node/Communication/MAC/MyMAC folder
   after creating this first.

3) Add whenever you like (make sure that you remember where) your msg files. Usually it is more convenient
   to add them inside the directory where the *.ned files of the modules that use them, reside. If more than
   one modules use this msg file then place it to the lower upper-most directory that includes all these 
   module folders.

4) Change to directory "Castalia/config" and modify the file "Castalia.config".
   Make sure that the "ROOT" variable contains the full path of the root directory of Castalia.

5) Change to directory "Castalia/config" and modify the file "makemakefiles"
   This file is used by the script "makemake" and contains information on how to generate
   Makefile files at each sub-directory of the Castalia/src folder.
   
   * Variables:
   a)OPTS ==> here you can add/modify the options of the opp_makemake tool. Usually it has not to be modified at all.
   b)ALL_WSN_INCLUDES ==> here you MUST add every directory that either contains an *.h (C++ header) file or
                          it will contain one after the invocation of opp_makemake by the Castalia/makemake script (usually 
                          the *.msg files produce extra header files). What you have to do is to add all the directories
                          with the *.h or *.msg files that you created in the form of: 
				-I$(SRCDIR)/Your_include_Dir_Under_Castalia_src.
   * Make Targets:
   You MUST modify the "WSNetwork" target. Most of the times you need to add a line like:
        cd $(SRCDIR)/Your_Dir_Under_Castalia_src && $(MAKEMAKE) $(OPTS) -n -r -I$(SRCDIR)/Your_include_Dir_A_Under_Castalia_src
   for each new directory that you have created and that contains source(*.ned, *.cc, *.h) or *.msg files.
   ATTENTION :  You may need to add more than one -I$(SRCDIR)/Your_include_Dir_XX_Under_Castalia_src flag at the end of 
		this line if you use a source file from a different directory

6) You don't have to create your own omnetpp.config. It is generated automatically after the execution of "makemake".

7) You don't have to create your own nedfiles.lst. It is generated automatically after the execution of "makemake".

8) You may wish to add dependencies across directories. 
   To do that, create a file with name "makefrag" and place it inside the directory where you wish to add the dependencies
   among its subdirectories/modules. You can add as many "makefrag" files as the total number of the produced Makefile 
   files by the "makemake" script. The opp_makemake injects/includes the "makefrag" inside the generated Makefile.
   Adding "makefrag" files is a good way to keep some extra settings inside the Makefile files without loosing it every time 
   they are regenerated by the makemake script (which uses the opp_makemake tool).

9) If there are some extra/external libraries/header files that you'd like to include inside all the produced Makefile files
   then uncomment the two last lines of Castalia/config/Castalia.config and modify them as you wish.


10)Change to directory Castalia/  and type "./makemake"
   This will produce all the needed Makefile files automatically.

11)Change to directory Castalia/ (if you are not already in) and type make.

12)The Castalia simulator is built and ready for use.
   Check the Castalia/nedfiles.lst to all the ned files that have been found inside the sources.
   Check the Castalia/Bin to see the CastaliaBin executable binary with the core functionalities of the simulator.
   This binary is invoked everytime you need to run a simulation.


