#!/usr/bin/env bash



runtimes=( "nowa" "nowa-madvise" "nowa-the-queue" "fibril" "fibril-madvise" "cilkplus" "tbb" "serial" )
benchmarks=( "cholesky" "fft" "fib" "heat" "integrate" "knapsack" "lu" "matmul" "nqueens" "quicksort" "rectmul" "strassen" )

nprocs=$(nproc)
max_cores=$nprocs
steps=8

max_runs=50
warm_up_runs=1

common_cppflags="-DMAXRUNS=$max_runs -DCSV -DNUM_WARM_UP_RUNS=$warm_up_runs"
make_flags="-j $nprocs -l $nprocs"

bench_dir="benchmark"
res_dir_base="experiments/$(hostname)"
res_dir="$res_dir_base/$(date "+%Y-%m-%d%Z%H:%M:%S")"



benchmark_runtime() {
	local runtime="${1}"
	bin_dir=$bench_dir/$1
	out_dir=$res_dir/$1

	case $1 in
		nowa)
			flags="CPPFLAGS=$common_cppflags"
			make $make_flags clean
			make $make_flags
			make $make_flags install
			bin_dir=$bench_dir/nowa
			;;
		nowa-madvise)
			flags="CPPFLAGS=$common_cppflags -DFIBRIL_USE_MADVISE"
			bin_dir=$bench_dir/nowa
			make $make_flags clean
			make $make_flags "$flags"
			make $make_flags install
			;;
		nowa-the-queue)
			flags="CPPFLAGS=$common_cppflags -DDEQUE_USE_THE"
			bin_dir=$bench_dir/nowa
			make $make_flags clean
			make $make_flags "$flags"
			make $make_flags install
			;;
		fibril)
			flags="CPPFLAGS=$common_cppflags"
			bin_dir=$bench_dir/fibril
			make -C fibril $make_flags clean
			make -C fibril $make_flags "$flags"
			make -C fibril $make_flags install
			;;
		fibril-madvise)
			flags="CPPFLAGS=$common_cppflags -DFIBRIL_USE_MADVISE"
			bin_dir=$bench_dir/fibril
			make -C fibril $make_flags clean
			make -C fibril $make_flags "$flags"
			make -C fibril $make_flags install
			;;
		*)
			flags="CPPFLAGS=$common_cppflags"
			;;
	esac

	make $make_flags -C $bin_dir clean
	make $make_flags "$flags" -C $bin_dir ${benchmarks[@]}
	mkdir -p $out_dir

	for b in ${benchmarks[@]}; do
		out_file=$out_dir/$b.csv

		echo -e "cores\trun\ttime\tmaxrss\trss\tfaults" > $out_file

		local time=$(date +%T)
		if [ $1 == "serial" ]; then
			echo "${time} Benchmarking ${b} with ${runtime}"
			BENCHMARK_NPROCS=1 stdbuf --output=L \
					$bin_dir/$b |\
					tee -a $out_file
		else
			s=$((max_cores/steps))
			for i in "1" $(seq $s $s $max_cores); do
				echo "${time} Benchmarking ${b} with ${runtime} using ${i} cores"
				BENCHMARK_NPROCS=$i stdbuf --output=L \
						$bin_dir/$b |\
						tee -a $out_file
			done
		fi
	done
}

meta_data() { # $1 res_dir
	out_file=$1/meta.csv

	echo -e "commitid\tkernelname\tkernelrelease\tosname\tosrelease\tmaxruns\tmaxcores\tsteps" > $out_file

	echo -e -n "$(git rev-parse HEAD)\t" >> $out_file
	echo -e -n "$(uname -s)\t" >> $out_file
	echo -e -n "$(uname -r)\t" >> $out_file
	echo -e -n "$(lsb_release -s -i)\t" >> $out_file
	echo -e -n "$(lsb_release -s -r)\t" >> $out_file
	echo -e -n "$max_runs\t" >> $out_file
	echo -e -n "$max_cores\t" >> $out_file
	echo -e "$((steps + 1))" >> $out_file

	IFS="" read -ra tmp <<< $(gcc --version)
	echo "${tmp[0]}" > $1/gcc_version.txt

	[ -n "$tags" ] && echo "$tags" > $1/tags.txt

	lscpu -J > $1/cpu.json
}

usage() {
	echo "usage:"
	echo "./run_benchmarks.sh [options]"
	echo -e "\t-h\t\tdisplay this help and exit"
	echo -e "\t-t=TAGS\t\tsave TAGS to tags.txt in the output directory"
	echo -e "\t-r=RUNTIMES\tbenchmark only RUNTIMES"
}



while [ $# -gt 0 ]; do
	case $1 in
		-d*)
			set -x
			;;
		-r=*)
			runtimes=( )
			IFS="," read -ra rs <<< $(echo $1 | cut -d"=" -f 2)
			for r in ${rs[@]}; do
				runtimes+=( $r )
			done
			;;
		-b=*)
			benchmarks=( )
			IFS="," read -ra rs <<< $(echo $1 | cut -d"=" -f 2)
			for r in ${rs[@]}; do
				benchmarks+=( $r )
			done
			;;
		-t=*)
			tags=$(echo $1 | cut -d'=' -f 2)
			;;
		-m=*)
			max_runs=$(echo $1 | cut -d'=' -f 2)
			common_cppflags="-DMAXRUNS=$max_runs -DCSV -DNUM_WARM_UP_RUNS=$warm_up_runs"
			;;
		-w=*)
			warm_up_runs=$(echo $1 | cut -d'=' -f 2)
			common_cppflags="-DMAXRUNS=$max_runs -DCSV -DNUM_WARM_UP_RUNS=$warm_up_runs"
			;;
		-s=*)
			steps=$(echo $1 | cut -d'=' -f 2)
			if [ $(((max_cores/steps)*steps)) -ne $max_cores ]; then
				echo "Warning: Number of cores not dividable by number of steps!"
			fi
			;;
		-h)
			usage
			exit 0
			;;
		*)
			usage
			exit 1
			;;
	esac
	shift
done


make $make_flags
make $make_flags install

mkdir -p $res_dir
ln -s -T -f $(basename $res_dir) $res_dir_base/latest

meta_data $res_dir

for r in ${runtimes[@]}; do
	benchmark_runtime $r
done


exit 0
