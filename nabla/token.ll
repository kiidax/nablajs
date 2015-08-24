/* Nabla JS - A small EMCAScript interpreter with straight-forward implementation.
 * Copyright (C) 2014 Katsuya Iida. All rights reserved.
 */

%{
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <cassert>
#include <cmath>
#include <string>

#include "ast.hh"
#define YY_NO_INPUT
#define YY_INPUT(buf,result,max_size) { result = 0; }
#ifdef _WIN32
#define YY_NO_UNISTD_H 1
#endif
using namespace nabla::internal;
#define YYLTYPE SourceLocation
#include "parser.hh"
extern AstBuilder* yy_builder;
extern SyntaxNode* yy_result;
extern std::string yy_name;
int yyparse();
static std::string yy_string;
static int yy_state_before_comment;
int yycolumn = 1;
static int escape_char(int ch) {
  if (ch == 'n') return '\n';
  if (ch == 'r') return '\r';
  if (ch == 't') return '\t';
  return ch;
}
#define YY_USER_ACTION                              \
  {                                                 \
    yylloc.start.line = yylloc.end.line = yylineno; \
    yylloc.start.column = yycolumn;                 \
    yylloc.end.column = yycolumn + yyleng - 1;      \
    yycolumn += yyleng;                             \
  }
%}

%option noyywrap
%option nounput
%option nostdinit
%option batch
%option never-interactive
%option yylineno

%x ident getset mcomment scomment dstr sstr regex regexcls

%%

<INITIAL,ident,getset>{
[ \t\r]                 ;
[\n]                    { yycolumn = 1; }
"/*"                    { yy_state_before_comment = YYSTATE; BEGIN(mcomment); }
"//"                    { yy_state_before_comment = YYSTATE; BEGIN(scomment); }
}

\"                      BEGIN(dstr);
\'                      BEGIN(sstr);

break                   { return TBREAK; }
do                      { return TDO; }
instanceof              { return TINSTANCEOF; }
typeof                  { return TTYPEOF; }
case                    { return TCASE; }
else                    { return TELSE; }
new                     { return TNEW; }
var                     { return TVAR; }
catch                   { return TCATCH; }
finally                 { return TFINALLY; }
return                  { return TRETURN; }
void                    { return TVOID; }
continue                { return TCONTINUE; }
for                     { return TFOR; }
switch                  { return TSWITCH; }
while                   { return TWHILE; }
debugger                { return TDEBUGGER; }
function                { return TFUNCTION; }
this                    { return TTHIS; }
with                    { return TWITH; }
default                 { return TDEFAULT; }
if                      { return TIF; }
throw                   { return TTHROW; }
delete                  { return TDELETE; }
in                      { return TIN; }
try                     { return TTRY; }

null                    { return TNULL; }
true                    { return TTRUE; }
false                   { return TFALSE; }

<getset>get             { return TGET; }
<getset>set             { return TSET; }

"++"                    { return TINCREMENT; }
"--"                    { return TDECREMENT; }

"<="                    { return TLE; }
">="                    { return TGE; }
"=="                    { return TEQ; }
"!="                    { return TNE; }
"==="                   { return TSTRICTEQ; }
"!=="                   { return TSTRICTNE; }
">>>"                   { return TSHIFTRIGHTLOGICAL; }
"<<"                    { return TSHIFTLEFT; }
">>"                    { return TSHIFTRIGHT; }

"&&"                    { return TLOGAND; }
"||"                    { return TLOGOR; }

"*="                    { return TASSIGNMUL; }
"/="                    { return TASSIGNDIV; }
"%="                    { return TASSIGNMOD; }
"+="                    { return TASSIGNADD; }
"-="                    { return TASSIGNSUB; }
"<<="                   { return TASSIGNSHIFTLEFT; }
">>="                   { return TASSIGNSHIFTRIGHT; }
">>>="                  { return TASSIGNSHIFTRIGHTLOG; }
"&="                    { return TASSIGNAND; }
"^="                    { return TASSIGNXOR; }
"|="                    { return TASSIGNOR; }

[a-zA-Z$_][a-zA-Z0-9$_]* { yylval.strval = new std::string(yytext, yytext + strlen(yytext)); return TIDENTIFIER; }
(0|[1-9][0-9]*)\.([0-9]*([eE][-+]?[0-9]+)?)? { yylval.nval = atof(yytext); return TNUMBER; }
[1-9][0-9]*             { yylval.nval = atoi(yytext); return TNUMBER; }
0[xX][0-9a-zA-Z]+       { yylval.nval = strtol(yytext + 2, nullptr, 16); return TNUMBER; }
0                       { yylval.nval = 0; return TNUMBER; }
[\.+\-*/%=,;:\{\}\(\)\[\]<>!~&^|?:] { return yytext[0]; }
.                       { printf("Unknown token!\n"); yyterminate(); }

<ident,getset>{
\"               BEGIN(dstr);
\'               BEGIN(sstr);
[a-zA-Z$_][a-zA-Z0-9$_]* { yylval.strval = new std::string(yytext, yytext + strlen(yytext)); BEGIN(INITIAL); return TIDENTIFIER; }
[\.+\-*/%=,;:\{\}\(\)\[\]<>!~&^|?:] { BEGIN(INITIAL); return yytext[0]; }
(0|[1-9][0-9]*)\.([0-9]*([eE][-+]?[0-9]+)?)? { yylval.nval = atof(yytext); BEGIN(INITIAL); return TNUMBER; }
[1-9][0-9]*      { yylval.nval = atoi(yytext); BEGIN(INITIAL); return TNUMBER; }
0[xX][0-9a-zA-Z]+ { yylval.nval = strtol(yytext + 2, nullptr, 16); BEGIN(INITIAL); return TNUMBER; }
0                { yylval.nval = 0; BEGIN(INITIAL); return TNUMBER; }
}

<mcomment>[^*]*         ;
<mcomment>"*"+[^*/]*    ;
<mcomment>"*"+"/"       { BEGIN(yy_state_before_comment); }

<scomment>[^\n]*\n      { BEGIN(yy_state_before_comment); yycolumn = 1; }

<dstr>[^\\\n\r\"]+      { yy_string += yytext; }
<dstr>\\[^\n\r]         { yy_string += escape_char(yytext[1]); }
<dstr>\"                { BEGIN(INITIAL); yylval.strval = new std::string(yy_string); yy_string.clear(); return TSTRING; }

<sstr>[^\\\n\r\']+      { yy_string += yytext; }
<sstr>\\[^\n\r]         { yy_string += escape_char(yytext[1]); }
<sstr>\'                { BEGIN(INITIAL); yylval.strval = new std::string(yy_string); yy_string.clear(); return TSTRING; }

<regex>[^\\/[]+         { yy_string += std::string(yytext); }
<regex>\\.              { yy_string += std::string(yytext); }
<regex>\[               { yy_string += std::string(yytext); BEGIN(regexcls); }
<regex>\/               { yylval.strval = new std::string(yy_string); yy_string.clear(); BEGIN(INITIAL); return TREGEX; }

<regexcls>[^]\\]+       { yy_string += std::string(yytext); }
<regexcls>\\.           { yy_string += std::string(yytext); }
<regexcls>\]            { yy_string += std::string(yytext); BEGIN(regex); }

%%

void yy_begin_regex() {
    BEGIN(regex);
}

void yy_begin_ident() {
    BEGIN(ident);
}

void yy_begin_getset() {
    BEGIN(getset);
}

// Converts UTF-16 to CESU-8 (variant of UTF-8).
static void yy_make_buffer_from_u16(const char16_t* s, size_t n, char*& bufbase, yy_size_t& bufsize) {
  // Count the number of bytes needed for CESU-8.
  const char16_t* sp = s;
  const char16_t* send = s + n;
  size_t dsize = 0;
  while (sp < send) {
    int ch = *sp++;
    if (ch < 0x80) dsize++;
    else if (ch < 0x800) dsize += 2;
    else dsize += 3;
  }

  // C++ char can be singed or unsigned.
  char* ds = new char[dsize + 2];

  // Then converts.
  sp = s;
  char *dp = ds;
  while (sp < send) {
    int ch = *sp++;
    if (ch < 0x80) *dp++ = ch;
    else if (ch < 0x800) {
      *dp++ = ((ch >>  6) & 0x1f) | 0xc0;
      *dp++ = ((ch      ) & 0x3f) | 0x80;
    } else {
      *dp++ = ((ch >> 12) & 0x0f) | 0xe0;
      *dp++ = ((ch >>  6) & 0x3f) | 0x80;
      *dp++ = ((ch      ) & 0x3f) | 0x80;
    }
  }

  // Flex needs two additional bytes.
  *dp++ = YY_END_OF_BUFFER_CHAR;
  *dp++ = YY_END_OF_BUFFER_CHAR;

  bufbase = ds;
  bufsize = dsize + 2;
}

Program* yy_parse_string(AstBuilder* builder, const char16_t* s, size_t n, const char16_t* name) {
  yy_result = nullptr;
  std::u16string u16name(name);
  yy_name = std::string(u16name.begin(), u16name.end());
  yy_string.clear();
  char* bufbase;
  yy_size_t bufsize;
  yy_make_buffer_from_u16(s, n, bufbase, bufsize);
  YY_BUFFER_STATE bs = yy_scan_buffer(bufbase, bufsize);
  assert(bs);
  // yydebug = 1;
  yy_builder = builder;
  int res = yyparse();
  if (res == 1) {
    yy_result = nullptr;
  } else if (res == 2) {
    exit(-1);
  }
  yy_builder = nullptr;
  yy_delete_buffer(bs);
  yylex_destroy();
  delete bufbase;
  Program* result = static_cast<Program*>(yy_result);
  yy_result = nullptr;
  yy_name.clear();
  yy_string.clear();
  return result;
}
