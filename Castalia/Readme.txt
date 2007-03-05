-----------------------------------------
-------- HOW TO BUILD CASTALIA ----------
-----------------------------------------



*** Directory Strucure ***

In the Root directory (Castalia/) there are the following contents :

bin           --> Here is placed the produced executable of Castalia (CastaliaBin)

config        --> Contains the configuration files needed for building the Makefile files

src           --> Here resides the source code of Castalia (*.cc, *.h, *.msg, *.ned files)
                  The directory's structure is respective to that of the structure of the 
                  compound modules and submodules that Castalia has. Each compound module 
                  resides in a separate forlder. This folder contains as many subfolders 
                  as the number of the used sub-modules by this compound module (and so forth).

test          --> A test simulation that uses the binary of Castalia & exploints the feature of 
                  dynamic NED loading.

Makefile      --> This file is created after the invocation of the makemake script. In general, it is
                  created/generated a Makefile for every directory inside the src folder.

makemake      --> The script that the user uses to generate the appropriate Makefiles.Of course it uses 
                  the opp_makemake tool. It uses the configuration files inside the ./config directory.

nedfiles.lst  --> This file is produced automatically after the run of makemake. This file is extremely 
                  useful as it contains all the ned files of the source of Castalia with their relative paths. 
                  It is used inside the omnet.ini file to include all the modules of the Castalia simulator. 
                  All the listed modules are loaded in the runtime dynamically.

Readme.txt    --> This file.





*** How to build Castalia ***
The procedure to build Castalia is quite similar to that of INET framework for Omnet++.
Because the source files of Castalia have a directory structure similar to the logical structure
of its modular structure, it would be dufficult to the user to create a custom script to build
the simulator everytime it modified the sources.
Instead, the only thing that it has to do now is to modify one configuration file. That is it all!

1) Add your new Modules (simple or compound) by creating *.ned, *.cc, *.h files

2) Add your new modules inside new directories so that the directory structure is always similar to the 
   structure of the modules of Castalia. Eg. if you create a new routing protocol, place the ned file and
   the sources inside Castalia/src/Node/L3_Network folder. If your new module is a compound one then create
   a new directory for it and add all the related simple modules inside that directory.

3) Add whenever you like (make sure that you remember where) your msg files. Usually it is more convenient
   to add them inside the directory of the modules that they use them. If more that one modules use this
   msg file then place it to the upper most directory that has directly under it all these modules folders.

4) Change to directory "Castalia/config" and modify the file Castalia.config.
   Make sure that the "ROOT" variable contains the full path of the root directory of Castalia.

5) Change to directory "Castalia/config" and modify the file makemakefiles
   This file is the Makefile uses by the script "makemake" and keeps information on how to generate
   the Makefile file at each sub-directory of the Castalia/src folder.
   Variables:
   a)OPTS ==> here you can add/modify the flags for the opp_makemake calls. Usually it has not to be modified art all.
   b)ALL_WSN_INCLUDES ==> here you MUST add every directory that either already contains an *.h (C++ header) file or
                          it will contain one after the call of opp_makemake by the Castalia/makemake script (usually 
                          the *.msg files produce extra header files). What you have to do is to add all the directories
                          with the *.h or *.msg files that you created.
   Make Targets:
   You MUST modify the "WSNetwork" target. What you have to do usually is adding a line like:
        cd $(SRCDIR)/Node && $(MAKEMAKE) $(OPTS) -n -r 
   for each new directory that you have created with source code or ned files.
   ATTENTION :  * You may need to add a -I$(SRCDIR)/xxx flag at the end of this line if you use a module from a different directory
                * You may need to edit the existing lines and add/modifiy the -I$(SRCDIR)/yyy flags appropriately.

6) You don't have to create your own omnetppconfig. It is generated automatically after the execution of "makemake".

7) You don't have to create your own nedfiles.lst. It is generated automatically after the execution of "makemake".

8) You may wish to add dependencies across directories. 
   To do that, create a file "makefrag" and place it inside the directory path that you wish to add the dependency
   among its subdirectories/modules. You can add as many "makefrag" files as the total number of the produced Makefile 
   files by the "makemake" script. The opp_makemake just injects/includes the "makefrag" inside the Makefile that it produces.
   It is a perfect way to keep some extra settings inside the Makefile files without loosing it every time they are regenerated
   by the makemake script (that uses the opp_makemake tool).

9) If there are some extra/external libraries/header files that you'd like to include inside the produced Makefile files
   then uncomment the two last lines of Castalia/config/Castalia.config and modify them as you wish.


10)Change to directory Castalia/  and type "./makemake"
   This will produce all the needed Makefile files automatically.

11)Change to directory Castalia/ (if you are not already in) and type make.

12)The Castalia simulator is built and ready for use.
   Check the Castalia/nedfiles.lst to all the ned files that have been found inside the sources.
   Check the Castalia/Bin to see the CastaliaBin executable binary with the core functionalities of the simulator.
   This binary is invoked everytime you need to run a simulation.




******************************************************************************************************************
FOR DETAILED INSTRUCTION ON HOW TO RUN YOU OWN CUSTOM SIMULATIONS CHECK THE README.TXT FILE IN CASTALIA/TEST FOLDER.
******************************************************************************************************************

