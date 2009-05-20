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
     preload-ned-files = *.ned @../../nedfiles.lst
   Make sure it points to $(CASTALIA_HOME)/nedfiles.lst 

2) Check the castaliaHome_RelativePath variable found in the runConnMap script.
   Make sure it points to $(CASTALIA_HOME)

3) Run:   ./runSimpleAggregation

4) Check the  file: Castalia-Primary-Output.txt for the generated statistics
