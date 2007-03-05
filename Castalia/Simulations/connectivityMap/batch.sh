#! /bin/sh

for ((i=1; $i<31; i++)); do
   ../../bin/CastaliaBin -f omnetpp.ini -r $i  >> FullDetailedResults.txt
done