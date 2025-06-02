#include "../internal.hh"

namespace whilelang {
    using namespace trieste;

    PassDef blockify() {
        PassDef blockify = {
            "blockify",
            blockify_wf,
            dir::bottomup | dir::once,
            {
                T(FunDef) <<
                    (T(FunId)[FunId] *
                     T(ParamList)[ParamList] *
                     (T(Stmt) << T(Block)[Body])) >>
                    [](Match &_) -> Node {
                        Node blocks = Blocks;
                        Node block = Block;
                        Node body = Body;
                        for (auto child : *(_(Body))) {
                            auto stmt = child / Stmt;
                            if (stmt != Label && block->size() == 0)
                                return Error << (ErrorAst << _(Body))
                                             << (ErrorMsg ^ "Block without a label (found " + stmt->str() + ")");

                            if (stmt == Label) {
                                if (block->size() == 0) {
                                    // First label in the block
                                    block << stmt;
                                } else {
                                    // Fallthrough
                                    block << body << (Jump << stmt->clone());
                                    blocks << block;
                                    block = Block << stmt;
                                    body = Body;
                                }
                            } else if (stmt->type().in({Cond, Jump, Return})) {
                                block << body << stmt;
                                blocks << block;
                                block = Block;
                                body = Body;
                            } else {
                                body << child;
                            }
                        }
                        if (block->size() != 0)
                            return Error << (ErrorAst << _(Body))
                                         << (ErrorMsg ^ "Block without a terminator: " + block->str());

                        return FunDef << _(FunId) << _(ParamList) << blocks;
                    },
            }
        };

        return blockify;
    }
}
