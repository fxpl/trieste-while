# Run the while compiler with static analysis (-si) on every .while sample.
set(TESTSUITE_REGEX "\\.while$")
set(TESTSUITE_DEFINE define_while_analyse_test)

function(define_while_analyse_test test)
  cmake_path(REMOVE_EXTENSION test LAST_ONLY OUTPUT_VARIABLE stem)
  cmake_path(GET test STEM test_name)
  set(node "${stem}_analysis_out")

  testsuite_output_path(output NODE "${node}" FILE "${test_name}.trieste")

  testsuite_add_test(
    NAME "${node}"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    GOLDENS
      exit_code.txt
      stdout.txt
      stderr.txt
      "${test_name}.trieste"
    COMMAND "$<TARGET_FILE:while>" -si "${test}" -o "${output}")
endfunction()
