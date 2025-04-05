#include "internal.hh"
// #include "passes/internal.hh"


namespace whilelang {

using namespace trieste;

Reader reader()
  {
    return {
      "while",
      {
        // Parsing
        expressions(),
        statements(),

        // Checking
        check_refs(),
        eval(),
      },
      whilelang::parser(),
    };
  }

}
