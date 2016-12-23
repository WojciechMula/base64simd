# usage: python script/knl.py results/knights-landing*.txt

import sys
from collections import OrderedDict

def load(path):
    D = OrderedDict()

    with open(path) as f:
        for line in f:
            pos = line.find('...')
            if pos < 0:
                continue

            name = line[:pos].strip()
            time = float(line[pos + 3:].split()[0])

            if name in D:
                D[name] = min(D[name], time)
            else:
                D[name] = time

    return D


def compress_SSE_and_AVX2(data):
    SSE_name = None
    SSE_time = None
    AVX2_name = None
    AVX2_time = None
    for name, time in data.iteritems():
        if name.startswith('SSE '):
            if SSE_time is None or SSE_time < time:
                SSE_name = name
                SSE_time = time
        elif name.startswith('AVX2 '):
            if AVX2_time is None or AVX2_time < time:
                AVX2_name = name
                AVX2_time = time
            
    D = OrderedDict()
    SSE_added = False
    AVX2_added = False
    for name, time in data.iteritems():
        if name.startswith('SSE '):
            if not SSE_added:
                D[SSE_name] = SSE_time
                SSE_added = True

        elif name.startswith('AVX2 '):
            if not AVX2_added:
                D[AVX2_name] = AVX2_time
                AVX2_added = True

        else:
            D[name] = time

    return D


def print_data(data):
    base = None
    for name, time in data.iteritems():
        if base is None:
            print '%-63s %0.4f' % (name, time)
            base = time
        else:
            speedup = base/time
            bar = '#' * int(3*speedup)
            print '%-63s %0.4f (speed-up %.2f) %s' % (name, time, speedup, bar)


def main():
    for path in sys.argv[1:]:
        if len(sys.argv) > 2:
            print path

        D = load(path)
        D = compress_SSE_and_AVX2(D)
        print_data(D)


if __name__ == '__main__':
    main()
