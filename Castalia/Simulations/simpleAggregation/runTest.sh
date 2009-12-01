#!/bin/bash

rm Castalia-Trace.txt

../../CastaliaBin

rm -rf output

mkdir output

cat Castalia-Trace.txt | grep "received from" > output/SinkReceived.txt

cat Castalia-Trace.txt | grep "Aggregated Value" > output/SinkAggrValues.txt

cat Castalia-Trace.txt | grep "Sensed" | grep "App_8" > output/Node8Sensed.txt

