import sys
import re

mb_height = 0
mb_width = 0
nb_components = 0
nb_blocks = 0

with open(sys.argv[1]) as f:
    i = 0
    for line in f:
        splits = line.split(' ')
        vals = [int(e) for e in splits]
        if i == 0:
            mb_height = vals[0]
            mb_width = vals[1]
            nb_components = vals[2]
            nb_blocks = vals[3]
            sys.stdout.write(line)
        else:
            sys.stdout.write("%d %d %d %d %d\n" % (mb_height-vals[0]-1, mb_width-vals[1]-1, vals[2], vals[3], vals[4]))
        i += 1
