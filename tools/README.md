Tools for fuzzing efficiently with afl-fuzz 

### Start fuzzing

usage:

`./start_fuzzing.sh <number of cores> <arguments to afl-fuzz>`

example:

`./start_fuzzing.sh 8 -i in -o out -x png.dict -- ./readpng`

### Stop fuzzing

usage:

`./stop_fuzzing.sh <number of cores>`

example:

`./stop_fuzzing.sh 8`

### Merge queues and cmin

usage:

`./merge_cmin.sh <afl-fuzz output dir> <path to fuzzed app>`

example:

`./merge_cmin.sh out ./readpng`

Only works if afl's output directory contains subfolders for *named* instances.

### Parallel tmin

usage:

`./afl-ptmin.sh <number of cores> <input dir> <output dir> <path to fuzzed app>`

example:

`screen -dmS tmin ./afl-ptmin.sh 8 in out ./readpng`

### Generate and merge graphs

usage:

`./gen_graphs.sh <afl-fuzz output dir>`

example:

`./gen_graphs.sh out`