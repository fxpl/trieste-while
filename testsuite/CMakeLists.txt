include (${trieste_SOURCE_DIR}/cmake/testsuite.cmake)

testsuite(while)

# Fuzz the compiler passes and the components of each optimisation level. The
# seed is random, so a failure may not reproduce on retry: the output includes
# the seed to rerun with.
foreach(level 0 1 2)
  add_test(NAME while/fuzz/O${level} COMMAND while test -O${level} -c 200)
endforeach()
