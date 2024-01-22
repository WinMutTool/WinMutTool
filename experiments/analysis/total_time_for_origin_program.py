#!/usr/bin/env python3

import sys

if __name__=='__main__':
    filename = sys.argv[1]
    total = 0
    with open(filename, errors='ignore') as f:
        for l in f:
            if l.startswith('+') or l.startswith('-'):
                continue
            try:
                total += int(l.split(':')[0].strip())
            except:
                print(l)
    print(total / 1000000)
