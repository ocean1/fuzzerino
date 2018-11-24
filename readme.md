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

#TODOs
+ implement smart heuristics
    + smart handling of pointers
    + bogus structures
    + reuse allocated memory (structures, class instances...)
+ maybe avoid segfaults by allocating on purpose when crashing and restarting
+ handle Phi instructions
+ explicitly skip the output of calls that return pointers (malloc)
+ intra-function data-flow optimization: only instrument "leaf-nodes" (e.g. those that are not dominated, for example those that end up in a store, or are not referenced anymore...)
+ optimization: put a jump/call and skip over fuzzing, then nop it in child procs
+ optimization: align RAND_POOL at RAND_POOL_SIZE, then increment ptr directly wrapping with % RAND_POOL_SIZE
+ rand pool must be per thread (meh... not much of it used), or need atomic instructions to update

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
