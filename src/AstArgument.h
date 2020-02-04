/*
 * Souffle - A Datalog Compiler
 * Copyright (c) 2013, Oracle and/or its affiliates. All rights reserved
 * Licensed under the Universal Permissive License v 1.0 as shown at:
 * - https://opensource.org/licenses/UPL
 * - <souffle root>/licenses/SOUFFLE-UPL.txt
 */

/************************************************************************
 *
 * @file AstArgument.h
 *
 * Define the classes Argument, Variable, and Constant to represent
 * variables and constants in literals. Variable and Constant are
 * sub-classes of class argument.
 *
 ***********************************************************************/

#pragma once

#include "AstNode.h"
#include "AstType.h"
#include "FunctorOps.h"
#include "SymbolTable.h"
#include "Util.h"
#include <cassert>
#include <cstddef>
#include <memory>
#include <ostream>
#include <string>
#include <utility>
#include <vector>

namespace souffle {

/* Forward declarations */
class AstLiteral;

/**
 * Intermediate representation of an argument
 */
class AstArgument : public AstNode {
public:
    ~AstArgument() override = default;

    /** Obtains a list of all embedded child nodes */
    std::vector<const AstNode*> getChildNodes() const override {
        return std::vector<const AstNode*>();  // type is just cached, not essential
    }

    /** Create clone */
    AstArgument* clone() const override = 0;
};

/**
 * Subclass of Argument that represents a named variable
 */
class AstVariable : public AstArgument {
public:
    AstVariable(std::string n) : AstArgument(), name(std::move(n)) {}

    /** Updates this variable name */
    void setName(const std::string& name) {
        this->name = name;
    }

    /** @return Variable name */
    const std::string& getName() const {
        return name;
    }

    /** Print argument to the given output stream */
    void print(std::ostream& os) const override {
        os << name;
    }

    /** Creates a clone of this AST sub-structure */
    AstVariable* clone() const override {
        auto* res = new AstVariable(name);
        res->setSrcLoc(getSrcLoc());
        return res;
    }

    /** Mutates this node */
    void apply(const AstNodeMapper& /*mapper*/) override {
        // no sub-nodes to consider
    }

protected:
    /** Variable name */
    std::string name;

    /** Implements the node comparison for this node type */
    bool equal(const AstNode& node) const override {
        assert(nullptr != dynamic_cast<const AstVariable*>(&node));
        const auto& other = static_cast<const AstVariable&>(node);
        return name == other.name;
    }
};

/**
 * Subclass of Argument that represents an unnamed variable
 */
class AstUnnamedVariable : public AstArgument {
public:
    AstUnnamedVariable() : AstArgument() {}

    /** Print argument to the given output stream */
    void print(std::ostream& os) const override {
        os << "_";
    }

    /** Creates a clone of this AST sub-structure */
    AstUnnamedVariable* clone() const override {
        auto* res = new AstUnnamedVariable();
        res->setSrcLoc(getSrcLoc());
        return res;
    }

    /** Mutates this node */
    void apply(const AstNodeMapper& /*mapper*/) override {
        // no sub-nodes to consider
    }

protected:
    /** Implements the node comparison for this node type */
    bool equal(const AstNode& node) const override {
        assert(nullptr != dynamic_cast<const AstUnnamedVariable*>(&node));
        return true;
    }
};

/**
 * Subclass of Argument that represents a counter (for projections only)
 */
class AstCounter : public AstArgument {
public:
    AstCounter() : AstArgument() {}

    /** Print argument to the given output stream */
    void print(std::ostream& os) const override {
        os << "$";
    }

    /** Creates a clone of this AST sub-structure */
    AstCounter* clone() const override {
        auto* res = new AstCounter();
        res->setSrcLoc(getSrcLoc());
        return res;
    }

    /** Mutates this node */
    void apply(const AstNodeMapper& /*mapper*/) override {
        // no sub-nodes to consider within constants
    }

protected:
    /** Implements the node comparison for this node type */
    bool equal(const AstNode& node) const override {
        assert(nullptr != dynamic_cast<const AstCounter*>(&node));
        return true;
    }
};

/**
 * Subclass of Argument that represents a datalog constant value
 */
class AstConstant : public AstArgument {
public:
    /** @return Return the ram representation of this constant */
    RamDomain getRamRepresentation() const {
        return ramRepresentation;
    }

    /** Mutates this node */
    void apply(const AstNodeMapper& /*mapper*/) override {
        // no sub-nodes to consider within constants
    }

protected:
    AstConstant(RamDomain i) : AstArgument(), ramRepresentation(i) {}

    /** Constant represented as RamDomain value.
     * In the case of a string this is the entry in symbol table.
     * In the case of a float/unsigned this is the bit cast of the value. */
    RamDomain ramRepresentation;

    /** Implements the node comparison for this node type */
    bool equal(const AstNode& node) const override {
        assert(nullptr != dynamic_cast<const AstConstant*>(&node));
        const auto& other = static_cast<const AstConstant&>(node);
        return ramRepresentation == other.ramRepresentation;
    }
};

/**
 * Subclass of Argument that represents a datalog constant value
 */
class AstStringConstant : public AstConstant {
public:
    AstStringConstant(SymbolTable& symTable, const std::string& c)
            : AstConstant(symTable.lookup(c)), symTable(symTable) {}

    /** @return String representation of this Constant */
    const std::string& getConstant() const {
        return symTable.resolve(ramRepresentation);
    }

    /**  Print argument to the given output stream */
    void print(std::ostream& os) const override {
        os << "\"" << getConstant() << "\"";
    }

    /** Creates a clone of this AST sub-structure */
    AstStringConstant* clone() const override {
        auto* res = new AstStringConstant(symTable, ramRepresentation);
        res->setSrcLoc(getSrcLoc());
        return res;
    }

private:
    SymbolTable& symTable;
    AstStringConstant(SymbolTable& symTable, size_t index) : AstConstant(index), symTable(symTable) {}
};

/**
 * Subclass of Argument that represents a souffle constant numeric value.
 */
template <typename numericType>  // numericType ⲉ {RamSigned, RamUnsigned, RamFloat}
class AstNumericConstant : public AstConstant {
public:
    AstNumericConstant(numericType value) : AstConstant(ramBitCast(value)) {}

    /**  Print argument to the given output stream */
    void print(std::ostream& os) const override {
        os << getConstant();
    }

    /** Get the value of the constant. */
    numericType getConstant() const {
        return ramBitCast<numericType>(ramRepresentation);
    }

    /** Creates a clone of this AST sub-structure */
    AstNumericConstant<numericType>* clone() const override {
        auto* copy = new AstNumericConstant<numericType>(getConstant());
        copy->setSrcLoc(getSrcLoc());
        return copy;
    }
};

// This definitions are used by AstVisitor.
using AstNumberConstant = AstNumericConstant<RamSigned>;
using AstFloatConstant = AstNumericConstant<RamFloat>;
using AstUnsignedConstant = AstNumericConstant<RamUnsigned>;

/**
 * Subclass of AstConstant that represents a null-constant (no record)
 */
class AstNullConstant : public AstConstant {
public:
    AstNullConstant() : AstConstant(0) {}

    /**  Print argument to the given output stream */
    void print(std::ostream& os) const override {
        os << '-';
    }

    /** Creates a clone of this AST sub-structure */
    AstNullConstant* clone() const override {
        auto* res = new AstNullConstant();
        res->setSrcLoc(getSrcLoc());
        return res;
    }
};

/**
 * A common base class for AST functors
 */
// TODO (azreika): consider pushing some common Intr/Extr functor functionality here
class AstFunctor : public AstArgument {
public:
    std::vector<AstArgument*> getArguments() const {
        return toPtrVector(args);
    }

    /** Get argument at idx. */
    AstArgument* getArg(const size_t idx) const {
        assert(idx < args.size() && "argument index out of bounds");
        return args[idx].get();
    }

    /** set argument */
    void setArg(const size_t idx, std::unique_ptr<AstArgument> arg) {
        assert(idx < args.size() && "argument index out of bounds");
        args[idx] = std::move(arg);
    }

    /** get number of arguments */
    size_t getArity() const {
        return args.size();
    }

    /** Obtains a list of all embedded child nodes */
    std::vector<const AstNode*> getChildNodes() const override {
        auto res = AstArgument::getChildNodes();
        for (auto& cur : args) {
            res.push_back(cur.get());
        }
        return res;
    }

    /** Mutates this node */
    void apply(const AstNodeMapper& map) override {
        for (auto& arg : args) {
            arg = map(std::move(arg));
        }
    }

protected:
    AstFunctor(std::vector<std::unique_ptr<AstArgument>> operands) : args(std::move(operands)){};
    std::vector<std::unique_ptr<AstArgument>> args;
};

/**
 * Subclass of AstFunctor that represents an intrinsic (built-in) functor
 */
class AstIntrinsicFunctor : public AstFunctor {
public:
    template <typename... Operands>
    AstIntrinsicFunctor(FunctorOp function, Operands... operands) : AstFunctor({}), function(function) {
        std::unique_ptr<AstArgument> tmp[] = {std::move(operands)...};
        for (auto& cur : tmp) {
            args.push_back(std::move(cur));
        }

        assert(isValidFunctorOpArity(function, args.size()) && "invalid number of arguments for functor");
    }

    AstIntrinsicFunctor(FunctorOp function, std::vector<std::unique_ptr<AstArgument>> operands)
            : AstFunctor(std::move(operands)), function(function){};

    FunctorOp getFunction() const {
        return function;
    }

    void setFunction(const FunctorOp functor) {
        function = functor;
    }

    /** Get the return type of the functor. */
    RamPrimitiveType checkReturnType() const {
        return functorReturnType(function);
    }

    /** Get type of the functor argument*/
    RamPrimitiveType getArgType(const size_t arg) const {
        return functorOpArgType(arg, function);
    }

    /** Print argument to the given output stream */
    void print(std::ostream& os) const override {
        if (isInfixFunctorOp(function)) {
            os << "(";
            os << join(args, getSymbolForFunctorOp(function), print_deref<std::unique_ptr<AstArgument>>());
            os << ")";
        } else {
            os << getSymbolForFunctorOp(function);
            os << "(";
            os << join(args, ",", print_deref<std::unique_ptr<AstArgument>>());
            os << ")";
        }
    }

    /** Clone this node */
    AstIntrinsicFunctor* clone() const override {
        std::vector<std::unique_ptr<AstArgument>> argsCopy;
        for (auto& arg : args) {
            argsCopy.emplace_back(arg->clone());
        }
        auto res = new AstIntrinsicFunctor(function, std::move(argsCopy));
        res->setSrcLoc(getSrcLoc());
        return res;
    }

protected:
    FunctorOp function;

    /** Implements the node comparison for this node type */
    bool equal(const AstNode& node) const override {
        assert(nullptr != dynamic_cast<const AstIntrinsicFunctor*>(&node));
        const auto& other = static_cast<const AstIntrinsicFunctor&>(node);
        return function == other.function && equal_targets(args, other.args);
    }
};

/**
 * Subclass of AstFunctor that represents an extrinsic (user-defined) functor
 */
class AstUserDefinedFunctor : public AstFunctor {
public:
    AstUserDefinedFunctor() : AstFunctor({}){};
    AstUserDefinedFunctor(std::string name, std::vector<std::unique_ptr<AstArgument>> args)
            : AstFunctor(std::move(args)), name(std::move(name)){};

    ~AstUserDefinedFunctor() override = default;

    /** get name */
    const std::string& getName() const {
        return name;
    }

    /** set name */
    void setName(const std::string& n) {
        name = n;
    }

    /** add argument to argument list */
    void add(std::unique_ptr<AstArgument> arg) {
        args.push_back(std::move(arg));
    }

    /** print user-defined functor */
    void print(std::ostream& os) const override {
        os << '@' << name << "(" << join(args, ",", print_deref<std::unique_ptr<AstArgument>>()) << ")";
    }

    /** Create clone */
    AstUserDefinedFunctor* clone() const override {
        auto res = new AstUserDefinedFunctor();
        for (auto& cur : args) {
            res->args.emplace_back(cur->clone());
        }
        res->setSrcLoc(getSrcLoc());
        res->setName(getName());
        return res;
    }

protected:
    /** name of user-defined functor */
    std::string name;

    /** Implements the node comparison for this node type */
    bool equal(const AstNode& node) const override {
        assert(nullptr != dynamic_cast<const AstUserDefinedFunctor*>(&node));
        const auto& other = static_cast<const AstUserDefinedFunctor&>(node);
        return name == other.name && equal_targets(args, other.args);
    }
};

/**
 * An argument that takes a list of values and combines them into a
 * new record.
 */
class AstRecordInit : public AstArgument {
public:
    AstRecordInit() = default;

    ~AstRecordInit() override = default;

    void add(std::unique_ptr<AstArgument> arg) {
        args.push_back(std::move(arg));
    }

    std::vector<AstArgument*> getArguments() const {
        return toPtrVector(args);
    }

    void print(std::ostream& os) const override {
        os << "[" << join(args, ",", print_deref<std::unique_ptr<AstArgument>>()) << "]";
    }

    /** Creates a clone of this AST sub-structure */
    AstRecordInit* clone() const override {
        auto res = new AstRecordInit();
        for (auto& cur : args) {
            res->args.emplace_back(cur->clone());
        }
        res->setSrcLoc(getSrcLoc());
        return res;
    }

    /** Mutates this node */
    void apply(const AstNodeMapper& map) override {
        for (auto& arg : args) {
            arg = map(std::move(arg));
        }
    }

    /** Obtains a list of all embedded child nodes */
    std::vector<const AstNode*> getChildNodes() const override {
        auto res = AstArgument::getChildNodes();
        for (auto& cur : args) {
            res.push_back(cur.get());
        }
        return res;
    }

protected:
    /** The list of components to be aggregated into a record */
    std::vector<std::unique_ptr<AstArgument>> args;

    /** Implements the node comparison for this node type */
    bool equal(const AstNode& node) const override {
        assert(nullptr != dynamic_cast<const AstRecordInit*>(&node));
        const auto& other = static_cast<const AstRecordInit&>(node);
        return equal_targets(args, other.args);
    }
};

/**
 * An argument capable of casting a value of one type into another.
 */
class AstTypeCast : public AstArgument {
public:
    AstTypeCast(std::unique_ptr<AstArgument> value, AstTypeIdentifier type)
            : value(std::move(value)), type(std::move(type)) {}

    void print(std::ostream& os) const override {
        os << "as(" << *value << "," << type << ")";
    }

    AstArgument* getValue() const {
        return value.get();
    }

    const AstTypeIdentifier& getType() const {
        return type;
    }

    void setType(const AstTypeIdentifier& type) {
        this->type = type;
    }

    /** Obtains a list of all embedded child nodes */
    std::vector<const AstNode*> getChildNodes() const override {
        auto res = AstArgument::getChildNodes();
        res.push_back(value.get());
        return res;
    }

    /** Creates a clone of this AST sub-structure */
    AstTypeCast* clone() const override {
        auto res = new AstTypeCast(std::unique_ptr<AstArgument>(value->clone()), type);
        res->setSrcLoc(getSrcLoc());
        return res;
    }

    /** Mutates this node */
    void apply(const AstNodeMapper& map) override {
        value = map(std::move(value));
    }

protected:
    /** The value to be casted */
    std::unique_ptr<AstArgument> value;

    /** The target type name */
    AstTypeIdentifier type;

    /** Implements the node comparison for this node type */
    bool equal(const AstNode& node) const override {
        assert(nullptr != dynamic_cast<const AstTypeCast*>(&node));
        const auto& other = static_cast<const AstTypeCast&>(node);
        return type == other.type && *value == *other.value;
    }
};

/**
 * An argument aggregating a value from a sub-query.
 */
class AstAggregator : public AstArgument {
public:
    /**
     * The kind of utilised aggregation operator.
     * Note: lower-case is utilized due to a collision with
     *  constants in the parser.
     */
    enum Op { min, max, count, sum };

    /** Creates a new aggregation node */
    AstAggregator(Op fun) : fun(fun), expr(nullptr) {}

    /** Destructor */
    ~AstAggregator() override = default;

    // -- getters and setters --

    Op getOperator() const {
        return fun;
    }

    void setTargetExpression(std::unique_ptr<AstArgument> arg) {
        expr = std::move(arg);
    }

    const AstArgument* getTargetExpression() const {
        return expr.get();
    }

    std::vector<AstLiteral*> getBodyLiterals() const {
        return toPtrVector(body);
    }

    void clearBodyLiterals() {
        body.clear();
    }

    void addBodyLiteral(std::unique_ptr<AstLiteral> lit) {
        body.push_back(std::move(lit));
    }

    // -- others --

    /** Prints this instance in a parse-able format */
    void print(std::ostream& os) const override;

    /** Obtains a list of all embedded child nodes */
    std::vector<const AstNode*> getChildNodes() const override;

    /** Creates a clone of this AST sub-structure */
    AstAggregator* clone() const override;

    /** Mutates this node */
    void apply(const AstNodeMapper& map) override {
        if (expr) {
            expr = map(std::move(expr));
        }
        for (auto& cur : body) {
            cur = map(std::move(cur));
        }
    }

protected:
    /** Implements the node comparison for this node type */
    bool equal(const AstNode& node) const override {
        assert(nullptr != dynamic_cast<const AstAggregator*>(&node));
        const auto& other = static_cast<const AstAggregator&>(node);
        return fun == other.fun && equal_ptr(expr, other.expr) && equal_targets(body, other.body);
    }

private:
    /** The aggregation operator of this aggregation step */
    Op fun;

    /** The expression to be aggregated */
    std::unique_ptr<AstArgument> expr;

    /** A list of body-literals forming a sub-query which's result is projected and aggregated */
    std::vector<std::unique_ptr<AstLiteral>> body;
};

/**
 * An argument taking its value from an argument of a RAM subroutine
 */
class AstSubroutineArgument : public AstArgument {
public:
    AstSubroutineArgument(size_t n) : AstArgument(), number(n) {}

    /** Return argument number */
    size_t getNumber() const {
        return number;
    }

    /** Print argument to the given output stream */
    void print(std::ostream& os) const override {
        os << "arg_" << number;
    }

    /** Creates a clone of this AST sub-structure */
    AstSubroutineArgument* clone() const override {
        auto* res = new AstSubroutineArgument(number);
        res->setSrcLoc(getSrcLoc());
        return res;
    }

    /** Mutates this node */
    void apply(const AstNodeMapper& /*mapper*/) override {
        // no sub-nodes to consider
    }

protected:
    /** Implements the node comparison for this node type */
    bool equal(const AstNode& node) const override {
        assert(nullptr != dynamic_cast<const AstSubroutineArgument*>(&node));
        const auto& other = static_cast<const AstSubroutineArgument&>(node);
        return number == other.number;
    }

private:
    /** Index of argument in argument list*/
    size_t number;
};

}  // end of namespace souffle
