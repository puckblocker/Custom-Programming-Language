// ============================================================================
//  parser.cpp — Recursive-descent parser
// ----------------------------------------------------------------------------
// MSU CSE 4714/6714 Capstone Project (Fall 2025)
// Author: Derek Willis
// ============================================================================

#include <memory>
#include <stdexcept>
#include <sstream>
#include <string>
#include <set>
#include "lexer.h"
#include "ast.h"
#include "debug.h"
using namespace std;

// Forward Declarations
unique_ptr<Statement> parseWrite();
unique_ptr<Block> parseBlock();
unique_ptr<Program> parseProgram();
void parseDeclaration();
unique_ptr<compoundStmt> parseCompound();
unique_ptr<Statement> parseStatement();
unique_ptr<Statement> parseRead();
unique_ptr<Statement> parseAssign();
unique_ptr<ValueNode> parseTerm();
unique_ptr<ValueNode> parsePrimary();
unique_ptr<ValueNode> parseValue();
unique_ptr<ValueNode> parseFactor();

// -----------------------------------------------------------------------------
// One-token lookahead
// -----------------------------------------------------------------------------
bool havePeek = false;
Token peekTok = 0;
string peekLex;

inline const char *tname(Token t) { return tokName(t); }

Token peek()
{
  if (!havePeek)
  {
    peekTok = yylex();
    if (peekTok == 0)
    {
      peekTok = TOK_EOF;
      peekLex.clear();
    }
    else
    {
      peekLex = yytext ? string(yytext) : string();
    }
    dbg::line(string("peek: ") + tname(peekTok) + (peekLex.empty() ? "" : " [" + peekLex + "]") + " @ line " + to_string(yylineno));
    havePeek = true;
  }
  return peekTok;
}
Token nextTok()
{
  Token t = peek();
  dbg::line(string("consume: ") + tname(t));
  havePeek = false;
  return t;
}
Token expect(Token want, const char *msg)
{
  Token got = nextTok();
  if (got != want)
  {
    dbg::line(string("expect FAIL: wanted ") + tname(want) + ", got " + tname(got));
    ostringstream oss;
    oss << "Parse error (line " << yylineno << "): expected "
        << tname(want) << " — " << msg << ", got " << tname(got)
        << " [" << (yytext ? yytext : "") << "]";
    throw runtime_error(oss.str());
  }
  return got;
}

// TODO: implement parsing functions for each grammar in your language

// -----------------------------------------------------------------------------
// Program → PROGRAM IDENT ';' Block EOF
// -----------------------------------------------------------------------------
unique_ptr<Program> parseProgram()
{
  expect(PROGRAM, "start of program");
  expect(IDENT, "program name");
  string nameLex = peekLex;
  expect(SEMICOLON, "after program name");

  auto p = make_unique<Program>();
  p->name = nameLex;
  p->block = parseBlock();

  expect(TOK_EOF, "at end of file (no trailing tokens after program)");
  return p;
}

unique_ptr<Block> parseBlock()
{
  auto node = make_unique<Block>();
  if (peek() == VAR)
  {
    expect(VAR, "parseBlock: Expected a VAR token");

    while (peek() == IDENT)
    {
      parseDeclaration();
    }
  }
  node->compound = parseCompound();
  return node;
}
unique_ptr<Statement> parseWrite()
{
  expect(WRITE, "parseWrite: Start of a write");
  expect(OPENPAREN, "parseWrite: Must follow WRITE");
  Token Type = peek();
  if (Type != STRINGLIT && Type != IDENT)
  {
    throw runtime_error("parseWrite: Expected a String Lit or an Identifier");
  }
  string contentLex = peekLex;
  expect(Type, "parseWrite: Expected a String Lit or an Identifier");
  expect(CLOSEPAREN, "To close write");
  auto buffer = make_unique<writeStmt>();
  buffer->content = contentLex;
  buffer->type = Type;
  return buffer;
}

void parseDeclaration()
{
  expect(IDENT, "parseDeclaration: Expected an Identifier");
  string idLex = peekLex;
  expect(COLON, "parseDeclaration: Expected a Colon after after Identifier");
  Token Type = peek();
  if (Type != INTEGER && Type != REAL)
  {
    throw runtime_error("parseDeclaration: Expect type to be an integer or real");
  }
  expect(Type, "parseDeclaration: Expected type");
  expect(SEMICOLON, "parseDeclaration: Expected a semicolon");

  if (symbolTable.find(idLex) != symbolTable.end())
  {
    throw runtime_error("parseDeclaration: duplicate");
  }
  if (Type == INTEGER)
  {
    symbolTable[idLex] = 0;
  }
  else
  {
    symbolTable[idLex] = 0.0;
  }
}

unique_ptr<compoundStmt> parseCompound()
{
  expect(TOK_BEGIN, "parseCompound: Expected a Begin Token");
  auto buff = make_unique<compoundStmt>();
  buff->stmts.push_back(parseStatement());
  while (peek() == SEMICOLON)
  {
    expect(SEMICOLON, "parseCompound: Expected a semicolon");
    if (peek() == END)
    {
      break;
    }
    buff->stmts.push_back(parseStatement());
  }
  expect(END, "parseCompound: Expected an End Token");
  return buff;
}

unique_ptr<Statement> parseStatement()
{
  switch (peek())
  {
  case IDENT:
    return parseAssign();
  case TOK_BEGIN:
    return parseCompound();
  case READ:
    return parseRead();
  case WRITE:
    return parseWrite();
  default:
    throw runtime_error("parseStatement: Token Not accepted");
  }
}

// value -> term { (+/-) term }
unique_ptr<ValueNode> parseValue()
{
  auto node = parseTerm();
  while (true)
  {
    Token t = peek();
    if (t == PLUS || t == MINUS)
    {
      Token op = t;
      expect(t, "additive operator (+/-) in value");
      auto rhs = parseTerm();

      auto bin = make_unique<BinaryOp>();
      bin->op = op;
      bin->left = move(node);
      bin->right = move(rhs);
      node = move(bin);
    }
    else
    {
      break;
    }
  }
  return node;
}

// primary -> FLOATLIT | INTLIT | IDENT | ( value )
unique_ptr<ValueNode> parsePrimary()
{
  Token type = peek();
  switch (type)
  {
  case FLOATLIT:
  {
    string vLex = peekLex;
    expect(FLOATLIT, "parsePrimary: Expected a FLOATLIT token");
    auto bin = make_unique<RealLitNode>();
    bin->v = stod(vLex);
    return bin;
  }
  case INTLIT:
  {
    string valLex = peekLex;
    expect(INTLIT, "parsePrimary: Expected a INTLIT token");
    auto bin2 = make_unique<IntLitNode>();
    bin2->v = stoi(valLex);
    return bin2;
  }
  case IDENT:
  {
    string nameLex = peekLex;
    expect(IDENT, "parsePrimary: Expected a IDENT token");
    auto bin3 = make_unique<IdentNode>();
    bin3->name = nameLex;
    return bin3;
  }
  case OPENPAREN:
  {
    expect(OPENPAREN, "parsePrimary: Expected an OPENPAREN");
    auto node = parseValue();
    expect(CLOSEPAREN, "parsePrimary: Expected a CLOSEPAREN");
    return node;
  }
  default:
    throw runtime_error("parsePrimary: Invalid Token");
  }
}

// term -> factor { (*|/|MOD|^^) factor }
unique_ptr<ValueNode> parseTerm()
{
  auto node = parseFactor();
  while (true)
  {
    Token t = peek();
    if (t == MULTIPLY || t == DIVIDE || t == MOD || t == CUSTOM_OPER)
    {
      Token op = t;
      expect(t, "parseTerm: Expected multiple, divide, mod, or exponential");
      auto rhs = parseFactor();

      auto bin = make_unique<BinaryOp>();
      bin->op = op;
      bin->left = move(node);
      bin->right = move(rhs);
      node = move(bin);
    }
    else
    {
      break;
    }
  }
  return node;
}

// factor -> [ ++ | -- | ] primary | primary
unique_ptr<ValueNode> parseFactor()
{
  Token type = peek();
  if (type == INCREMENT || type == DECREMENT)
  {
    expect(type, "parseFactor: Expected an increment or decrement");
    auto node = parsePrimary();
    auto bin = make_unique<UnaryOp>();
    bin->op = type;
    bin->sub = move(node);
    node = move(bin);
    return bin;
  }
  return parsePrimary();
}

unique_ptr<Statement> parseRead()
{
  expect(READ, "parseRead: Expected Read");
  expect(OPENPAREN, "parseRead: Expected Open Parentheses");

  expect(IDENT, "parseRead: Expected Identifier");
  string targetLex = peekLex;
  expect(CLOSEPAREN, "parseRead: Expected Close Parentheses");

  auto node = make_unique<readStmt>();
  node->target = targetLex;
  return node;
}

unique_ptr<Statement> parseAssign()
{
  expect(IDENT, "Expected identifier (name) for assignment");
  string idLex = peekLex;
  expect(ASSIGN, "Expected an assignment (:=) after identifier (name)");

  // Token Type = peek();

  // if (Type != INTLIT && Type != FLOATLIT && Type != IDENT)
  // {
  //   throw runtime_error("parseAssignment: Expected INTLIT, FLOATLIT, IDENT for value");
  // }

  auto valLex = parseValue();
  auto buff = make_unique<assignStmt>();
  buff->id = idLex;
  buff->rhs = move(valLex);

  return buff;
}