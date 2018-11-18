# test C++
on C++ we might incur in some "overhead" when parsing the AST we need to parse a C++ AST
so we'll need to consider operators and such stuff, is it going to work on non toy examples...? :)
Like upx can still be triggered by the "passed parameter fuzzing technique but we need to cover other stuff as well.... (and optimize, like you just are getting in a param, don't fuzz on line before perhaps that don't change the stuff, we need to instrument only l-side of operations? (or r? o perhaps both depending on target?)
