#!/bin/sh

# cat: read the file. here for linearity of the data flow and readibility.
# sed:
#  1. prepare it for the next point.
#     - remove the '\"' for the next step
#     - remove the strings, to make sure no /* or */ is wrongly interpreted as comment.
#     - remove the '-'s
#     - /* -> -o[pen]
#     - */ -> -c[lose]
#  2. removes the comments, assuming perfectly paired /* and */
#     - store everything in memory
#     - if it's the last line remove everything between a -o and a -c
#  3. remove indentation from empty lines, for the next point.
# cat: join any group of empty lines into one
# wc: count the remaining lines
cat linenoise.c | sed 's/\\"//g;s/"[^"]*"//g;s/-/=/g;s|/\*|-o|g;s|\*/|-c|g' | sed 'H;$!d;x;s/-o[^-]*-c//g' | sed 's/^[ \t]*$//' | cat -s | wc -l
