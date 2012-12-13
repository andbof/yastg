#!/bin/sh
echo quit | ./yastg > /tmp/out
awk '/^Created /{print $NF}' < /tmp/out | awk -Fx '{print $1,$2}' > /tmp/all
awk '/^Growing civ Terran Federation/{print $NF}' < /tmp/out | awk -Fx '{print $1,$2}' > /tmp/terran
awk '/^Growing civ Foociv/{print $NF}' < /tmp/out | awk -Fx '{print $1,$2}' > /tmp/foo
awk '/^Growing civ Kingdom of Grazny/{print $NF}' < /tmp/out | awk -Fx '{print $1,$2}' > /tmp/grazny
echo 'set terminal png; set output "/tmp/map.png"; ' \
	'plot "/tmp/all", "/tmp/terran", "/tmp/foo", "/tmp/grazny"' \
	| gnuplot
display /tmp/map.png
