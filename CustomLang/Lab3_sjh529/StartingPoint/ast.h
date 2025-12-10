// =============================================================================
//   ast.h — AST for TIPS Subset (matches PDF diagrams)
// =============================================================================
// MSU CSE 4714/6714 Capstone Project (Fall 2025)
// Author: Derek Willis
//
//   Part 1 : PROGRAM, BLOCK, WRITE
//   Part 2 : VAR/READ/ASSIGN + symtab + Compound Statement + BLOCK (fr this time)
//   Part 3 : expression/simple/term/factor + relations/logic/arithmetic
//   Part 4 : IF/WHILE, custom op/keyword, skins
// =============================================================================
#pragma once
#include <memory>
#include <string>
#include <iostream>
#include <vector>
#include <map>
#include <variant>
#include <cassert>
#include <cmath>
using namespace std;
using Value = variant<int, double>;

// -----------------------------------------------------------------------------
// Pretty printer
// -----------------------------------------------------------------------------
inline void ast_line(ostream &os, string prefix, bool last, string label)
{
  os << prefix << (last ? "└── " : "├── ") << label << "\n";
}

// Symbol Table
inline map<string, variant<int, double>> symbolTable;

// Helper Functions
inline double as_double(const Value &v)
{
  return holds_alternative<int>(v) ? static_cast<double>(get<int>(v)) : get<double>(v);
}
inline int as_int_strict(const Value &v)
{
  if (!holds_alternative<int>(v))
    throw runtime_error("MOD requires INTEGER operands");
  return get<int>(v);
}

// Forward Declarations
struct Write;
struct Block;
struct Statement;
struct assignStmt;
struct readStmt;
struct writeStmt;
struct compoundStmt;
struct Program;

// TODO: Define and Implement structures to hold each data node
// TODO: Overload << for Program

// PART 3
struct ValueNode
{
  virtual void print_tree(ostream &os, string prefix) = 0;
  virtual Value interpret(ostream &out) = 0;
};

struct IntLitNode : ValueNode
{
  int v;

  // Print Tree
  void print_tree(ostream &os, string prefix)
  {
    ast_line(os, prefix, true, "IntLitNode: " + to_string(v));
  }

  // Interpret
  Value interpret(ostream &out)
  {
    (void)out;
    // Provides other functions with v (when called with interpret)
    return v;
  }
};

struct RealLitNode : ValueNode
{
  double v;

  // Print Tree
  void print_tree(ostream &os, string prefix)
  {
    ast_line(os, prefix, true, "RealLitNode: " + to_string(v));
  }

  // Interpret
  Value interpret(ostream &out)
  {
    (void)out;
    // Provides other functions with v (when called with interpret)
    return v;
  }
};

struct IdentNode : ValueNode
{
  string name;

  // Print Tree
  void print_tree(ostream &os, string prefix)
  {
    ast_line(os, prefix, true, "IdentNode: " + name);
  }

  // Interpret
  Value interpret(ostream &out)
  {
    // Reads symbolTable
    auto it = symbolTable.find(name); // Searches for the key
    return it->second;                // returns the value associated with the key of name
  }
};

struct UnaryOp : ValueNode
{
  Token op;
  unique_ptr<ValueNode> sub;

  // Print Tree
  void print_tree(ostream &os, string prefix)
  {
    ast_line(os, prefix, false, "Unary");
    ast_line(os, prefix + "|  ", false, "op: " + string(tokName(op)));
    sub->print_tree(os, prefix + "  ");
  }

  // Interpret
  Value interpret(ostream &out)
  {
    (void)out; // Out is intentionally unused throughout the interpret function
    // Check for MINUS
    if (op == MINUS)
    {
      Value v = sub->interpret(out); //
      if (holds_alternative<int>(v)) // Checks for v to be an int
        return -get<int>(v);         // Return the negative value of v if it is an int
    }

    // Check for INCREMENT or DECREMENT
    if (op == INCREMENT || op == DECREMENT)
    {
      auto *id = dynamic_cast<IdentNode *>(sub.get()); // Sets id to the value within sub
      // Checks for invalid id
      if (!id)
        throw runtime_error("++/-- must apply to an identifier");
      auto &slot = symbolTable[id->name]; // Sets slot to the id name
      // Checks for INC or DEC for integers
      if (holds_alternative<int>(slot))
      {
        int x = get<int>(slot);
        x += (op == INCREMENT ? 1 : -1); // INCs or DECs x depending on intention
        slot = x;
        return x;
      }

      // Checks for INC or DEC for doubles
      else
      {
        double x = get<double>(slot);
        x += (op == INCREMENT ? 1.0 : -1.0); // INCs or DECs depending on intention
        slot = x;
        return x;
      }
    }
    throw runtime_error("Unknown unary operator");
  }
};

struct BinaryOp : ValueNode
{
  Token op;
  unique_ptr<ValueNode> left, right;

  // Print Tree
  void print_tree(ostream &os, string prefix)
  {
    ast_line(os, prefix, false, "Binary " + string(tokName(op)));
    left->print_tree(os, prefix + "|  ");
    right->print_tree(os, prefix + "  ");
  }

  // Interpret
  Value interpret(ostream &out)
  {
    (void)out;
    Value a = left->interpret(out);
    Value b = right->interpret(out);

    switch (op)
    {
    case PLUS:
    case MINUS:
    {
      if (holds_alternative<int>(a) && holds_alternative<int>(b))
      {
        return (op == PLUS) ? get<int>(a) + get<int>(b)
                            : get<int>(a) - get<int>(b);
      }
      double ad = as_double(a), bd = as_double(b);
      return (op == PLUS) ? ad + bd : ad - bd;
    }

    case MULTIPLY:
    case DIVIDE:
    {
      // Checks for both being int
      if (holds_alternative<int>(a) && holds_alternative<int>(b))
      {
        // Return appropriate answer based on op
        return (op == MULTIPLY) ? get<int>(a) * get<int>(b)
                                : get<int>(a) / get<int>(b);
      }
      // Else convert them to doubles
      double aDoub = as_double(a), bDoub = as_double(b);
      // Return appropriate answer based on op
      return (op == MULTIPLY) ? aDoub * bDoub : aDoub / bDoub;
    }

    case MOD: // Only works for 2 int
    {
      // Checks for both being int
      // if (holds_alternative<double>(a) && holds_alternative<double>(b))
      // {
      //   return get<int>(a) % get<int>(b);
      // }
      // throw runtime_error("MOD must only have INTs.");
      int intA = as_int_strict(a);
      int intB = as_int_strict(b);
      return intA % intB;
    }

    case CUSTOM_OPER: // Only works for 2 doubles
    {
      // Checks for both being int
      if (holds_alternative<int>(a) && holds_alternative<int>(b))
      {
        throw runtime_error("EXPON must only have doubles.");
      }
      // Else covnert them to doubles
      double aDoub = as_double(a), bDoub = as_double(b);
      // Return appropriate answer based on op
      return pow(aDoub, bDoub);
    }
    default:
      throw runtime_error("BinaryOp: Fails to match any case.");
    }
  }
};

// PART 2
struct Statement // Base clase for all statements
{
  // Member Variables
  // Member Functions
  virtual void print_tree(ostream &out, string prefix) = 0;
  virtual void interpret(ostream &) = 0;
};

struct assignStmt : Statement // Update an existing variable's value
{
  // Member Variables
  string id;                 // key of the symbolTable
  unique_ptr<ValueNode> rhs; // Right hand side of the assign

  // Member Functions
  void print_tree(ostream &os, string prefix)
  {
    ast_line(os, prefix, false, "Assign " + id + " :=");
    rhs ? rhs->print_tree(os, prefix + "  ")
        : ast_line(os, prefix + "  ", true, "(null expr)");
  }

  void interpret(ostream &out)
  {
    (void)out;
    auto val = rhs->interpret(out);
    auto &slot = symbolTable[id];

    if (auto p = get<int>(slot))
    {
      // Slot currently holds int -> assign an int
      p = static_cast<int>(
          holds_alternative<int>(val)
              ? get<int>(val)
              : get<double>(val));
    }
    else if (auto p = get_if<double>(&slot))
    {
      // Slot currently holds double -> assign a double
      *p = static_cast<double>(
          holds_alternative<int>(val)
              ? get<int>(val)
              : get<double>(val));
    }
  }
};

struct readStmt : Statement // Read input into a variable
{
  // Member Variables
  string target;

  // Member Functions
  void print_tree(ostream &os, string prefix)
  {
    ast_line(os, prefix, true, "ReadStmt: " + target);
  }
  void interpret(ostream &out)
  {
    auto it = symbolTable.find(target);
    // if (it == symbolTable.end())
    //   throw runtime_error("readStmt: undeclared variable");

    visit([&](auto &value)
          { cin >> value; }, it->second);
  }
};

struct writeStmt : Statement // Outputs value or a string
{
  // Member Variables
  string content;
  Token type;

  // Member Functions
  void print_tree(ostream &os, string prefix)
  {
    if (type == IDENT)
    {
      ast_line(os, prefix, true, "writeStmt (IDENT): " + content);
    }
    else
    {
      ast_line(os, prefix, true, "writeStmt (STRING): " + content);
    }
  }
  void interpret(ostream &out)
  {
    auto it = symbolTable.find(content);

    if (type == IDENT)
    {
      visit([&out](auto &&value)
            { out << value << endl; }, it->second);
    }
    else
    {
      out << content << endl;
    }
  }
};

struct compoundStmt : Statement // A sequence of statements
{
  // Member Variables
  vector<unique_ptr<Statement>> stmts;

  // Member Functions
  void print_tree(ostream &os, string prefix) // Displays a "pretty" list of children
  {
    for (auto &s : stmts)
      s->print_tree(os, "   " + prefix);
  }
  void interpret(ostream &out) // For s in stmts: s->interpret(out)
  {
    for (auto &s : stmts)
    {
      s->interpret(out);
    }
  }
};

struct Block
{
  // Member Variables
  unique_ptr<compoundStmt> compound;
  void print_tree(ostream &out)
  {
    ast_line(out, "", true, "Block");
    if (!symbolTable.empty())
    {
      ast_line(out, "  ", false, "Symbol Table:");
      for (auto &[id, value] : symbolTable)
      {
        if (holds_alternative<int>(value)) // Check for int
          ast_line(out, "   ", true, id + " := " + to_string(get<int>(value)));
        else
          ast_line(out, "   ", true, id + " := " + to_string(get<double>(value)));
      }
    }
    if (compound)
      compound->print_tree(out, "  "); // Prints the tree for compound
  }
  void interpret(ostream &out)
  {
    if (compound)
      compound->interpret(out);
  }
};

struct Program
{
  string name;
  unique_ptr<Block> block;
  void print_tree(ostream &os)
  {
    cout << "Program\n";
    ast_line(os, "", false, "name: " + name);
    if (block)
      block->print_tree(os);
    else
    {
      ast_line(os, "", true, "Block");
      ast_line(os, "    ", true, "(empty)");
    }
  }
  void interpret(ostream &out)
  {
    if (block)
      block->interpret(out);
  }

  friend ostream &operator<<(ostream &os, unique_ptr<Program> &p)
  {
    if (p)
      p->print_tree(os);
    return os;
  }
};