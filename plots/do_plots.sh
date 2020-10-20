#!/bin/bash

# $1 prefix for plot tex files
# $2 path with data

if [ $# -ne 2 ]; then
	echo "usage"
	exit 1
fi

set -x
set -e

echo -n '' > data.tex
./generate_plot.py -o=$1.tex -s=$2/serial "\\ourWaitFreeAlgo{}":$2/nowa Fibril:$2/fibril "Cilk Plus":$2/cilkplus TBB:$2/tbb -r -l=4
rubber --pdf $1.tex &

./generate_plot.py -o="$1-queues.tex" -s=$2/serial "\\ourWaitFreeAlgo{} (CL-queue)":$2/nowa "\\ourWaitFreeAlgo{} (THE-queue)":$2/nowa-the-queue Fibril:$2/fibril -b=cholesky,fib -c=2 -l=4
rubber --pdf "$1-queues.tex" &

./generate_plot.py -o="$1-queues-4.tex" -s=$2/serial "\\ourWaitFreeAlgo{} (CL-queue)":$2/nowa "\\ourWaitFreeAlgo{} (THE-queue)":$2/nowa-the-queue Fibril:$2/fibril -b=cholesky,fib,nqueens,matmul -c=2 -l=4
rubber --pdf "$1-queues-4.tex" &

./generate_plot.py -o="$1-with-madvise.tex" -s=$2/serial "\\ourWaitFreeAlgo{} w/o \\texttt{madvise()}":$2/nowa "\\ourWaitFreeAlgo{} w/ madvise":$2/nowa-madvise "Cilk Plus":$2/cilkplus -b=cholesky,lu -c=2 -l=4
rubber --pdf "$1-with-madvise.tex" &

./generate_plot.py -o="$1-with-madvise-4.tex" -s=$2/serial "\\ourWaitFreeAlgo{} w/o \\texttt{madvise()}":$2/nowa "\\ourWaitFreeAlgo{} w/ \\texttt{madvise()}":$2/nowa-madvise "Cilk Plus":$2/cilkplus -b=cholesky,lu,matmul,nqueens -c=2 -l=4
rubber --pdf "$1-with-madvise-4.tex" &

./generate_plot.py -o="$1-with-madvise-6.tex" -s=$2/serial "\\ourWaitFreeAlgo{} w/o \\texttt{madvise()}":$2/nowa "\\ourWaitFreeAlgo{} w/ \\texttt{madvise()}":$2/nowa-madvise "Cilk Plus":$2/cilkplus -b=cholesky,lu,matmul,nqueens,integrate,rectmul -c=2 -l=4
rubber --pdf "$1-with-madvise-6.tex" &

./generate_plot.py -o="$1-with-madvise-8.tex" -s=$2/serial "\\ourWaitFreeAlgo{} w/o \\texttt{madvise()}":$2/nowa "\\ourWaitFreeAlgo{} w/ \\texttt{madvise()}":$2/nowa-madvise "Cilk Plus":$2/cilkplus -b=cholesky,lu,heat,fib,matmul,nqueens,integrate,rectmul -c=2 -l=4
rubber --pdf "$1-with-madvise-8.tex" &

./generate_plot.py -o="$1-with-madvise-all.tex" -s=$2/serial "\\ourWaitFreeAlgo{} w/o \\texttt{madvise()}":$2/nowa "\\ourWaitFreeAlgo{} w/ \\texttt{madvise()}":$2/nowa-madvise "Cilk Plus":$2/cilkplus -c=2 -l=4
rubber --pdf "$1-with-madvise-all.tex" &

./generate_plot.py -o="$1-teaser.tex" -s=$2/serial "\\ourWaitFreeAlgo{}":$2/nowa Fibril:$2/fibril "Cilk Plus":$2/cilkplus TBB:$2/tbb -b=nqueens -l=4
rubber --pdf "$1-teaser.tex" &

./generate_plot.py -o="$1-queues-appendix.tex" -s=$2/serial "\\ourWaitFreeAlgo{}":$2/nowa "\\ourWaitFreeAlgo{} THE":$2/nowa-the-queue Fibril:$2/fibril -r -l=4
rubber --pdf "$1-queues-appendix.tex" &

#./generate_plot.py -o="$1-with-madvise-appendix.tex" -s=$2/serial "\\ourWaitFreeAlgo{}":$2/nowa "\\ourWaitFreeAlgo{} w/ \\texttt{madvise()}":$2/nowa-madvise "Cilk Plus":$2/cilkplus -r -l=4
#rubber --pdf "$1-with-madvise-appendix.tex" &

./generate_plot.py -o="$1-fibril-with-madvise-appendix.tex" -s=$2/serial "Fibril":$2/fibril "Fibril w/ \\texttt{madvise()}":$2/fibril-madvise "Cilk Plus":$2/cilkplus -r -l=4
rm "$1-fibril-with-madvise-appendix.tex"
#rubber --pdf "$1-fibril-with-madvise-appendix.tex" &

./generate_plot.py -o="$1-with-madvise-appendix.tex" -s=$2/serial "\\ourWaitFreeAlgo{}":$2/nowa Fibril:$2/fibril "Cilk Plus":$2/cilkplus "\\ourWaitFreeAlgo{} w/ \\texttt{madvise()}":$2/nowa-madvise "Fibril w/ \\texttt{madvise()}":$2/fibril-madvise -r -l=4
rubber --pdf "$1-with-madvise-appendix.tex" &

# ./generate_plot.py -o="$1-openmp-4.tex" -s=$2/serial "\\ourWaitFreeAlgo{}":$2/nowa Fibril:$2/fibril "Cilk Plus":$2/cilkplus TBB:$2/tbb "OpenMP (libgomp)":$2/openmp -c=2 -l=5 -l -u -b=fib,heat,knapsack,strassen
# rubber --pdf "$1-openmp-4.tex" &

# ./generate_plot.py -o="$1-openmp-6.tex" -s=$2/serial "\\ourWaitFreeAlgo{}":$2/nowa Fibril:$2/fibril "Cilk Plus":$2/cilkplus TBB:$2/tbb "OpenMP (libgomp)":$2/openmp -c=2 -l=5 -l -u -b=fft,fib,heat,knapsack,quicksort,strassen
# rubber --pdf "$1-openmp-6.tex" &

./generate_plot.py -o="$1-openmp-full.tex" -s=$2/serial "\\ourWaitFreeAlgo{}":$2/nowa TBB:$2/tbb "OpenMP (libgomp)":$2/openmp "OpenMP (libomp)":$2/openmp-gcc-libomp -l=5 -l -u -r
rubber --pdf "$1-openmp-full.tex" &

./generate_plot.py -o="$1-openmp-full-w-tied.tex" -s=$2/serial "\\ourWaitFreeAlgo{}":$2/nowa TBB:$2/tbb "\\libgomp{}":$2/openmp "\\libomp{} (untied)":$2/openmp-gcc-libomp "\\libomp{} (tied)":$2/openmp-gcc-libomp-tied-tasks -l=5 -l -u -r
rubber --pdf "$1-openmp-full-w-tied.tex" &

wait -f
