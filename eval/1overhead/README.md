# Instrumentation overhead experiment

### Tools used

* Python 3 `timeit` module ([https://docs.python.org/3/library/timeit.html](https://docs.python.org/3/library/timeit.html))

### How to run

`./run.sh`

### Example output

    (map_binary) no instrumentation: 922 usec
    (map_binary) instrumented, no mutations: 2.41 msec
    (map_binary) instrumented, mutations: 2.96 msec

    (t1) no instrumentation: 670 usec
    (t1) instrumented, no mutations: 2.48 msec
    (t1) instrumented, mutations: 2.82 msec

    (t2) no instrumentation: 676 usec
    (t2) instrumented, no mutations: 2.47 msec
    (t2) instrumented, mutations: 2.82 msec

    (gif2apng) no instrumentation: 5.87 msec
    (gif2apng) instrumented, no mutations: 8.34 msec
    (gif2apng) instrumented, mutations: 8.66 msec

    (miniz) no instrumentation: 1.07 msec
    (miniz) instrumented, no mutations: 2.55 msec
    (miniz) instrumented, mutations: 2.88 msec

### Experiment results

* Instrumentation overhead is not much of an issue. The difference in execution time is fairly constant between very small test programs (t1, t2) and bigger programs (gif2apng). This suggests that the overhead is mostly due to the initialization phase of the linked runtime but, thanks to the forkserver, it is performed only once.