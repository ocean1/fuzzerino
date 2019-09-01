fuzzerino
-----------

![alt text][resources/alpaca_small]

Kang inverse fuzzing (10.000 execs - 5 secs) then switches to normal fuzzer, with fuzzerino we can cover more space:
  - we apply coverage also to the generator since we might be exploring a new path (not present in Kang)
  - we propose it's better to simply run the generator for a certain amount of time (like 2 hours?) and explore as many paths in the generator, and the output "quality" is measured by means of: fuzz values that do NOT crash the program, that generate a unique seed, and that increase the coverage of G. We argue that the generated outputs while not excerting new states in a given program, might do so in others, and once you run the "inverse" fuzzer, better keep them stored and resuse it across several programs (e.g. run them directly and use t-min on them, to select those before running afl and mutating them :))
  - kang thesis does not show the difference in path exploration between a pure inverse and direct fuzzing approach
  - we don't keep track of file sizes as kang do but enforce a hard limit for filesize via ulimits, the generator won't be able to write files bigger than a given limit (e.g., empirically we can set to 10k, mostly we will be generating extremely smaller files instead... :)
  - it's also weird how they limit the operations and fuzzing "strategies", moreover not going "deep" by considering limit values
  - the approach might be very useful to test network protocols - those that need to keep a state in particular, when we want to discover bugs depending on the state (and embedded systems in general).
  - they make no consideration on memory allocation related crashes (e.g., we already disable all malloc checks and plan to let the allocator get some more memory to reduce the chance of crashes by overwriting a close object - also we can switch to some fast malloc implementation like 9cc they even remove frees, for short lived processes it's a great strategy "exit()" garbage collector :P
- https://repository.upenn.edu/cgi/viewcontent.cgi?article=1553&context=cis_papers crazy stuff about program inversion to test different TIFF readers, might do the same experiment to find different behavior in client (e.g., libmagick, PIL, libpng all handle a bit differently png bombs, we find that some images aren't parsed correctly and are cause of bugs in different programs (e.g. eog, feh, out-of-memory in chrome against firefox which runs -- I'd love trying to reproduce this again, and then understand the real cause of crash -- new version of chrome happily runs for now)
- miss comparison with https://pdfs.semanticscholar.org/8c8d/8fd604404df3518f40ba95f3085196e9b929.pdf
- https://ieeexplore.ieee.org/abstract/document/5984105

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

#TODOs
+ use "coverage", we need a inststatus telling us when a given instruction is disabled from fuzzing, when we want to measure if we got new paths explored, and the fuzzing "strategy", this way we can measure for a given input/run how many times a given instruction is executed, we can skip all the others (and measure those that haven't been hit yet by a dry run), and then choose a fuzzing strategy for the others (e.g. wait for iteration X and then mutate, or always mutate, or change the way we mutate it ADD/XOR/NULL the value etc...) this will solve a bunch of problems
+ we can use CAS (content addressable storage) to move the outputs without generating a shitton of outputs by using a fast hash like xxhash, and storing the sample only when needed to avoid fduping over
+ build custom generator respecting useful properties for a nice format (PNG/GIF/PE?...), perhaps instead work on a small client to fuzz some nice network protocol, that's were it may shine (even timing based stuff shouldn't change much, possibly we just want to avoid mutations that cause a disconnect, explore a bit the state of a variable, and decide if you just avoid fuzzing it at all at a given time) fuzz fucking filezilla (like build an ftp server and client!), or putty... `https://juliareda.eu/2018/12/eu-fossa-bug-bounties/`
+ handle Phi instructions: those need to be grouped, at the start of a basic block
+ data analysis (optimization) remove instructions that do not change the flow of data output (sinks are writes or calls/invoke)
+ data analysis: implement fuzzing for pointers, just change randomly stuff in pointed structure, try to infer length of struct perhaps by instrumenting mallocs and taint tracking objects? (maybe a custom allocator might help -- especially when stuff gets allocated precisely for each object) -- let's try maybe fuzzing MQTT? :PPPP better also apache or something

+ implement smart heuristics
    + smart handling of pointers
    + bogus structures
    + reuse allocated memory (structures, class instances...)
+ maybe avoid segfaults by allocating on purpose when crashing and restarting
+ explicitly skip the output of calls that return pointers (malloc) (?)
+ intra-function data-flow optimization: only instrument "leaf-nodes" (e.g. those that are not dominated, for example those that end up in a store, or are not referenced anymore...)
+ optimization: align RAND_POOL at RAND_POOL_SIZE, then increment ptr directly wrapping with % RAND_POOL_SIZE
+ rand pool must be per thread (meh... not much of it used), or need atomic instructions to update
+ add floats to tests
+ https://pdfs.semanticscholar.org/bfca/81c7fbc6b2c32430dc756b936a6b0c2d0585.pdf use tyche-i to generate pseudorand numbers

        /*
        * if (CallInst *Call = dyn_cast<CallInst>(&I)) {
        *  if (Function *CF = Call->getCalledFunction() )
        *    // TODO maybe will explode with null ptr, check returned Name
        *    if (CF->getName() == "malloc"){
        *      continue;
        *    }
        *}
        */
        //if (dyn_cast<LoadInst>(&I)){
        /* TODO: let's just look at Type maybe, if integer instrument,
         * then we'll figure out for pointers or usage as GEP
         * index later */
            // specific case we want to change, load of integers
            // pretty easy as long as it doesn't get to be an
            // index used by a GEP instruction
            // once we find a target, let's get an ID (inst_inst?)
            // and create a small function to instrument and add
            // some value depending on RAND_POOL and GFZ_MAP
            // GFZ_MAP will tell if mutation is active (set by fuzzer)
        //}


Trophies:
+ https://github.com/OpenPrinting/cups-filters/issues/82
+ https://github.com/OpenPrinting/cups-filters/issues/81
+ https://github.com/glennrp/libpng/issues/265
+ https://gitlab.gnome.org/GNOME/eog/issues/33 (private bug, cannot be seen)
