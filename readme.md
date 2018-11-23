g-fuzz
-----------

g-fuzzer is a novel fuzzer focusing on exploiting the semantic of a binary format generator
to generate batch of samples to be used for directly fuzzing a target, or that can be employed
as further seeds for afl-fuzz.

(e.g. we will need to compare the output of the seeder and 

code/approach based off afl-fuzz by lcamtuf

instead of focusing on a coverage based approach we focus on changing at runtime
data used in generating and output binary format from a generator

  + gfuzz: contains a fork of afl-fuzz, and instrumentation pass
  + seeder: contains some basic scripts to generate stuff and make eliciting different characteristics of the generator less painful
  + tests: set of minimal tests to start trying out g-fuzz approach, these will be used also as instrumentation/fuzzing test-cases

simple instrumentation (only Integer/Float Type Instructions)
          BB     I     F
UPX     [19437 42291 19437]
libpng  [14549 39535 14549]

