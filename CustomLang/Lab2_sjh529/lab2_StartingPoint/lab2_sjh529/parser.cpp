// ============================================================================
// parser.cpp - Recursive descent parser that BUILDS the AST 
// ----------------------------------------------------------------------------
// Author: Derek Willis (Fall 2025)
// ============================================================================

#include "parser.h"
#include "lexer.h"
#include "debug.h"
#include <stdexcept>
#include <string>
using namespace std;

// Provided by the lexer (Flex)
extern int yylex();
extern char* yytext;
extern FILE* yyin;


// Single-token lookahead
static int lookahead = 0;

// Advance to the next token
static void next() {
    lookahead = yylex();
    if (gDebug) {
        if (lookahead == TOK_EOF) dbg("next: TOK_EOF");
        else dbg(string("next: ") + tokenName(lookahead) + " (" + yytext + ")");
    }
}

// Match a specific token and return its lexeme, or throw with the given message.
static string expect(int tok, const char* msgIfMismatch) {
    if (lookahead == tok) {
        string lex = yytext;
        if (gDebug) dbg(string("match ") + tokenName(tok) + " (" + lex + ")");
        next();
        return lex;
    }
    if (gDebug) {
        dbg(string("mismatch: got ") + tokenName(lookahead) +
            ", expected " + tokenName(tok));
    }
    throw runtime_error(msgIfMismatch);
}


// TODO: define and implement parseSentence, parseNounPhrase, parseAdjectivePhrase,
//        parseVerbPhrase

unique_ptr<NounPhrase> parseNounPhrase()
{
    dbgLine("Enter <noun phrase");
    DebugIndent _scope;
    // FIRST check
    if(lookahead != ARTICLE && lookahead != POSSESSIVE)
    {
        throw runtime_error("<noun phrase> did not start with an article or possessive");
    }
    auto node = make_unique<NounPhrase>();
    node->adj = parseAdjectivePhrase();
    node->nounLexeme = expect(NOUN, "<noun phrase> did not have a noun.");
    return node;
}

unique_ptr<VerbPhrase> parseVerbPhrase()
{
    dbgLine("enter <verb phrase>");
    DebugIndent _scope;
    // FIRST check
    if(lookahead != VERB && lookahead != ADVERB)
    {
        throw runtime_error("<verb phrase> did not start with a verb or an adverb.");
    }
    auto node = make_unique<VerbPhrase>();
    while (lookahead == ADVERB)
    {
        node->adverbs.push_back(expect(ADVERB, "..."));
    }
    node->verbLexeme = expect(VERB, "<verb phrase> did not start with a verb or an adverb");
    return node;
}

unique_ptr<Sentence> parseSentence()
{
    dbgLine("enter <sentence>");
    DebugIndent _scope;
    // FIRST check
    if(lookahead != ARTICLE && lookahead != POSSESSIVE)
    {
        throw runtime_error("<sentence> did not start with an article or possessive.");
    }
    auto node = make_unique<Sentence>();
    node->subjectNP = parseNounPhrase();
    node->verbP = parseVerbPhrase();
    node->objectNP = parseNounPhrase();
    return node;
}

// <adjective phrase> → (ARTICLE | POSSESSIVE) ADJECTIVE
// Errors:
// "<adjective phrase> did not start with an article or possessive."
// "<adjective phrase> did not have an adjective."
unique_ptr<AdjectivePhrase> parseAdjectivePhrase() {
dbgLine("enter <adjective phrase>");
DebugIndent _scope;
// FIRST check
if (lookahead != ARTICLE && lookahead != POSSESSIVE) {
throw runtime_error(
"<adjective phrase> did not start with an article or possessive."
);
}
auto node = make_unique<AdjectivePhrase>();
// Determiner (ARTICLE | POSSESSIVE)
if (lookahead == ARTICLE) {
node->detType = AdjectivePhrase::DetType::Article;
node->detLexeme = expect(ARTICLE,
"<adjective phrase> did not start with an article or possessive.");
} else { // POSSESSIVE
node->detType = AdjectivePhrase::DetType::Possessive;
node->detLexeme = expect(POSSESSIVE,
"<adjective phrase> did not start with an article or possessive.");
}
// ADJECTIVE
node->adjLexeme = expect(ADJECTIVE,
"<adjective phrase> did not have an adjective.");
return node;
}


// Entry point: initialize, parse, enforce EOF
unique_ptr<Sentence> parseStart() {
    next();                      // prime lookahead
    auto root = parseSentence(); // may throw on first syntax error
    if (lookahead != TOK_EOF) {
        throw runtime_error("Extra input after complete sentence.");
    }
    return root;
}
