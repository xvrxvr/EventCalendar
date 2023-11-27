#include "common.h"

inline bool rand_bit()
{
    return esp_random() & 1;
}

inline int pow10(int v)
{
    int res=1;
    while(v--) res*=10;
    return res;
}

class CantGenerate : public std::exception {};

using I2 = std::pair<int, int>;

class GenSetup {
    std::initializer_list<int> v_range;
    int v_abs_max;
    int sum_of_v_range;
protected:
    bool final_check = false;
public:
    // v_range_list should has static scope!
    GenSetup(std::initializer_list<int> v_range_list) : v_range(v_range_list), v_abs_max(pow10(v_range_list.size())) 
    {
        sum_of_v_range = 0;
        for(auto v: v_range) sum_of_v_range += v;
        if (!sum_of_v_range) sum_of_v_range=1;
    }

    int filter(int selected) const
    {
        return std::max(1, std::min<int>(v_range.size(), selected));
    }

    int choose_digits() const
    {
        int index=1, idx = esp_random() % sum_of_v_range;
        for(auto v: v_range)
        {
            idx -= v;
            if (idx <= 0) return index;
            ++index;
        }
        return index;
    }

    bool assert_max(int val) const
    {
        return val <=0 || val >= v_abs_max;
    }

    bool is_final_check() const {return final_check;}
};


enum Opcode {
    O_Plus,
    O_Minus,
    O_Mul,
    O_Div,
    O_Leaf
};

class Operation {
    friend class Generator;

    int prio() const {return opcode >> 1;}

protected:
    const Opcode opcode;
    const GenSetup& setup;
    std::shared_ptr<Operation> left, right;
public:
    using Ptr = std::shared_ptr<Operation>;
    Operation(Opcode opcode, const GenSetup& setup, Ptr left = {}, Ptr right = {}) : opcode(opcode), setup(setup), left(left), right(right) {}
    virtual ~Operation() {}

    static Ptr build(Opcode opcode, const GenSetup& setup);
   
    virtual I2 digits(int selected) const {return {selected, selected};}

    virtual void check()
    {
        if (left) left->check();
        if (right) right->check();
    }

    virtual bool fix(int new_val) = 0;

    virtual int get_val() const = 0;

    I2 values() const {return {left->get_val(), right->get_val()};}

    void force(Ptr&, int new_value);

    virtual void to_string(Prn& prn)
    {
        static const char* mnems[] = {"+", "-", "*", "/", ""};
        bool left_br = left->prio() < prio();
        bool right_br = right->prio() <= prio();
        if (left_br) prn.strcat("(");
        left->to_string(prn);
        if (left_br) prn.strcat(")");
        prn.strcat(mnems[opcode]);
        if (right_br) prn.strcat("(");
        right->to_string(prn);
        if (right_br) prn.strcat(")");
    }
};


class OperationPlus : public Operation {
public:
    OperationPlus(const GenSetup& setup, Ptr left = {}, Ptr right = {}) : Operation(O_Plus, setup, left, right) {}

    virtual int get_val() const override {return left->get_val() + right->get_val();}

    virtual bool fix(int new_val) override
    {
        if (setup.assert_max(new_val)) return false;
        auto [l, r] = values();
        return left->fix(new_val - r) || right->fix(new_val - l);
    }
};

class OperationMinus: public Operation {
public:
    OperationMinus(const GenSetup& setup, Ptr left = {}, Ptr right = {}) : Operation(O_Minus, setup, left, right) {}

    virtual int get_val() const override
    {
        auto [l, r] = values();
        int diff = l - r;
        if (setup.is_final_check() && diff <= 0) throw CantGenerate();
        return diff;
    }

    virtual void check() override
    {
        Operation::check();
        auto [l, r] = values();
        int diff = l - r;
        if (diff > 0) return;
        if (l < 2 && r < 2) throw CantGenerate();
        int rr = l / 4 + esp_random() % (l/2+1);     // Eval new l/r for existent r/l, based on values of existent
        int ll = r + r / 2 + esp_random() % (r/2+1);
        if (rand_bit())
        {
           if (left->fix(ll) || right->fix(rr)) return;
        }
        else
        {
           if (right->fix(rr) || left->fix(ll)) return;
        }
        if (rand_bit()) left->force(left, ll);
        else right->force(right, rr);
    }


    virtual bool fix(int new_val) override
    {
        if (setup.assert_max(new_val)) return false;
        auto [l, r] = values();
        return left->fix(new_val + r) || right->fix(l - new_val);
    }
};

class OperationMul : public Operation {
public:
    OperationMul(const GenSetup& setup, Ptr left = {}, Ptr right = {}) : Operation(O_Mul, setup, left, right) {}

    virtual I2 digits(int selected) const override 
    {
        int l_digit = setup.filter(selected/2);
        int r_digit = setup.filter(selected - l_digit);
        return {l_digit, r_digit};
    }

    virtual int get_val() const override {return left->get_val() * right->get_val();}

    virtual bool fix(int new_val) override
    {
        if (setup.assert_max(new_val)) return false;
        auto [l, r] = values();
        if ((new_val % r) == 0 && left->fix(new_val / r)) return true;
        if ((new_val % l) == 0 && right->fix(new_val / l)) return true;
        return false;
    }
};

class OperationDiv : public Operation {
public:
    OperationDiv(const GenSetup& setup, Ptr left = {}, Ptr right = {}) : Operation(O_Div, setup, left, right) {}

    virtual I2 digits(int selected) const override
    {
        auto l_digit = setup.filter(selected*2);
        auto r_digit = setup.filter(l_digit - selected);
        return {l_digit, r_digit};
    }

    virtual int get_val() const override { return left->get_val() / right->get_val();}

    virtual void check() override
    {
        Operation::check();
        auto [l, r] = values();
        if ((l % r) == 0) return;
        auto div = l / r;
        if (div < 2) throw CantGenerate();
        if (left->fix(div * r)) return;
        left->force(left, div * r);
    }

    virtual bool fix(int new_val) override
    {
        if (setup.assert_max(new_val)) return false;
        auto [l, r] = values();
        if (left->fix(new_val * r)) return true;
        if ((l % new_val) == 0 && right->fix(l / new_val)) return true;
        return false;
    }
};

enum LeafType {
    LT_Value,
    LT_Select
};

class LeafOp : public Operation {
    int value;
public:
    LeafOp(const GenSetup& setup, LeafType lt, int val) : Operation(O_Leaf, setup)
    {
        if (lt == LT_Select)
        {
            int min = pow10(val);
            value = esp_random() % (min*9) + min;
        }
        else
        {
            if (setup.assert_max(val)) throw CantGenerate();
            value = val;
        }
    }

    virtual int get_val() const override {return value;}

    virtual void check() override {}

    virtual bool fix(int new_val)
    {
        if (setup.assert_max(new_val)) return false;
        value = new_val;
        return true;
    }

    virtual void to_string(Prn& prn) override
    {
        prn.cat_printf("%d", value);
    }
};

class Generator : public GenSetup {
    Operation::Ptr _create_tree_shape(int& rest_opc, int selected)
    {
        if (rest_opc <= 0) return Operation::Ptr(new LeafOp(*this, LT_Select, selected));
        Opcode opc = Opcode((esp_random() & 3) + O_Plus);
        auto result = Operation::build(opc, *this);
        --rest_opc;
        auto [l_digits, r_digits] = result->digits(selected);
        if (rand_bit())
        {
            result->left = _create_tree_shape(rest_opc, l_digits);
            result->right = _create_tree_shape(rest_opc, r_digits);
        }
        else
        {
            result->right = _create_tree_shape(rest_opc, r_digits);
            result->left = _create_tree_shape(rest_opc, l_digits);
        }
        return result;
    }

public:
    using GenSetup::GenSetup;

    Operation::Ptr create_tree(int total_opc)
    {
        for(int i=0; i<100; ++i)
        {
            try 
            {
                final_check = false;
                int to = total_opc;
                auto tree = _create_tree_shape(to, choose_digits());
                tree->check();
                final_check = true;
                if (!assert_max(tree->get_val())) return tree;
            } 
            catch(CantGenerate&) {}
        }
        return {};
    }
};

Operation::Ptr Operation::build(Opcode opcode, const GenSetup& setup)
{
    switch(opcode)
    {
        case O_Plus:    return Ptr(new OperationPlus(setup));
        case O_Minus:   return Ptr(new OperationMinus(setup));
        case O_Mul:     return Ptr(new OperationMul(setup));
        case O_Div:     return Ptr(new OperationDiv(setup));
        default:        return {};
    }
}

void Operation::force(Ptr& result, int new_value)
{
    int delta = new_value - get_val();
    if (delta == 0) return;
    Ptr right(new LeafOp(setup, LT_Value, abs(delta)));
    if (delta > 0) result.reset(new OperationPlus(setup, result, right));
    else result.reset(new OperationMinus(setup, result, right));
}


/*
LEVELS = [
    [(1,), 1, 1],   # 1
    [(2,1), 1, 1],  # 2
    [(1,2), 1, 2],  # 3
    [(1,2), 1, 3],  # 4
    [(0,2,1), 1, 3],  #5
    [(0,2,1), 2, 5],  #6
    [(0,1,2), 3, 5]   #7
]

while True:
    level = input("Select Level (1-7): ")
    level = int(level)
    if level < 1 or level > 7:
        print('Invalid level')
        continue
    level = LEVELS[level-1]
    tree = Generator(level[0]).create_tree(randint(level[1], level[2]))
    prompt = f'{tree} = '
    result = tree.get_val()
    while True:
        ans = input(prompt)
        if ans == '?':
            print(result)
            break
        if int(ans) == result:
            print('Yes!')
            break
        print('No...')
*/
