#pragma once
#include "lang.hh"

namespace whilelang {
    using namespace ::trieste;

    std::string get_identifier(const Node &node);

    std::string build_qualified_name(const Node ident);
}
