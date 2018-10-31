# -*- coding: utf-8 -*-
from collections import OrderedDict
from table import Table
from utils import unicode_bar
from benchmark_parser import parse, update_speedup, get_maximum_speedup
import sys
import codecs

sys.stdout = codecs.getwriter('utf-8')(sys.stdout)


def main():
    with open(sys.argv[1]) as f:
        data = load(f)
        update_speedup(data)
        global_max_speedup = get_maximum_speedup(data)
        render(data, global_max_speedup)


def load(file):
    tmp = parse(file)

    def update_measurements(target, source):
        assert set(target.keys()) == set(source.keys())

        for key, measurement in source.iteritems():
            target[key].min(measurement)

    data = None;
    key = None
    for item in tmp:
        if type(item) is str:
            if key is None:
                key = item
            else:
                assert key == item
        else:
            if data is None:
                data = item
            else:
                update_measurements(data, item)

    assert data is not None
    return data


def render(measurements, global_max_speedup):

    table = Table()
    table.add_header(["procedure", "best",     "avg.",      "speedup", ""])
    table.add_header(["",          "[cycles]", "[cycles]",  "", ""])

    for name, measurement in measurements.iteritems():

        barlen = 50 * measurement.speedup/global_max_speedup

        table.add_row([
            name,
            '%0.3f' % measurement.best,
            '%0.3f' % measurement.avg,
            '%0.2f' % measurement.speedup,
            '``%s``' % unicode_bar(barlen)
        ])

    print unicode(table)


if __name__ == '__main__':
    main()
