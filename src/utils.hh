#pragma once
#include "lang.hh"

namespace whilelang {
    using namespace ::trieste;

    std::string get_identifier(const Node &node);

    std::string build_qualified_name(const Node ident);

    // The variables and parameters an Ident refers to, leaving out functions
    // of the same name
    Nodes lookup_variable(const Node &ident);

    // Thrown when a pass gets input that an earlier pass should have
    // rejected. This never happens when compiling, but can when a pass is
    // tested in isolation.
    struct InvalidInput : std::runtime_error {
        using std::runtime_error::runtime_error;
    };
}
