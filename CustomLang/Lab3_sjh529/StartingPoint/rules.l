/*****************************************************************************
 *   rules.l - Flex Rules for TIPS Language Subset
 *   MSU CSE 4714/6714 Capstone Project (Fall 2025)
 *   Author: Derek Willis
*****************************************************************************/
%option noyywrap nodefault nounput
%option yylineno

%{
#include "lexer.h"
%}

%%
[ \t\r\n]+              
PROGRAM                 { return PROGRAM; }
BEGIN                   { return TOK_BEGIN; }
END                     { return END; }
WRITE                   { return WRITE; }
VAR                     { return VAR; }
INTEGER                 { return INTEGER; }
REAL                    { return REAL; }
READ                    { return READ; }
":="                    { return ASSIGN; }
[0-9]+\.[0-9]+          { return FLOATLIT; }
[0-9]+                  { return INTLIT; }
MOD                     { return MOD; }
"+"                     { return PLUS; }
"-"                     { return MINUS; }
"*"                     { return MULTIPLY; }
"/"                     { return DIVIDE; }
"^^"                    { return CUSTOM_OPER; }
"++"                    { return INCREMENT; }
"--"                    { return DECREMENT; }
[A-Z][A-Z0-9]{0,7}      { return IDENT; }
[A-Z][A-Z0-9]{8,}       { return UNKNOWN; }
:                       { return COLON; }
";"                     { return SEMICOLON; }
"("                     { return OPENPAREN; }
'[^'\n]{0,80}'          { return STRINGLIT; }
'[^'\n]{81,}'           { return UNKNOWN; }
")"                     { return CLOSEPAREN; }
.                       { return UNKNOWN; }
<<EOF>>                 { return TOK_EOF; }

%%
