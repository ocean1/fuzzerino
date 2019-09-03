# Parser coverage experiment

### Tools used

* `lcov` ([http://ltp.sourceforge.net/coverage/lcov.php](http://ltp.sourceforge.net/coverage/lcov.php))

### Quick tutorial

To compile parsers and generate `*.gcno` files:

* add `--coverage` to `CFLAGS` or `CXXFLAGS`

To generate coverage data (`*.gcda` files):

* run parser with all the selected test cases

To capture coverage data:

* `lcov --directory . --capture --output-file cov.info`

To generate human-readable HTML from coverage data:

* `genhtml cov.info -o cov`

To delete hit counts and start over with new test cases:

* `lcov --directory . --zerocounters`

### How to run

`./run.sh`

This bash script runs every `run.sh` script in every subfolder, which in turn runs the parser with every test case folder present in `../../datasets`, generates and captures coverage data, generates human-readable HTML and opens it in Firefox. Between one parser and the other the user is prompted to press any key to continue, in order to avoid opening too many tabs in Firefox.

### Experiment results

* The measured parser coverage for seeds generated with gfz is either superior to the other datasets (upng), or at least reasonable (picopng, libimagequant, libspng, libpng). It must be noted that the datasets we compare ours with are either the result of many hours of fuzzing (afl) or many hours of human work (pngsuite), while our dataset has been pretty much pulled out of thin air in less than 12 hours.