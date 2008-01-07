#!/bin/sh

i=0
ls models/*.cmod |
  cut '-d.' -f1 |
  cut '-d/' -f2 |
  sort | uniq |
  while read x; do
    cat models/${x}.*cmod |
      grep '^##' |
      sed -e "s/^##//" \
	  -e "s/%parent%/Altair/" \
	  -e "s/%name%/${x}/" \
	  -e "s/%orbitascendingnode%/0/" \
	  -e "s/%orbiteccentricity%/0/" \
	  -e "s/%orbitinclination%/0/" \
	  -e "s/%orbitmeananomaly%/$(printf "0.%06d" ${i})/" \
	  -e "s/%orbitperiod%/1/" \
	  -e "s/%orbitradius%/10/" \
	  -e "s/%obliquity%/0/" \
	  -e "s/%equatorascendingnode%/0/"
    (( i++ ))
  done >shipyard.ssc
