#!/usr/bin/env python3
import os
import re

print "called " + __file__
print "make table_names.csv"
os.system('sqlcmd -i table_names.sql -o table_names.csv.in -h -1 -s "," -W -m 1')

with open('table_names.csv.in') as infile, open('table_names.csv', 'w') as outfile:
    for line in infile:
        #line = re.sub('\(N rows affected\)', '', line)
        line = re.sub("\\((.*?)\\)", '', line)
        if line.rstrip() != '':
            outfile.write(line)

os.remove('table_names.csv.in')
print "Done!"

'''What does \\((.+?)\\) mean?
This regular Expression pattern will start from \\( which will match ( as it is reserved in regExp
so we need escape this character,same thing for \\) 
and (.*?) will match any character zero or more time anything moreover in () 
considered as Group which we are finding.'''