#!/bin/sh
echo quit | ./yastg > out
grep -a 'Defined' out | awk '{print $5,$6}' > all
grep -a 'Growing civ Terran Federation' out | awk '{print $11,$12}' > terran
grep -a 'Growing civ Foociv' out | awk '{print $10,$11}' > foo
grep -a 'Growing civ Kingdom of Grazny' out | awk '{print $12,$13}' > grazny
echo 'set terminal png; set output "map.png"; plot "all", "terran", "foo", "grazny"' | gnuplot
display map.png
