#include "internal.hh"

namespace whilelang {
    using namespace trieste;

    Rewriter compiler() {
        return {
            "compiler",
            {
                to3addr(),
                blockify(),
                compile(),
            },
            whilelang::normalization_wf
        };
    }
}
