cd ..
cd hotspot
make
cd ..
make

#setting $GRAPHITE_ROOT to HotSniper7's root directory
export GRAPHITE_ROOT=$(pwd)
cd benchmarks
#setting $BENCHMARKS_ROOT to the benchmarks directory
export BENCHMARKS_ROOT=$(pwd)
#compiling the benchmarks
make
cd ..
cd simulationcontrol
PYTHONIOENCODING="UTF-8" python3 run.py
PYTHONIOENCODING="UTF-8" python3 parse_results.py
