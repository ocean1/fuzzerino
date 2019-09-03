# Instrumentation overhead experiment

### Tools used

* Python 3 `timeit` module ([https://docs.python.org/3/library/timeit.html](https://docs.python.org/3/library/timeit.html))

### How to run

`./run.sh`

### Example output

    (map_binary) no instrumentation: 856 usec
    (map_binary) instrumented, no mutations: 2.31 msec
    (map_binary) instrumented, mutations: 2.76 msec

    (t1) no instrumentation: 853 usec
    (t2) instrumented, no mutations: 2.31 msec
    (t3) instrumented, mutations: 2.67 msec

    (t2) no instrumentation: 859 usec
    (t2) instrumented, no mutations: 2.34 msec
    (t3) instrumented, mutations: 2.74 msec

    (gif2apng) no instrumentation: 5.4 msec
    (gif2apng) instrumented, no mutations: 7.64 msec
    (gif2apng) instrumented, mutations: 8.11 msec

    (miniz) no instrumentation: 1.08 msec
    (miniz) instrumented, no mutations: 2.6 msec
    (miniz) instrumented, mutations: 2.96 msec

### Experiment results

* Instrumentation overhead is not much of an issue. The difference in execution time is fairly constant between very small test programs (t1, t2) and bigger programs (gif2apng). This suggests that the overhead is mostly due to the initialization phase of the linked runtime but, thanks to the forkserver, it is performed only once.