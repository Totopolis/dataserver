#!/usr/bin/env python3
import os
import re

print "called " + __file__
print "make schema_names.csv"
os.system('sqlcmd -i schema_names.sql -o schema_names.csv.in -h -1 -s "," -W -m 1')

with open('schema_names.csv.in') as infile, open('schema_names.csv', 'w') as outfile:
    for line in infile:
        #line = re.sub('\(N rows affected\)', '', line)
        line = re.sub("\\((.*?)\\)", '', line)
        if line.rstrip() != '':
            outfile.write(line)

os.remove('schema_names.csv.in')
print "Done!"

'''What does \\((.+?)\\) mean?
This regular Expression pattern will start from \\( which will match ( as it is reserved in regExp
so we need escape this character,same thing for \\) 
and (.*?) will match any character zero or more time anything moreover in () 
considered as Group which we are finding.'''