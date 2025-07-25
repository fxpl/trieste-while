#pragma once
#include "../utils.hh"
#include "dataflow_analysis.hh"

namespace whilelang {

    enum class ZeroAbstractType { Bottom, Zero, NonZero, Top };

    struct ZeroLatticeValue {
        ZeroAbstractType type;

        bool operator==(const ZeroLatticeValue &other) const {
            return type == other.type;
        }

        ZeroLatticeValue join(const ZeroLatticeValue &other) const {
            auto top = ZeroLatticeValue::top();
            auto zero = ZeroLatticeValue::zero();
            auto non_zero = ZeroLatticeValue::non_zero();
            auto bottom = ZeroLatticeValue::bottom();

            if (*this == bottom) {
                return other;
            } else if (other == bottom) {
                return *this;
            } else if (*this == zero && other == zero) {
                return zero;
            } else if (*this == non_zero && other == non_zero) {
                return non_zero;
            }

            return top;
        }

        friend std::ostream &
        operator<<(std::ostream &os, const ZeroLatticeValue &value) {
            switch (value.type) {
                case ZeroAbstractType::Top:
                    os << "?";
                    break;
                case ZeroAbstractType::Zero:
                    os << "0";
                    break;
                case ZeroAbstractType::NonZero:
                    os << "N";
                    break;
                case ZeroAbstractType::Bottom:
                    os << "_";
                    break;
            }
            return os;
        }

        static ZeroLatticeValue top() {
            return {ZeroAbstractType::Top};
        }
        static ZeroLatticeValue zero() {
            return {ZeroAbstractType::Zero};
        }
        static ZeroLatticeValue non_zero() {
            return {ZeroAbstractType::NonZero};
        }
        static ZeroLatticeValue bottom() {
            return {ZeroAbstractType::Bottom};
        }
    };

    using ZeroState = std::map<std::string, ZeroLatticeValue>;

    ZeroLatticeValue handle_atom(const Node atom, ZeroState &incoming_state) {
        if (atom == Int) {
            return get_int_value(atom) == 0 ? ZeroLatticeValue::zero() :
                                              ZeroLatticeValue::non_zero();
        } else if (atom == Ident) {
            std::string rhs_var = get_identifier(atom);
            return incoming_state[rhs_var];
        } else {
            return ZeroLatticeValue::top();
        }
    };

    struct ZeroImpl {
		using StateTable = NodeMap<ZeroState>;

        static ZeroState create_state(const Vars &vars) {
            ZeroState state = ZeroState();

            for (auto var : vars) {
                state[var] = ZeroLatticeValue::bottom();
            }
            return state;
        }

        static bool state_join(ZeroState &x, const ZeroState &y) {
            bool changed = false;

            auto it1 = x.begin();
            auto it2 = y.begin();

            while (it1 != x.end() && it2 != y.end()) {
                auto join_res = it1->second.join(it2->second);

                if (join_res != it1->second) {
                    it1->second = join_res;
                    changed = true;
                }
                it1++;
                it2++;
            }

            if (it1 != x.end() || it2 != y.end()) {
                throw std::runtime_error("States are not comparable");
            }

            return changed;
        }

        static ZeroState flow(
            const Node &inst,
            StateTable &state_table,
            std::shared_ptr<ControlFlow> cfg) {
            auto incoming_state = state_table[inst];
            if (inst == Assign) {
                auto var = get_identifier(inst / Ident);
                Node rhs = (inst / Rhs) / Expr;

                if (rhs == Atom) {
                    auto atom = rhs / Expr;
                    incoming_state[var] = handle_atom(atom, incoming_state);
                } else if (rhs == FunCall) {
                    auto prevs = cfg->predecessors(inst);
                    ZeroLatticeValue val = ZeroLatticeValue::bottom();

                    for (auto node : prevs) {
                        if (node == Return) {
                            val = val.join(handle_atom(
                                (node / Atom) / Expr, incoming_state));
                        }
                    }

                    auto pre_fun_call_state = state_table[rhs];
                    pre_fun_call_state[var] = val;
                    return pre_fun_call_state;
                }
            } else if (inst == FunCall) {
                auto params = cfg->get_fun_def(inst) / ParamList;
                auto args = inst / ArgList;

                for (size_t i = 0; i < params->size(); i++) {
                    auto param_id = params->at(i) / Ident;
                    auto arg = args->at(i) / Atom;
                    auto var = get_identifier(param_id);

                    incoming_state[var] =
                        handle_atom(arg / Expr, incoming_state);
                }
            }

            return incoming_state;
        }
    };

    std::ostream &operator<<(std::ostream &os, const ZeroState &state) {
        for (const auto &[_, value] : state) {
            os << std::setw(PRINT_WIDTH) << value;
        }
        return os;
    }
}
