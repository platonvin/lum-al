/*
File containing ALL source files for unity build (aka single translation unit)
this gives higher perfomance (like -flto, but better) and decreases compile times on github actions
(github action's CPU is 2 cores)
*/

#include "al.cpp"
#include "setup.cpp"

//for unity build, this is for safity
#undef malloc
#undef calloc