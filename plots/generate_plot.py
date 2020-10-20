#!/usr/bin/env python3

import sys
import csv
import statistics as stats

default_benchmarks = [
    'cholesky', 'fft', 'fib', 'heat', 'integrate', 'knapsack', 'lu', 'matmul',
    'nqueens', 'quicksort', 'rectmul', 'strassen'
]

# FIXME fix xtick for artifact / xmin / title
#title style={\n\
#at={(0.5,0.9)},\n\
#font=\small,\n\
#},\n\
#legend style={\n\
#    at={(-0.00,1.15)},\n\
#    anchor=south west,\n\
#    font=\small,\n\
#    draw=none,\n\
#},\n\
#height=4.5cm,\n\
#width=7cm,\n\
plotTopGeneric = "\
\documentclass{standalone}\n\
\input{common.tex}\n\
\n\
\\begin{document}\n\
\\begin{tikzpicture}\n\
\\begin{groupplot}[\n\
        group style={\n\
                group name=%s,\n\
                group size=%d by %d,\n\
                vertical sep=0.65cm,\n\
                horizontal sep=%.2fcm,\n\
                xticklabels at=edge bottom,\n\
            xlabels at=edge bottom,\n\
            ylabels at=edge left,\n\
        },\n\
        cycle list name=vibrant,\n\
        title style={\n\
            at={(0.5,0.9)},\n\
            font=\small,\n\
        },\n\
        xmin=0,\n\
        xtick pos=bottom,\n\
        ytick pos=left,\n\
        xminorticks=false,\n\
        yminorticks=false,\n\
        ymajorgrids=true,\n\
        height=4.0cm,\n\
        width=6.5cm,\n\
        enlarge x limits={\n\
            auto,\n\
            value=0.04,\n\
        },\n\
        legend columns=%d,\n\
        legend style={\n\
            at={(%s,1.2)},\n\
            anchor=south,\n\
            font=\small,\n\
            draw=none,\n\
        },\n\
%s\
]\n"

plotTopLinear = "\
        xtick={64,128,192,256},\n\
        ymin=0,\n\
"

plotTopLogarithmic = "\
        xmode=log,\n\
        ymode=log,\n\
        xtick={64,128,256},\n\
        xticklabels={64,128,256},\n\
"

#at={(1.05,1.15)},\n\
#at={(0.5,1.15)},\n\
#anchor=south,\n\

plotBottom = "\
\\end{groupplot}\n\
\n\
%s\n\
%s\n\
\n\
\\end{tikzpicture}\n\
\\end{document}"

plotPlotMean = "\
\\addplot+[sharp plot, error bars/.cd, y dir=both, y explicit]\n\
    table [x=cores, y=speedup, y error=err]{\n\
    cores\tspeedup\terr"

plotPlotMedian = "\
\\addplot+[sharp plot, error bars/.cd, y dir=both, y explicit]\n\
    table [x=cores, y=speedup, y error plus=err+, y error minus=err-]{\n\
    cores\tspeedup\terr-\terr+"


def usage():
    print("usage:")
    print("./generate_plot.py OUTFILE SERIAL [NAME:PATH]...")
    print("FIXME")
    print("\tOUTFILE\t\tfilename of generated plot tex file")
    print("\tSERIAL\t\tpath to the .csv files with the serial times")
    print(
        "\tNAME:PATH\ttuples of the name in the plots and the path to the .csv files"
    )


def fixName(name):
    n = ""
    for c in name:
        if c == '_':
            n += '\\'
        n += c
    return n


def do_dataref_replacement(name):
    dataref_replacements = {
        '\\ourWaitFreeAlgo{}' : 'nowa',
        '\\libomp{}' : 'libomp',
        '\\libgomp{}' : 'libgomp',
    }
    for original, substitute in dataref_replacements.items():
        name = name.replace(original, substitute)

    name = name.replace('/', '')
    return name


def main(args):
    if len(args) < 2:
        usage()
        sys.exit(1)

    keys = {
        's': lambda v: locals().__setitem__('serialPath', v),
        'o': lambda v: locals().__setitem__('output', v)
    }
    rts = []
    benchmarks = default_benchmarks
    columns = 0
    plot_format = (2, 6)
    generateRelativeNumbers = False
    legendColumns = 0
    logscale = False
    uniformPlotPoints = False
    for arg in args[1:]:
        if arg[0] == '-':
            if '=' in arg:
                key, val = arg[1:].split('=', 1)
                if key == 'o':
                    output = val
                    plot_name = output.split('.')[0]
                elif key == 's':
                    serialPath = val
                elif key == 'l':
                    try:
                        legendColumns = int(val)
                    except:
                        usage()
                        sys.exit(1)
                elif key == 'c':
                    try:
                        columns = int(val)
                    except:
                        usage()
                        sys.exit(1)
                elif key == 'b':
                    benchmarks = val.split(',')
                else:
                    usage()
                    sys.exit(1)
            else:
                key = arg[1:]
                if key == 'h':
                    usage()
                    sys.exit(0)
                elif key == 'r':
                    generateRelativeNumbers = True
                elif key == 'l':
                    logscale = True
                elif key == 'u':
                    uniformPlotPoints = True
                else:
                    usage()
                    sys.exit(1)
        elif ':' in arg:
            rts += [arg.split(':', 1)]
        else:
            usage()
            sys.exit(1)
    runtimes = dict(rts)
    if columns == 0:
        if len(benchmarks) <= 6:
            columns = 1
        else:
            columns = 2
    plot_format = (columns, int(len(benchmarks) / columns))
    if (len(benchmarks) % columns) != 0:
        print('WARNING: Not all columns of plots are filled!')
    if legendColumns == 0:
        legendColumns = 2 * columns

    data = dict()
    for r in runtimes:
        data[r] = dict()
        for b in benchmarks:
            data[r][b] = dict()
            with open(f"{runtimes[r]}/{b}.csv", newline='') as csvfile:
                reader = csv.DictReader(csvfile, dialect='excel-tab')
                for row in reader:
                    if data[r][b].get(row['cores']) == None:
                        data[r][b][row['cores']] = []
                    data[r][b][row['cores']] += [row]

    serial = dict()
    for b in benchmarks:
        serial[b] = []
        with open(f"{serialPath}/{b}.csv", newline='') as csvfile:
            reader = csv.DictReader(csvfile, dialect='excel-tab')
            for row in reader:
                serial[b] += [row]

    serialTimeMedi = dict()
    stdevSerialTimeMedi = dict()
    serialTimeMean = dict()
    stdevSerialTimeMean = dict()
    for b in benchmarks:
        floatValues = list(map(lambda a: float(a['time']), serial[b]))

        serialTimeMedi[b] = stats.median(floatValues)
        stdevSerialTimeMedi[b] = stats.stdev(floatValues, serialTimeMedi[b])

        serialTimeMean[b] = stats.mean(floatValues)
        stdevSerialTimeMean[b] = stats.stdev(floatValues, serialTimeMean[b])

    speedup = dict()
    for r in runtimes:
        speedup[r] = dict()
        for b in benchmarks:
            speedup[r][b] = []

            for core_count in list(data[r][b]):
                speedups = list(
                    map(lambda a: serialTimeMean[b] / float(a['time']),
                        data[r][b][core_count]))
                # N.B. using geometric mean here.
                mean_speedup = stats.geometric_mean(speedups)
                standard_deviation = stats.stdev(speedups, mean_speedup)
                speedup[r][b] += [
                    (core_count, mean_speedup, standard_deviation)
                ]

            # Sort the speedup dict by core count.
            # This is usually not required, as the core count is already sorted.
            speedup[r][b].sort(key=lambda a: int(a[0]))

    if generateRelativeNumbers:
        r_ref = rts[0][0]

        if False:
            mc = str(
                max(map(lambda x: int(x), data[r_ref][benchmarks[0]].keys())))
            for r in runtimes:
                for b in benchmarks:
                    d = data[r][b][mc]
                    rss = sum(map(lambda x: int(x['rss']), d))
                    d.sort(key=lambda x: int(x['run']))
                    maxrss = d[len(d) - 1]['maxrss']
                    print(f'{r} {b} {rss} {maxrss}')
                    pass

        max_cores = len(speedup[r_ref][benchmarks[0]]) - 1
        rrp = do_dataref_replacement(r_ref)
        output_relative = 'data.tex'
        with open(output_relative, mode='at', newline='') as outfile:
            max_runs = len(serial[benchmarks[0]])
            print(f'\\drefset{{/meta/maxruns}}{{{max_runs}}}', file=outfile)
            for r in runtimes:
                rp = do_dataref_replacement(r)
                mc = str(
                    max(
                        map(lambda x: int(x),
                            data[r_ref][benchmarks[0]].keys())))
                for b in benchmarks:
                    d = data[r][b].get(mc, None)
                    if d is None:
                        print(f"Warning: No data available for {r}/{b}/{mc} cores", file=sys.stderr)
                        continue
                    rss = round(
                        int(sum(map(lambda x: int(x['rss']), d))) / 1024, 0)
                    d.sort(key=lambda x: int(x['run']))
                    maxrss = round(int(d[len(d) - 1]['maxrss']) / 1024, 0)

                    entry = speedup[r][b][len(speedup[r][b]) - 1]
                    times = list(map(lambda a: float(a['time']), data[r][b][mc]))

                    spdupMean = round(entry[1], 3)
                    spdupStdev = round(entry[2], 3)
                    tMean = round(stats.mean(times), 3)
                    tStdev = round(stats.stdev(times, tMean), 3)

                    print(
                        f'\\drefset[unit=\\mebi\\byte]{{/mem/rss/{rp.lower()}/{b}}}{{{rss}}}',
                        file=outfile)
                    print(
                        f'\\drefset[unit=\\mebi\\byte]{{/mem/maxrss/{rp.lower()}/{b}}}{{{maxrss}}}',
                        file=outfile)
                    print(
                        f'\\drefset[unit=\\second]{{/perf/abs/time/mean/{rp.lower()}/{b}}}{{{tMean}}}',
                        file=outfile)
                    print(
                        f'\\drefset[unit=\\second]{{/perf/abs/time/stdev/{rp.lower()}/{b}}}{{{tStdev}}}',
                        file=outfile)
                    print(
                        f'\\drefset{{/perf/abs/speedup/mean/{rp.lower()}/{b}}}{{{spdupMean}}}',
                        file=outfile)
                    print(
                        f'\\drefset{{/perf/abs/speedup/stdev/{rp.lower()}/{b}}}{{{spdupStdev}}}',
                        file=outfile)

                if speedup.get(r) == None:
                    print(f"Ignoring {r} as there are no speedup values", file=sys.stderr)

                if r == r_ref:
                    continue
                divs = dict()
                best = 0
                worst = 10e10
                for b in benchmarks:
                    # N.B. We only consider max_cores for the
                    # best/worst/avg values (with or without
                    # knapsack).
                    div = (speedup[r_ref][b][max_cores][1] /
                           speedup[r][b][len(speedup[r][b]) - 1][1])
                    if div > best:
                        best = div
                        best_name = b
                    if div < worst:
                        worst = div
                        worst_name = b
                    divs[b] = div

                divsWithOutKnapsack = dict(divs)
                divsWithOutKnapsack.pop('knapsack')
                divsWithOutKnapsackSorted = sorted(divsWithOutKnapsack.items(), key=lambda item: item[1])
                worst_wo = round(divsWithOutKnapsackSorted[0][1], 2)
                best_wo = round(divsWithOutKnapsackSorted[-1][1], 2)

                best = round(best, 2)
                worst = round(worst, 2)
                avg = round(stats.geometric_mean(divs.values()), 2)
                avg_wo = round(stats.geometric_mean(divsWithOutKnapsack.values()), 2)

                print(
                    f'\\drefset[unit=\\times]{{/perf/rel/{rrp.lower()} {rp.lower()}/max}}{{{best}}}',
                    file=outfile)
                print(
                    f'\\drefset[unit=\\times]{{/perf/rel/{rrp.lower()} {rp.lower()}/min}}{{{worst}}}',
                    file=outfile)
                print(
                    f'\\drefset[unit=\\times]{{/perf/rel/{rrp.lower()} {rp.lower()}/max wo}}{{{best_wo}}}',
                    file=outfile)
                print(
                    f'\\drefset[unit=\\times]{{/perf/rel/{rrp.lower()} {rp.lower()}/min wo}}{{{worst_wo}}}',
                    file=outfile)
                print(
                    f'\\drefset[unit=\\times]{{/perf/rel/{rrp.lower()} {rp.lower()}/avg}}{{{avg}}}',
                    file=outfile)
                print(
                    f'\\drefset[unit=\\times]{{/perf/rel/{rrp.lower()} {rp.lower()}/avg wo}}{{{avg_wo}}}',
                    file=outfile)
                for b in divs:
                    div = round(divs[b], 2)
                    print(
                        f'\\drefset[unit=\\times]{{/perf/rel/{rrp.lower()} {rp.lower()}/{b}}}{{{div}}}',
                        file=outfile)

    if uniformPlotPoints:
        for b in benchmarks:
            init = True
            for r in runtimes:
                if init:
                    init = False
                    cores = set(data[r][b].keys())
                else:
                    cores.intersection_update(set(data[r][b].keys()))
            # We assume all benchmarks ran with the same numbers of cores for
            # one given runtime, so we don't need to check all benchmarks.
            break

    with open(output, mode='wt', newline='') as outfile:
        if (plot_format[0] % 2) == 0:
            legendPos = '1.05'
        else:
            legendPos = '0.5'
        if logscale:
            verticalSep = 1.2
            ylabelOffset = 0.9
            plotTopType = plotTopLogarithmic
        else:
            verticalSep = 0.7
            ylabelOffset = 0.6
            plotTopType = plotTopLinear
        print(plotTopGeneric % (plot_name, plot_format[0], plot_format[1],
                         verticalSep, legendColumns, legendPos,plotTopType),
              file=outfile)
        printLegend = True
        for b in benchmarks:
            if b in default_benchmarks:
                serialTime = serialTimeMean[b]
                stdev = stdevSerialTimeMean[b]

                serialTime = "{:.3f}".format(serialTime)
                stdev = "{:.3f}".format(stdev)

                print(f"% stdev: {stdev}", file=outfile)
                print(f"\\nextgroupplot[title={{\\{b}{{}} (${{\\bar{{T}}}}_{{S}} = \\SI[separate-uncertainty, multi-part-units=single]{{{serialTime}+-{stdev}}}{{\\second}}$) }}]",
                      file=outfile)
            else:
                print(f"\\nextgroupplot[title={fixName(b)}]", file=outfile)
            for r in runtimes:
                print(plotPlotMean, file=outfile)
                for entry in speedup[r][b]:
                    if uniformPlotPoints and not cores.__contains__(entry[0]):
                            continue
                    core_count = entry[0]
                    mean_speedup = entry[1]
                    standard_deviation = entry[2]
                    print(f"    {core_count}\t{mean_speedup}\t{standard_deviation}", file=outfile)
                print("};", file=outfile)
            if printLegend:
                printLegend = False
                for r in runtimes:
                    print(f"\\addlegendentry{{{fixName(r)}}}", file=outfile)
            print("", file=outfile)
        if len(benchmarks) % 2 == 1 and not len(benchmarks) == 1:
            print('\\nextgroupplot[group/empty plot]\n', file=outfile)
        xlabel = '\\draw ($(%s c1r%d.south west)!0.5!(%s c%dr%d.south east) + (0,-0.4)$) node[anchor=north] (xlabel) {\\textbf{Threads}};' % (
            plot_name, plot_format[1], plot_name, plot_format[0],
            plot_format[1])
        ylabel = '\\draw ($(%s c1r1.north west)!0.5!(%s c1r%d.south west) + (-%.1f,0)$) node[anchor=south,rotate=90] (ylabel) {\\textbf{Speedup}};' % (
            plot_name, plot_name, plot_format[1], ylabelOffset)
        print(plotBottom % (xlabel, ylabel), file=outfile)


if __name__ == "__main__":
    main(sys.argv)
