#pragma once

namespace EQuest {

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
    GenSetup(std::initializer_list<int> v_range_list);

    int filter(int selected) const
    {
        return std::max(1, std::min<int>(v_range.size(), selected));
    }

    int choose_digits() const;

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

    virtual void to_string(Prn& prn);
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

    virtual void check() override;

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

    virtual void check() override;
    virtual bool fix(int new_val) override;
};

enum LeafType {
    LT_Value,
    LT_Select
};

class LeafOp : public Operation {
    int value;
public:
    LeafOp(const GenSetup& setup, LeafType lt, int val);

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
    Operation::Ptr _create_tree_shape(int& rest_opc, int selected);

public:
    using GenSetup::GenSetup;

    Operation::Ptr create_tree(int total_opc);
};

} // namespace EQuest
