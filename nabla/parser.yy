/* Nabla JS - A small EMCAScript interpreter with straight-forward implementation.
 * Copyright (C) 2014 Katsuya Iida. All rights reserved.
 */

%{
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <cmath>
#include <cstdio>
#include <string>
#include <iostream>

#include "ast.hh"
#include "debug.hh"
#define YYDEBUG 1
int yylex(void);
void yyerror(const char *s);
using namespace nabla::internal;
extern int yylineno;
AstBuilder* yy_builder;
SyntaxNode* yy_result;
std::string yy_name;
void yy_begin_regex();
void yy_begin_ident();
void yy_begin_getset();
#define YYLTYPE SourceLocation
#define YYLLOC_DEFAULT(Cur, Rhs, N)                       \
  do                                                      \
    if (N) {                                              \
      (Cur).start.line   = YYRHSLOC(Rhs, 1).start.line;   \
      (Cur).start.column = YYRHSLOC(Rhs, 1).start.column; \
      (Cur).end.line     = YYRHSLOC(Rhs, N).end.line;     \
      (Cur).end.column   = YYRHSLOC(Rhs, N).end.column;   \
    } else {                                              \
      (Cur).start.line   = (Cur).end.line   =             \
        YYRHSLOC(Rhs, 0).end.line;                        \
      (Cur).start.column = (Cur).end.column =             \
        YYRHSLOC(Rhs, 0).end.column;                      \
    }                                                     \
  while (0)
#define YYLOC_FIX(x, y) ((x)->loc = (y))
#define YYSTR(x) (yy_builder->GetStringConstant(*(x)))

static SequenceExpression* concat_expression(const SourceLocation& loc, Expression* x, Expression* y) {
  SequenceExpression* expr;
  if (x->type == SyntaxNode::kSequenceExpression) {
    expr = reinterpret_cast<SequenceExpression*>(x);
    expr->expressions.push_back(y);
    expr->loc = loc;
  } else {
    expr = new SequenceExpression(loc, nullptr);
    expr->expressions.push_back(x);
    expr->expressions.push_back(y);
  }
  return expr;
}

static std::vector<SwitchCase*>* concat_switch_case(std::vector<SwitchCase*>* a, SwitchCase* b, std::vector<SwitchCase*>* c) {
  if (a) {
    a->push_back(b);
    if (c) a->insert(a->end(), c->begin(), c->end());
    return a;
  } else {
    if (c) {
      c->insert(c->begin(), b);
      return c;
    }
    std::vector<SwitchCase*>* d = new std::vector<SwitchCase*>();
    d->push_back(b);
    return d;
  }
}

%}

%union {
    double nval;
    std::string* strval;
    SyntaxNode::AssignmentOperator asgnop;           
    Expression* exprval;
    CallExpression* callval;
    ObjectExpression* objval;
    Identifier* idval;                    
    std::vector<Identifier*>* idlist;
    PropertyNode* propval;
    std::vector<PropertyNode*>* proplist;
    FunctionExpression* funcval;
    std::vector<Expression*>* exprlist;
    Statement* stmtval;
    std::vector<Statement*>* stmtlist;
    VariableDeclarator* declval;
    std::vector<VariableDeclarator*>* decllist;
    SwitchCase* caseval;
    std::vector<SwitchCase*>* caselist;
    BlockStatement* blkval;
    CatchClause* catchval;
    Program* progval;
}

%token  <nval>          TNUMBER
%token  <strval>        TSTRING TIDENTIFIER
%token  <strval>        TREGEX

%token TENDL
%token TTHIS
%token TINCREMENT TDECREMENT
%token TNEW TDELETE TVOID TTYPEOF
%token TSHIFTLEFT TSHIFTRIGHT TSHIFTRIGHTLOGICAL
%token TGE TLE TINSTANCEOF TIN
%token TEQ TNE TSTRICTEQ TSTRICTNE
%token TIF TELSE
%token TDO TWHILE TFOR
%token TFUNCTION
%token TVAR
%token TCONTINUE TBREAK TRETURN TWITH TSWITCH TCASE TDEFAULT
%token TTRY TCATCH TFINALLY TTHROW TDEBUGGER

%token TNULL TTRUE TFALSE

%token TLOGAND TLOGOR

%token TGET TSET

// Assignment Operators

%token TASSIGNMUL TASSIGNDIV TASSIGNMOD TASSIGNADD TASSIGNSUB
%token TASSIGNSHIFTLEFT TASSIGNSHIFTRIGHT TASSIGNSHIFTRIGHTLOG
%token TASSIGNAND TASSIGNXOR TASSIGNOR

%left '?' ':'
%left TLOGOR
%left TLOGAND
%left '|'
%left '^'
%left '&'
%left TEQ TNE TSTRICTEQ TSTRICTNE
%left '<' '>' TLE TGE TINSTANCEOF TIN
%left TSHIFTLEFT TSHIFTRIGHT TSHIFTRIGHTLOGICAL
%left '+' '-'
%left '*' '/' '%'
%right TDELETE TVOID TTYPEOF TINCREMENT TDECREMENT TUPLUS TUMINUS '~' '!'
%right '(' ')' TELSE                        

%type   <idval>         Identifier IdentifierOpt
%type   <exprval>       IdentifierName

%type   <exprval>       Literal NullLiteral BooleanLiteral NumericLiteral
%type   <exprval>       StringLiteral RegularExpressionLiteral
%type   <strval>        RegularExpressionBody
%type   <strval>        RegularExpressionFlags
%type   <exprval>       PrimaryExpression PrimaryExpressionNoFB
%type   <exprval>       ArrayLiteral
%type   <exprlist>      ElementList Elision ElisionOpt
%type   <exprval>       ObjectLiteral
%type   <proplist>      PropertyNameAndValueList
%type   <propval>       PropertyAssignment
%type   <exprval>       PropertyName
%type   <idlist>        PropertySetParameterList
%type   <idlist>        PropertySetParameterListOpt
%type   <exprval>       MemberExpression MemberExpressionNoFB
%type   <exprval>       NewExpression NewExpressionNoFB
%type   <exprval>       CallExpression CallExpressionNoFB
%type   <exprlist>      ArgumentList Arguments
%type   <exprval>       LeftHandSideExpression LeftHandSideExpressionNoFB
%type   <exprval>       ShiftExpression ShiftExpressionNoFB
%type   <exprval>       RightHandSideExpression RightHandSideExpressionNoIn
%type   <exprval>       RightHandSideExpressionNoFB
%type   <exprval>       ConditionalExpression ConditionalExpressionNoIn
%type   <exprval>       ConditionalExpressionNoFB
%type   <exprval>       AssignmentExpression AssignmentExpressionNoIn
%type   <exprval>       AssignmentExpressionNoFB
%type   <asgnop>        AssignmentOperator
%type   <exprval>       Expression ExpressionOpt
%type   <exprval>       ExpressionNoIn ExpressionNoInOpt
%type   <exprval>       ExpressionNoFB

%type   <stmtval>       Statement
%type   <blkval>        Block
%type   <stmtlist>      StatementList StatementListOpt
%type   <stmtval>       VariableStatement
%type   <decllist>      VariableDeclarationList VariableDeclarationListNoIn
%type   <declval>       VariableDeclaration VariableDeclarationNoIn 
%type   <exprval>       InitialiserOpt InitialiserNoInOpt 
%type   <stmtval>       EmptyStatement
%type   <stmtval>       ExpressionStatement
%type   <stmtval>       IfStatement 
%type   <stmtval>       IterationStatement
%type   <stmtval>       ContinueStatement
%type   <stmtval>       BreakStatement
%type   <stmtval>       ReturnStatement
%type   <stmtval>       WithStatement
%type   <stmtval>       SwitchStatement
%type   <caselist>      CaseBlock CaseClauses CaseClausesOpt
%type   <caseval>       CaseClause DefaultClause
%type   <stmtval>       LabelledStatement
%type   <stmtval>       ThrowStatement
%type   <stmtval>       TryStatement
%type   <catchval>      Catch
%type   <blkval>        Finally
%type   <stmtval>       DebuggerStatement

%type   <stmtval>       FunctionDeclaration
%type   <exprval>       FunctionExpression
%type   <idlist>        FormalParameterList FormalParameterListOpt
%type   <blkval>        FunctionBody
%type   <progval>       Program
%type   <stmtlist>      SourceElements SourceElementsOpt
%type   <stmtval>       SourceElement

%start Program

%%

// A.1 Lexical Grammar

Identifier:     TIDENTIFIER
                { $$ = new Identifier(@$, YYSTR($1)); delete $1; }
        ;

IdentifierOpt:  { $$ = nullptr; }
        |       Identifier
        ;

IdentifierName: TIDENTIFIER
                { $$ = new Identifier(@$, YYSTR($1)); delete $1; }
        ;

Literal:        NullLiteral
        |       BooleanLiteral
        |       NumericLiteral
        |       StringLiteral
        |       RegularExpressionLiteral
        ;

NullLiteral:    TNULL { $$ = new NullLiteral(@$); }     
        ;

BooleanLiteral: TTRUE { $$ = new BooleanLiteral(@$, true); }
        |       TFALSE { $$ = new BooleanLiteral(@$, false); }
        ;

NumericLiteral: TNUMBER { $$ = new NumberLiteral(@$, $1); }
        ;

StringLiteral:  TSTRING { $$ = new StringLiteral(@$, YYSTR($1)); delete $1; }
        ;

RegularExpressionStart:
                '/' { yy_begin_regex(); }
        ;

RegularExpressionLiteral:
                RegularExpressionStart RegularExpressionBody RegularExpressionFlags
                { $$ = new RegExpLiteral(@$, *$2, *$3); delete $2; delete $3; }
        ;

RegularExpressionBody:
                TREGEX
        ;

RegularExpressionFlags:
                { $$ = new std::string(); }
        |       TIDENTIFIER
        ;

// A.3 Expressions

PrimaryExpression:
                TTHIS { $$ = new ThisExpression(@$); }
        |       Identifier { $$ = $1; /* cast */ }
        |       Literal
        |       ArrayLiteral
        |       ObjectLiteral
        |       '(' Expression ')' { $$ = $2; }
        ;

PrimaryExpressionNoFB:
                TTHIS { $$ = new ThisExpression(@$); }
        |       Identifier { $$ = $1; /* cast */ }
        |       Literal
        |       ArrayLiteral
        |       '(' Expression ')' { $$ = $2; }
        ;

ArrayLiteral:   '[' ElisionOpt ']'
                { $$ = new ArrayExpression(@$, $2); }
        |       '[' ElementList ']'
                { $$ = new ArrayExpression(@$, $2); }
        |       '[' ElementList ',' ElisionOpt ']'
                { $$ = new ArrayExpression(@$, $2); }
        ;

ElementList:    ElisionOpt AssignmentExpression
                { $$ = $1 ? $1 : new std::vector<Expression*>(); $$->push_back($2); }
        |       ElementList ',' ElisionOpt AssignmentExpression
                { $$ = $1; $$->push_back($4); }
        ;

Elision:        ','
                { $$ = new std::vector<Expression*>(); $$->push_back(nullptr); }
        |       Elision ',' { $$ = $1; $$->push_back(nullptr); }
        ;

ElisionOpt:
                { $$ = nullptr; }
        |       Elision
        ;

ObjectLiteralBegin:
                '{' { yy_begin_getset(); }
        ;

ObjectLiteralComma:
                ',' { yy_begin_getset(); }
        ;

ObjectLiteral:  ObjectLiteralBegin '}'
                { $$ = new ObjectExpression(@$, nullptr); }
        |       ObjectLiteralBegin PropertyNameAndValueList '}'
                { $$ = new ObjectExpression(@$, $2); }
        |       ObjectLiteralBegin PropertyNameAndValueList ObjectLiteralComma '}'
                { $$ = new ObjectExpression(@$, $2); }
        ;

PropertyNameAndValueList:
                PropertyAssignment
                { $$ = new std::vector<PropertyNode*>(); $$->push_back($1); }
        |       PropertyNameAndValueList ObjectLiteralComma PropertyAssignment
                { $$ = $1; $$->push_back($3); }
        ;

PropertyAssignment:
                PropertyName ':' AssignmentExpression
                { $$ = new PropertyNode(@$, $1, $3, SyntaxNode::kPropertyInit); }
        |       GetBegin ':' AssignmentExpression
                {
                  int strid = yy_builder->GetStringConstant("get");
                  Identifier* ident = new Identifier(@$, strid);
                  $$ = new PropertyNode(@$, ident, $3, SyntaxNode::kPropertyInit);
                }
        |       SetBegin ':' AssignmentExpression
                {
                  int strid = yy_builder->GetStringConstant("set");
                  Identifier* ident = new Identifier(@$, strid);
                  $$ = new PropertyNode(@$, ident, $3, SyntaxNode::kPropertyInit);
                }
        |       GetBegin PropertyName '(' ')' '{' FunctionBody '}'
                {
                  FunctionNode* func_node = new FunctionNode(@$, nullptr, nullptr, $6);
                  FunctionExpression* func_expr = new FunctionExpression(@$, func_node);
                  $$ = new PropertyNode(@$, $2, func_expr, SyntaxNode::kPropertyGet);
                }
        |       SetBegin PropertyName '(' PropertySetParameterListOpt ')' '{' FunctionBody '}'
                {
                  FunctionNode* func_node = new FunctionNode(@$, nullptr, $4, $7);
                  FunctionExpression* func_expr = new FunctionExpression(@$, func_node);
                  $$ = new PropertyNode(@$, $2, func_expr, SyntaxNode::kPropertySet);
                }
        ;

GetBegin:       TGET { yy_begin_ident(); }
        ;

SetBegin:       TSET { yy_begin_ident(); }
        ;

PropertyName:   IdentifierName
        |       StringLiteral
        |       NumericLiteral
        ;

PropertySetParameterList:
                Identifier { $$ = new std::vector<Identifier*>(); $$->push_back($1); }
        ;

PropertySetParameterListOpt:
                { $$ = nullptr; } // ECMA-262 doesn't allow this.
        |       PropertySetParameterList
        ;

MemberExpression:
                PrimaryExpression
        |       FunctionExpression
        |       MemberExpression '[' Expression ']'
                { $$ = new MemberExpression(@$, $1, $3, true); }
        |       MemberExpression '.' { yy_begin_ident(); } IdentifierName
                { $$ = new MemberExpression(@$, $1, $4, false); }
        |       TNEW MemberExpression Arguments
                { $$ = new NewExpression(@$, $2, $3); }
        ;

MemberExpressionNoFB:
                PrimaryExpressionNoFB
        |       MemberExpressionNoFB '[' Expression ']'
                { $$ = new MemberExpression(@$, $1, $3, true); }
        |       MemberExpressionNoFB '.' { yy_begin_ident(); } IdentifierName
                { $$ = new MemberExpression(@$, $1, $4, false); }
        |       TNEW MemberExpression Arguments
                { $$ = new NewExpression(@$, $2, $3); }
        ;

NewExpression:  MemberExpression
        |       TNEW NewExpression
                { $$ = new NewExpression(@$, $2, nullptr); }
        ;

NewExpressionNoFB:
                MemberExpressionNoFB
        |       TNEW NewExpression
                { $$ = new NewExpression(@$, $2, nullptr); }
        ;

CallExpression: MemberExpression Arguments
                { $$ = new CallExpression(@$, $1, $2); }
        |       CallExpression Arguments
                { $$ = new CallExpression(@$, $1, $2); }
        |       CallExpression '[' Expression ']'
                { $$ = new MemberExpression(@$, $1, $3, true); }
        |       CallExpression '.' { yy_begin_ident(); } IdentifierName
                { $$ = new MemberExpression(@$, $1, $4, false); }
        ;

CallExpressionNoFB:
                MemberExpressionNoFB Arguments
                { $$ = new CallExpression(@$, $1, $2); }
        |       CallExpressionNoFB Arguments
                { $$ = new CallExpression(@$, $1, $2); }
        |       CallExpressionNoFB '[' Expression ']'
                { $$ = new MemberExpression(@$, $1, $3, true); }
        |       CallExpressionNoFB '.' { yy_begin_ident(); } IdentifierName
                { $$ = new MemberExpression(@$, $1, $4, false); }
        ;

Arguments:      '(' ')' { $$ = nullptr; }
        |       '(' ArgumentList ')' { $$ = $2; }
        ;

ArgumentList:   AssignmentExpression
                { $$ = new std::vector<Expression*>(); $$->push_back($1); }
        |       ArgumentList ',' AssignmentExpression
                { $$ = $1; $$->push_back($3); }
        ;

LeftHandSideExpression:
                NewExpression
        |       CallExpression
        ;

LeftHandSideExpressionNoFB:
                NewExpressionNoFB
        |       CallExpressionNoFB
        ;

ShiftExpression:
                LeftHandSideExpression
        |       TDELETE ShiftExpression
                { $$ = new UnaryExpression(@$, SyntaxNode::kUnaryDelete, true, $2); }
        |       TVOID ShiftExpression
                { $$ = new UnaryExpression(@$, SyntaxNode::kUnaryVoid, true, $2); }
        |       TTYPEOF ShiftExpression
                { $$ = new UnaryExpression(@$, SyntaxNode::kUnaryTypeOf, true, $2); }
        |       TINCREMENT ShiftExpression
                { $$ = new UpdateExpression(@$, SyntaxNode::kUpdateIncrement, $2, true); }
        |       TDECREMENT ShiftExpression
                { $$ = new UpdateExpression(@$, SyntaxNode::kUpdateDecrement, $2, true); }
        |       '+' ShiftExpression %prec TUPLUS
                { $$ = new UnaryExpression(@$, SyntaxNode::kUnaryPositive, true, $2); }
        |       '-' ShiftExpression %prec TUMINUS
                { $$ = new UnaryExpression(@$, SyntaxNode::kUnaryNegative, true, $2); }
        |       '~' ShiftExpression
                { $$ = new UnaryExpression(@$, SyntaxNode::kUnaryBitwiseNot, true, $2); }
        |       '!' ShiftExpression
                { $$ = new UnaryExpression(@$, SyntaxNode::kUnaryLogicalNot, true, $2); }
        |       ShiftExpression TINCREMENT
                { $$ = new UpdateExpression(@$, SyntaxNode::kUpdateIncrement, $1, false); }
        |       ShiftExpression TDECREMENT
                { $$ = new UpdateExpression(@$, SyntaxNode::kUpdateDecrement, $1, false); }
        |       ShiftExpression '+' ShiftExpression
                { $$ = new BinaryExpression(@$, SyntaxNode::kBinaryAddition, $1, $3); }
        |       ShiftExpression '-' ShiftExpression
                { $$ = new BinaryExpression(@$, SyntaxNode::kBinarySubtraction, $1, $3); }
        |       ShiftExpression '*' ShiftExpression
                { $$ = new BinaryExpression(@$, SyntaxNode::kBinaryMultiplication, $1, $3); }
        |       ShiftExpression '/' ShiftExpression
                { $$ = new BinaryExpression(@$, SyntaxNode::kBinaryDivision, $1, $3); }
        |       ShiftExpression '%' ShiftExpression
                { $$ = new BinaryExpression(@$, SyntaxNode::kBinaryRemainder, $1, $3); }
        |       ShiftExpression TSHIFTLEFT ShiftExpression
                { $$ = new BinaryExpression(@$, SyntaxNode::kBinaryLeftShift, $1, $3); }
        |       ShiftExpression TSHIFTRIGHT ShiftExpression
                { $$ = new BinaryExpression(@$, SyntaxNode::kBinarySignedRightShift, $1, $3); }
        |       ShiftExpression TSHIFTRIGHTLOGICAL ShiftExpression
                { $$ = new BinaryExpression(@$, SyntaxNode::kBinaryUnsignedRightShift, $1, $3); }
        ;

ShiftExpressionNoFB:
                LeftHandSideExpressionNoFB
        |       TDELETE ShiftExpression
                { $$ = new UnaryExpression(@$, SyntaxNode::kUnaryDelete, true, $2); }
        |       TVOID ShiftExpression
                { $$ = new UnaryExpression(@$, SyntaxNode::kUnaryVoid, true, $2); }
        |       TTYPEOF ShiftExpression
                { $$ = new UnaryExpression(@$, SyntaxNode::kUnaryTypeOf, true, $2); }
        |       TINCREMENT ShiftExpression
                { $$ = new UpdateExpression(@$, SyntaxNode::kUpdateIncrement, $2, true); }
        |       TDECREMENT ShiftExpression
                { $$ = new UpdateExpression(@$, SyntaxNode::kUpdateDecrement, $2, true); }
        |       '+' ShiftExpression %prec TUPLUS
                { $$ = new UnaryExpression(@$, SyntaxNode::kUnaryPositive, true, $2); }
        |       '-' ShiftExpression %prec TUMINUS
                { $$ = new UnaryExpression(@$, SyntaxNode::kUnaryNegative, true, $2); }
        |       '~' ShiftExpression
                { $$ = new UnaryExpression(@$, SyntaxNode::kUnaryBitwiseNot, true, $2); }
        |       '!' ShiftExpression
                { $$ = new UnaryExpression(@$, SyntaxNode::kUnaryLogicalNot, true, $2); }
        |       ShiftExpressionNoFB TINCREMENT
                { $$ = new UpdateExpression(@$, SyntaxNode::kUpdateIncrement, $1, false); }
        |       ShiftExpressionNoFB TDECREMENT
                { $$ = new UpdateExpression(@$, SyntaxNode::kUpdateDecrement, $1, false); }
        |       ShiftExpressionNoFB '+' ShiftExpression
                { $$ = new BinaryExpression(@$, SyntaxNode::kBinaryAddition, $1, $3); }
        |       ShiftExpressionNoFB '-' ShiftExpression
                { $$ = new BinaryExpression(@$, SyntaxNode::kBinarySubtraction, $1, $3); }
        |       ShiftExpressionNoFB '*' ShiftExpression
                { $$ = new BinaryExpression(@$, SyntaxNode::kBinaryMultiplication, $1, $3); }
        |       ShiftExpressionNoFB '/' ShiftExpression
                { $$ = new BinaryExpression(@$, SyntaxNode::kBinaryDivision, $1, $3); }
        |       ShiftExpressionNoFB '%' ShiftExpression
                { $$ = new BinaryExpression(@$, SyntaxNode::kBinaryRemainder, $1, $3); }
        |       ShiftExpressionNoFB TSHIFTLEFT ShiftExpression
                { $$ = new BinaryExpression(@$, SyntaxNode::kBinaryLeftShift, $1, $3); }
        |       ShiftExpressionNoFB TSHIFTRIGHT ShiftExpression
                { $$ = new BinaryExpression(@$, SyntaxNode::kBinarySignedRightShift, $1, $3); }
        |       ShiftExpressionNoFB TSHIFTRIGHTLOGICAL ShiftExpression
                { $$ = new BinaryExpression(@$, SyntaxNode::kBinaryUnsignedRightShift, $1, $3); }
        ;

RightHandSideExpression:
                ShiftExpression
        |       RightHandSideExpression '<' RightHandSideExpression
                { $$ = new BinaryExpression(@$, SyntaxNode::kBinaryLessThan, $1, $3); }
        |       RightHandSideExpression '>' RightHandSideExpression
                { $$ = new BinaryExpression(@$, SyntaxNode::kBinaryGreaterThan, $1, $3); }
        |       RightHandSideExpression TLE RightHandSideExpression
                { $$ = new BinaryExpression(@$, SyntaxNode::kBinaryLessThanOrEqual, $1, $3); }
        |       RightHandSideExpression TGE RightHandSideExpression
                { $$ = new BinaryExpression(@$, SyntaxNode::kBinaryGreaterThanOrEqual, $1, $3); }
        |       RightHandSideExpression TINSTANCEOF RightHandSideExpression
                { $$ = new BinaryExpression(@$, SyntaxNode::kBinaryInstanceOf, $1, $3); }
        |       RightHandSideExpression TIN RightHandSideExpression
                { $$ = new BinaryExpression(@$, SyntaxNode::kBinaryIn, $1, $3); }
        |       RightHandSideExpression TEQ RightHandSideExpression
                { $$ = new BinaryExpression(@$, SyntaxNode::kBinaryEquals, $1, $3); }
        |       RightHandSideExpression TNE RightHandSideExpression
                { $$ = new BinaryExpression(@$, SyntaxNode::kBinaryDoesNotEqual, $1, $3); }
        |       RightHandSideExpression TSTRICTEQ RightHandSideExpression
                { $$ = new BinaryExpression(@$, SyntaxNode::kBinaryStrictEquals, $1, $3); }
        |       RightHandSideExpression TSTRICTNE RightHandSideExpression
                { $$ = new BinaryExpression(@$, SyntaxNode::kBinaryStrictDoesNotEqual, $1, $3); }
        |       RightHandSideExpression '&' RightHandSideExpression
                { $$ = new BinaryExpression(@$, SyntaxNode::kBinaryBitwiseAnd, $1, $3); }
        |       RightHandSideExpression '^' RightHandSideExpression
                { $$ = new BinaryExpression(@$, SyntaxNode::kBinaryBitwiseXor, $1, $3); }
        |       RightHandSideExpression '|' RightHandSideExpression
                { $$ = new BinaryExpression(@$, SyntaxNode::kBinaryBitwiseOr, $1, $3); }
        |       RightHandSideExpression TLOGAND RightHandSideExpression
                { $$ = new LogicalExpression(@$, SyntaxNode::kLogicalAnd, $1, $3); }
        |       RightHandSideExpression TLOGOR RightHandSideExpression
                { $$ = new LogicalExpression(@$, SyntaxNode::kLogicalOr, $1, $3); }
        ;

RightHandSideExpressionNoIn:
                ShiftExpression
        |       RightHandSideExpressionNoIn '<' RightHandSideExpressionNoIn
                { $$ = new BinaryExpression(@$, SyntaxNode::kBinaryLessThan, $1, $3); }
        |       RightHandSideExpressionNoIn '>' RightHandSideExpressionNoIn
                { $$ = new BinaryExpression(@$, SyntaxNode::kBinaryGreaterThan, $1, $3); }
        |       RightHandSideExpressionNoIn TLE RightHandSideExpressionNoIn
                { $$ = new BinaryExpression(@$, SyntaxNode::kBinaryLessThanOrEqual, $1, $3); }
        |       RightHandSideExpressionNoIn TGE RightHandSideExpressionNoIn
                { $$ = new BinaryExpression(@$, SyntaxNode::kBinaryGreaterThanOrEqual, $1, $3); }
        |       RightHandSideExpressionNoIn TINSTANCEOF RightHandSideExpressionNoIn
                { $$ = new BinaryExpression(@$, SyntaxNode::kBinaryInstanceOf, $1, $3); }
        |       RightHandSideExpressionNoIn TEQ RightHandSideExpressionNoIn
                { $$ = new BinaryExpression(@$, SyntaxNode::kBinaryEquals, $1, $3); }
        |       RightHandSideExpressionNoIn TNE RightHandSideExpressionNoIn
                { $$ = new BinaryExpression(@$, SyntaxNode::kBinaryDoesNotEqual, $1, $3); }
        |       RightHandSideExpressionNoIn TSTRICTEQ RightHandSideExpressionNoIn
                { $$ = new BinaryExpression(@$, SyntaxNode::kBinaryStrictEquals, $1, $3); }
        |       RightHandSideExpressionNoIn TSTRICTNE RightHandSideExpressionNoIn
                { $$ = new BinaryExpression(@$, SyntaxNode::kBinaryStrictDoesNotEqual, $1, $3); }
        |       RightHandSideExpressionNoIn '&' RightHandSideExpressionNoIn
                { $$ = new BinaryExpression(@$, SyntaxNode::kBinaryBitwiseAnd, $1, $3); }
        |       RightHandSideExpressionNoIn '^' RightHandSideExpressionNoIn
                { $$ = new BinaryExpression(@$, SyntaxNode::kBinaryBitwiseXor, $1, $3); }
        |       RightHandSideExpressionNoIn '|' RightHandSideExpressionNoIn
                { $$ = new BinaryExpression(@$, SyntaxNode::kBinaryBitwiseOr, $1, $3); }
        |       RightHandSideExpressionNoIn TLOGAND RightHandSideExpressionNoIn
                { $$ = new LogicalExpression(@$, SyntaxNode::kLogicalAnd, $1, $3); }
        |       RightHandSideExpressionNoIn TLOGOR RightHandSideExpressionNoIn
                { $$ = new LogicalExpression(@$, SyntaxNode::kLogicalOr, $1, $3); }
        ;

RightHandSideExpressionNoFB:
                ShiftExpressionNoFB
        |       RightHandSideExpressionNoFB '<' RightHandSideExpression
                { $$ = new BinaryExpression(@$, SyntaxNode::kBinaryLessThan, $1, $3); }
        |       RightHandSideExpressionNoFB '>' RightHandSideExpression
                { $$ = new BinaryExpression(@$, SyntaxNode::kBinaryGreaterThan, $1, $3); }
        |       RightHandSideExpressionNoFB TLE RightHandSideExpression
                { $$ = new BinaryExpression(@$, SyntaxNode::kBinaryLessThanOrEqual, $1, $3); }
        |       RightHandSideExpressionNoFB TGE RightHandSideExpression
                { $$ = new BinaryExpression(@$, SyntaxNode::kBinaryGreaterThanOrEqual, $1, $3); }
        |       RightHandSideExpressionNoFB TINSTANCEOF RightHandSideExpression
                { $$ = new BinaryExpression(@$, SyntaxNode::kBinaryInstanceOf, $1, $3); }
        |       RightHandSideExpressionNoFB TIN RightHandSideExpression
                { $$ = new BinaryExpression(@$, SyntaxNode::kBinaryIn, $1, $3); }
        |       RightHandSideExpressionNoFB TEQ RightHandSideExpression
                { $$ = new BinaryExpression(@$, SyntaxNode::kBinaryEquals, $1, $3); }
        |       RightHandSideExpressionNoFB TNE RightHandSideExpression
                { $$ = new BinaryExpression(@$, SyntaxNode::kBinaryDoesNotEqual, $1, $3); }
        |       RightHandSideExpressionNoFB TSTRICTEQ RightHandSideExpression
                { $$ = new BinaryExpression(@$, SyntaxNode::kBinaryStrictEquals, $1, $3); }
        |       RightHandSideExpressionNoFB TSTRICTNE RightHandSideExpression
                { $$ = new BinaryExpression(@$, SyntaxNode::kBinaryStrictDoesNotEqual, $1, $3); }
        |       RightHandSideExpressionNoFB '&' RightHandSideExpression
                { $$ = new BinaryExpression(@$, SyntaxNode::kBinaryBitwiseAnd, $1, $3); }
        |       RightHandSideExpressionNoFB '^' RightHandSideExpression
                { $$ = new BinaryExpression(@$, SyntaxNode::kBinaryBitwiseXor, $1, $3); }
        |       RightHandSideExpressionNoFB '|' RightHandSideExpression
                { $$ = new BinaryExpression(@$, SyntaxNode::kBinaryBitwiseOr, $1, $3); }
        |       RightHandSideExpressionNoFB TLOGAND RightHandSideExpression
                { $$ = new LogicalExpression(@$, SyntaxNode::kLogicalAnd, $1, $3); }
        |       RightHandSideExpressionNoFB TLOGOR RightHandSideExpression
                { $$ = new LogicalExpression(@$, SyntaxNode::kLogicalOr, $1, $3); }
        ;

ConditionalExpression:
                RightHandSideExpression
        |       RightHandSideExpression '?' AssignmentExpression ':' AssignmentExpression
                { $$ = new ConditionalExpression(@$, $1, $3, $5); }
        ;

ConditionalExpressionNoIn:
                RightHandSideExpressionNoIn
        |       RightHandSideExpressionNoIn '?' AssignmentExpression ':' AssignmentExpressionNoIn
                { $$ = new ConditionalExpression(@$, $1, $3, $5); }
        ;

ConditionalExpressionNoFB:
                RightHandSideExpressionNoFB
        |       RightHandSideExpressionNoFB '?' AssignmentExpression ':' AssignmentExpression
                { $$ = new ConditionalExpression(@$, $1, $3, $5); }
        ;

AssignmentExpression:
                ConditionalExpression
        |       LeftHandSideExpression '=' AssignmentExpression
                { $$ = new AssignmentExpression(@$, SyntaxNode::kAssignmentNone, $1, $3); }
        |       LeftHandSideExpression AssignmentOperator AssignmentExpression
                { $$ = new AssignmentExpression(@$, $2, $1, $3); }
        ;

AssignmentExpressionNoIn:
                ConditionalExpressionNoIn
        |       LeftHandSideExpression '=' AssignmentExpressionNoIn
                { $$ = new AssignmentExpression(@$, SyntaxNode::kAssignmentNone, $1, $3); }
        |       LeftHandSideExpression AssignmentOperator AssignmentExpressionNoIn
                { $$ = new AssignmentExpression(@$, $2, $1, $3); }
        ;

AssignmentExpressionNoFB:
                ConditionalExpressionNoFB
        |       LeftHandSideExpressionNoFB '=' AssignmentExpression
                { $$ = new AssignmentExpression(@$, SyntaxNode::kAssignmentNone, $1, $3); }
        |       LeftHandSideExpressionNoFB AssignmentOperator AssignmentExpression
                { $$ = new AssignmentExpression(@$, $2, $1, $3); }
        ;

AssignmentOperator:
                TASSIGNMUL { $$ = SyntaxNode::kAssignmentMultiplication; }
        |       TASSIGNDIV { $$ = SyntaxNode::kAssignmentDivision; }
        |       TASSIGNMOD { $$ = SyntaxNode::kAssignmentRemainder; }
        |       TASSIGNADD { $$ = SyntaxNode::kAssignmentAddition; }
        |       TASSIGNSUB { $$ = SyntaxNode::kAssignmentSubtraction; }
        |       TASSIGNSHIFTLEFT { $$ = SyntaxNode::kAssignmentLeftShift; }
        |       TASSIGNSHIFTRIGHT { $$ = SyntaxNode::kAssignmentSignedRightShift; }
        |       TASSIGNSHIFTRIGHTLOG { $$ = SyntaxNode::kAssignmentUnsignedRightShift; }
        |       TASSIGNAND { $$ = SyntaxNode::kAssignmentBitwiseAnd; }
        |       TASSIGNXOR { $$ = SyntaxNode::kAssignmentBitwiseXor; }
        |       TASSIGNOR { $$ = SyntaxNode::kAssignmentBitwiseOr; }
        ;

Expression:     AssignmentExpression
        |       Expression ',' AssignmentExpression
                { $$ = concat_expression(@$, $1, $3); }
        ;

ExpressionOpt:  { $$ = nullptr; }
        |       Expression
        ;

ExpressionNoIn: AssignmentExpressionNoIn
        |       ExpressionNoIn ',' AssignmentExpressionNoIn
                { $$ = concat_expression(@$, $1, $3); }
        ;

ExpressionNoInOpt:
                { $$ = nullptr; }
        |       ExpressionNoIn
        ;

ExpressionNoFB: AssignmentExpressionNoFB
        |       ExpressionNoFB ',' AssignmentExpression
                { $$ = concat_expression(@$, $1, $3); }
        ;

// A.4 Statements

Statement:      Block { $$ = $1; /* cast */ }
        |       VariableStatement
        |       EmptyStatement
        |       ExpressionStatement
        |       IfStatement
        |       IterationStatement
        |       ContinueStatement
        |       BreakStatement
        |       ReturnStatement
        |       WithStatement
        |       LabelledStatement
        |       SwitchStatement
        |       ThrowStatement
        |       TryStatement
        |       DebuggerStatement
        ;

Block:          '{' '}' { $$ = new BlockStatement(@$, nullptr); }
        |       '{' StatementList '}'
                { $$ = new BlockStatement(@$, $2); }
        ;

StatementList:  Statement
                { $$ = new std::vector<Statement*>(); $$->push_back($1); }
        |       FunctionDeclaration // ECMA-262 doesn't allow this.
                { $$ = new std::vector<Statement*>(); $$->push_back($1); }
        |       StatementList Statement
                { $$ = $1; $$->push_back($2); }
        |       StatementList FunctionDeclaration // ECMA-262 doesn't allow this.
                { $$ = $1; $$->push_back($2); }
        ;

StatementListOpt:
                { $$ = nullptr; }
        |       StatementList
        ;

VariableStatement:
                TVAR VariableDeclarationList ';'
                { $$ = new VariableDeclaration(@$, $2); }
        ;

VariableDeclarationList:
                VariableDeclaration
                { $$ = new std::vector<VariableDeclarator*>(); $$->push_back($1); }
        |       VariableDeclarationList ',' VariableDeclaration
                { $$ = $1; $$->push_back($3); }
        ;

VariableDeclarationListNoIn:
                VariableDeclarationNoIn
                { $$ = new std::vector<VariableDeclarator*>(); $$->push_back($1); }
        |       VariableDeclarationListNoIn ',' VariableDeclarationNoIn
                { $$ = $1; $$->push_back($3); }
        ;

VariableDeclaration:
                Identifier InitialiserOpt
                { $$ = new VariableDeclarator(@$, $1, $2); }
        ;

VariableDeclarationNoIn:
                Identifier InitialiserNoInOpt
                { $$ = new VariableDeclarator(@$, $1, $2); }
        ;

InitialiserOpt: { $$ = nullptr; }
        |       '=' AssignmentExpression { $$ = $2; }
        ;

InitialiserNoInOpt:
                { $$ = nullptr; }
        |       '=' AssignmentExpressionNoIn { $$ = $2; }
        ;

EmptyStatement: ';' { $$ = new EmptyStatement(@$); }
        ;

ExpressionStatement:
                ExpressionNoFB ';' { $$ = new ExpressionStatement(@$, $1); }
        ;

IfStatement:    TIF '(' Expression ')' Statement TELSE Statement
                { $$ = new IfStatement(@$, $3, $5, $7); }
        |       TIF '(' Expression ')' Statement
                { $$ = new IfStatement(@$, $3, $5, nullptr); }
        ;

IterationStatement:
                TDO Statement TWHILE '(' Expression ')'
                { $$ = new DoWhileStatement(@$, $2, $5); }
        |       TWHILE '(' Expression ')' Statement
                { $$ = new WhileStatement(@$, $3, $5); }
        |       TFOR '(' ExpressionNoInOpt ';' ExpressionOpt ';' ExpressionOpt ')' Statement
                { $$ = new ForStatement(@$, $3, $5, $7, $9); }
        |       TFOR '(' TVAR VariableDeclarationListNoIn ';' ExpressionOpt ';' ExpressionOpt ')' Statement
                { $$ = new ForStatement(@$, new VariableDeclaration(@4, $4), $6, $8, $10); }
        |       TFOR '(' LeftHandSideExpression TIN Expression ')' Statement
                { $$ = new ForInStatement(@$, $3, $5, $7); }
        |       TFOR '(' TVAR VariableDeclarationNoIn TIN Expression ')' Statement
                { std::vector<VariableDeclarator*>* declarations = new std::vector<VariableDeclarator*>(); declarations->push_back($4); VariableDeclaration* decl = new VariableDeclaration(@4, declarations); $$ = new ForInStatement(@$, decl, $6, $8); }
        ;

ContinueStatement:
                TCONTINUE ';'
                { $$ = new ContinueStatement(@$, nullptr); }
        |       TCONTINUE Identifier ';'
                { $$ = new ContinueStatement(@$, $2); }
        ;

BreakStatement: TBREAK ';'
                { $$ = new BreakStatement(@$, nullptr); }
        |       TBREAK Identifier ';'
                { $$ = new BreakStatement(@$, $2); }
        ;

ReturnStatement:
                TRETURN ';'
                { $$ = new ReturnStatement(@$, nullptr); }
        |       TRETURN Expression ';'
                { $$ = new ReturnStatement(@$, $2); }
        ;

WithStatement:  TWITH '(' Expression ')' Statement
                { $$ = new WithStatement(@$, $3, $5); }
        ;

SwitchStatement:
                TSWITCH '(' Expression ')' CaseBlock
                { $$ = new SwitchStatement(@$, $3, $5); }
        ;

CaseBlock:      '{' CaseClausesOpt '}' { $$ = $2; }
        |       '{' CaseClausesOpt DefaultClause CaseClausesOpt '}'
                { $$ = concat_switch_case($2, $3, $4); }
        ;

CaseClauses:    CaseClause
                { $$ = new std::vector<SwitchCase*>(); $$->push_back($1); }
        |       CaseClauses CaseClause
                { $$ = $1; $$->push_back($2); }
        ;

CaseClausesOpt: { $$ = nullptr; }
        |       CaseClauses
        ;

CaseClause:     TCASE Expression ':' StatementListOpt
                { $$ = new SwitchCase(@$, $2, $4); }
        ;

DefaultClause:  TDEFAULT ':' StatementListOpt
                { $$ = new SwitchCase(@$, nullptr, $3); }
        ;

LabelledStatement:
                Identifier ':' Statement
                { $$ = new LabeledStatement(@$, $1, $3); }
        ;

ThrowStatement:
                TTHROW Expression ';' { $$ = new ThrowStatement(@$, $2); }
        ;

TryStatement:   TTRY Block Catch
                { $$ = new TryStatement(@$, $2, $3, nullptr); }
        |       TTRY Block Finally
                { $$ = new TryStatement(@$, $2, nullptr, $3); }
        |       TTRY Block Catch Finally
                { $$ = new TryStatement(@$, $2, $3, $4); }
        ;

Catch:          TCATCH '(' Identifier ')' Block
                { $$ = new CatchClause(@$, $3, $5); }
        ;

Finally:        TFINALLY Block { $$ = $2; }
        ;

DebuggerStatement:
                TDEBUGGER ';' { $$ = new DebuggerStatement(@$); }
        ;

// Functions and Programs

FunctionDeclaration:
                TFUNCTION Identifier '(' FormalParameterListOpt ')' '{' FunctionBody '}'
                { $$ = new FunctionDeclaration(@$, new FunctionNode(@$, $2, $4, $7)); }
        ;

FunctionExpression:
                TFUNCTION IdentifierOpt '(' FormalParameterListOpt ')' '{' FunctionBody '}'
                { $$ = new FunctionExpression(@$, new FunctionNode(@$, $2, $4, $7)); }
        ;

FormalParameterListOpt:
                { $$ = nullptr; }
        |       FormalParameterList
        ;

FormalParameterList:
                Identifier
                { $$ = new std::vector<Identifier*>(); $$->push_back($1); }
        |       FormalParameterList ',' Identifier
                { $$ = $1; $$->push_back($3); }
        ;

FunctionBody:   SourceElementsOpt
                { $$ = new BlockStatement(@$, $1); }
        ;

Program:        SourceElementsOpt
                { $$ = new Program(@$, $1); yy_result = $$; }
        ;

SourceElementsOpt:
                { $$ = nullptr; }
        |       SourceElements
        ;

SourceElements: SourceElement
                { $$ = new std::vector<Statement*>(); $$->push_back($1); }
        |       SourceElements SourceElement
                { $$ = $1; $$->push_back($2); }
        ;

SourceElement:  Statement
        |       FunctionDeclaration
        ;

%%

void yyerror(const char *s) {
  fprintf(stderr, "file: %s: line: %d: error: %s\n", yy_name.c_str(), yylineno, s);
}
