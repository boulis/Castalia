# ***************************************************************************************
# *  Copyright: National ICT Australia,  2007, 2008, 2009				*
# *  Developed at the Networks and Pervasive Computing program				*
# *  Author(s): Athanassios Boulis, Dimosthenis Pediaditakis				*
# *  This file is distributed under the terms in the attached LICENSE file.		*
# *  If you do not find this file, copies can be found by writing to:			*
# *											*
# *      NICTA, Locked Bag 9013, Alexandria, NSW 1435, Australia			*
# *      Attention:  License Inquiry.							*
# *											*
# **************************************************************************************/



#!/bin/bash

if [ -z $1 ]; then
	echo -e "Error:\n\tThe user has to provide the first argument for the script!\n\tScript usage: ./create_new_application.sh NEW_APP_NAME_HERE"
	exit 1
fi


if [ -d ../$1 ]; then
	echo -e "Error:\n\tA directory with name \"$1\" already exists inside Castalia's application modules directory \n"
	exit 1
elif [ -a ../$1 ]; then
	echo -e "Error:\n\tA file with name \"$1\" already exists inside Castalia's application modules directory \n"
	exit 1
else
	if [ -w .. ]; then
		mkdir ../$1
	else
		echo -e "Error:\n\tYou need to have write perismissions for the Castalia's application modules directory\n"
		exit 1
	fi
fi

ned_filename="$1_ApplicationModule.ned"
cat template_ApplicationModule.ned | sed -e 's/template_ApplicationModule/'$1'_ApplicationModule/g' > ../$1/$ned_filename

if test $? == 0; then
	echo -e "--> $ned_filename generated"
else
	echo -e "Error:\n\tWhile trying to produce $ned_filename\n"
	exit 1
fi


msg_filename="$1_DataPacket.msg"
cat template_DataPacket.msg | sed -e 's/template_DataPacket/'$1'_DataPacket/g' > ../$1/$msg_filename

if test $? == 0; then
	echo -e "--> $msg_filename generated"
else
	echo -e "Error:\n\tWhile trying to produce $msg_filename\n"
	exit 1
fi


header_filename="$1_ApplicationModule.h"
upper_case_arg=`echo $1 | tr '[a-z]' '[A-Z]'`
cat template_ApplicationModule.h | sed -e 's/_TEMPLATE_APPLICATIONMODULE_H_/_'$upper_case_arg'_APPLICATIONMODULE_H_/g' > ../$1/$header_filename
sed -e 's/template_DataPacket_m/'$1'_DataPacket_m/g' ../$1/$header_filename > ../$1/$header_filename.tmp
mv ../$1/$header_filename.tmp ../$1/$header_filename
sed -e 's/template_ApplicationModule/'$1'_ApplicationModule/g' ../$1/$header_filename > ../$1/$header_filename.tmp
mv ../$1/$header_filename.tmp ../$1/$header_filename


if test $? == 0; then
	echo -e "--> $header_filename generated"
else
	echo -e "Error:\n\tWhile trying to produce $header_filename\n"
	exit 1
fi


cc_filename="$1_ApplicationModule.cc"
cat template_ApplicationModule.cc | sed -e 's/template_ApplicationModule/'$1'_ApplicationModule/g' > ../$1/$cc_filename
sed -e 's/template_DataPacket/'$1'_DataPacket/g' ../$1/$cc_filename > ../$1/$cc_filename.tmp
mv ../$1/$cc_filename.tmp ../$1/$cc_filename
sed -e 's/template_radioControlImpl.cc.inc/'$1'_radioControlImpl.cc.inc/g' ../$1/$cc_filename > ../$1/$cc_filename.tmp
mv ../$1/$cc_filename.tmp ../$1/$cc_filename
sed -e 's/template_tunableMacControlImpl.cc.inc/'$1'_tunableMacControlImpl.cc.inc/g' ../$1/$cc_filename > ../$1/$cc_filename.tmp
mv ../$1/$cc_filename.tmp ../$1/$cc_filename


if test $? == 0; then
	echo -e "--> $cc_filename generated"
else
	echo -e "Error:\n\tWhile trying to produce $cc_filename\n"
	exit 1
fi



cc_filename_MacInclude="$1_tunableMacControlImpl.cc.inc"
cat template_tunableMacControlImpl.cc.inc  | sed -e 's/template_ApplicationModule/'$1'_ApplicationModule/g' > ../$1/$cc_filename_MacInclude
if test $? == 0; then
	echo -e "--> $cc_filename_MacInclude generated"
else
	echo -e "Error:\n\tWhile trying to produce $cc_filename_MacInclude\n"
	exit 1
fi


cc_filename_RadioInclude="$1_radioControlImpl.cc.inc"
cat template_radioControlImpl.cc.inc | sed -e 's/template_ApplicationModule/'$1'_ApplicationModule/g' > ../$1/$cc_filename_RadioInclude
if test $? == 0; then
	echo -e "--> $cc_filename_RadioInclude generated"
else
	echo -e "Error:\n\tWhile trying to produce $cc_filename_RadioInclude\n"
	exit 1
fi



echo -e "\nThe files for the new application module \"$1\" have been generated successsfully.\n"
