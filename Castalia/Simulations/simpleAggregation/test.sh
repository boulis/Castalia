#!/bin/bash

rm Castalia-Debug.txt

../../bin/CastaliaBin

rm -rf output

mkdir output

cat Castalia-Debug.txt | grep "received from" > output/SinkReceived.txt

cat Castalia-Debug.txt | grep "Aggregated Value" > output/SinkAggrValues.txt

cat Castalia-Debug.txt | grep "Sensed" | grep "App_8" > output/Node8Sensed.txt

