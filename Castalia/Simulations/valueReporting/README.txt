# ***************************************************************************************
# *  Copyright: Athens Information Technology (AIT),  2007, 2008			*
# *		http://www.ait.gr							*
# *             Developed at the Broadband Wireless and Sensor Networks group (B-WiSe) 	*
# *		http://www.ait.edu.gr/research/Wireless_and_Sensors/overview.asp	*
# *											*
# *  Author(s): Dimosthenis Pediaditakis						*
# *											*
# *  This file is distributed under the terms in the attached LICENSE file.		*
# *  If you do not find this file, copies can be found by writing to:			*
# *											*
# *      NICTA, Locked Bag 9013, Alexandria, NSW 1435, Australia			*
# *      Attention:  License Inquiry.							*
# **************************************************************************************/



1) Check the relative path to the nedfiles.lst found in omnetpp.ini:
     preload-ned-files = *.ned @../../nedfiles.lst
   Make sure it points to $(CASTALIA_HOME)/nedfiles.lst 

2) Check the castaliaHome_RelativePath variable found in the runConnMap script.
   Make sure it points to $(CASTALIA_HOME)

3) Run:   ./runValueReporting

4) Check the  file: Castalia-Primary-Output.txt for the generated statistics


Extra helpful information:
After the simulation ends, and given that you have the following settings in your omnetpp.ini
	SN.node[*].nodeApplication.printDebugInfo = true
	SN.node[3].nodeApplication.isSink = true  (<--- here you can any node of your preference as the Sink)
you can collect two kinds if incormation.

A. Routing tree information
   Run the following command withing the directory that Castalia-Primary-Output.txt resides
      cat Castalia-Debug.txt | grep Level

B. See the values that the sink has received
   Run the following command withing the directory that Castalia-Primary-Output.txt resides
      cat Castalia-Debug.txt | grep Sink | grep "received from"