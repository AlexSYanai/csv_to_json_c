CSV to JSON
===========

A small program to read in a csv and print out json. The csv parsing is inspired by, and based off of code found in K&P's [The Practice of Programming](https://en.wikipedia.org/wiki/The_Practice_of_Programming).

####Compile:

```
gcc -o convert convert.c
```
####Run:
convert reads in the csv line by via line via the terminal, so the file needs to be piped in.

```
cat test.csv | ./convert
```