/* Nabla JS - A small EMCAScript interpreter with straight-forward implementation.
 * Copyright (C) 2014 Katsuya Iida. All rights reserved.
 */

#pragma once

#ifndef NABLA_AST_HH_
#define NABLA_AST_HH_

#include <map>
#include <memory>
#include <vector>

namespace nabla {
namespace internal {

struct Position {
  int line;
  int column;
};

struct SourceLocation {
  Position start;
  Position end;
};

class SyntaxNode {
 public:
  enum UnaryOperator {
    kUnaryDelete, kUnaryVoid, kUnaryTypeOf,
    kUnaryPositive, kUnaryNegative, kUnaryBitwiseNot, kUnaryLogicalNot
  };

  enum BinaryOperator {
    kBinaryMultiplication, kBinaryDivision, kBinaryRemainder,
    kBinaryAddition, kBinarySubtraction,
    kBinaryLeftShift, kBinarySignedRightShift, kBinaryUnsignedRightShift,
    kBinaryLessThan, kBinaryGreaterThan,
    kBinaryLessThanOrEqual, kBinaryGreaterThanOrEqual,
    kBinaryInstanceOf, kBinaryIn,
    kBinaryEquals, kBinaryDoesNotEqual,
    kBinaryStrictEquals, kBinaryStrictDoesNotEqual,
    kBinaryBitwiseAnd, kBinaryBitwiseXor, kBinaryBitwiseOr
  };

  enum LogicalOperator {
    kLogicalAnd, kLogicalOr
  };

  enum AssignmentOperator {
    kAssignmentNone,
    kAssignmentMultiplication, kAssignmentDivision, kAssignmentRemainder,
    kAssignmentAddition, kAssignmentSubtraction,
    kAssignmentLeftShift, kAssignmentSignedRightShift, kAssignmentUnsignedRightShift,
    kAssignmentBitwiseAnd, kAssignmentBitwiseXor, kAssignmentBitwiseOr
  };

  enum UpdateOperator {
    kUpdateIncrement, kUpdateDecrement
  };

  enum PropertyKind {
    kPropertySet, kPropertyGet, kPropertyInit
  };

// <!--
  enum SyntaxNodeType {
    kProgram,
    kFunction,
    kEmptyStatement,
    kBlockStatement,
    kExpressionStatement,
    kIfStatement,
    kLabeledStatement,
    kBreakStatement,
    kContinueStatement,
    kWithStatement,
    kSwitchStatement,
    kReturnStatement,
    kThrowStatement,
    kTryStatement,
    kWhileStatement,
    kDoWhileStatement,
    kForStatement,
    kForInStatement,
    kDebuggerStatement,
    kFunctionDeclaration,
    kVariableDeclaration,
    kVariableDeclarator,
    kThisExpression,
    kArrayExpression,
    kObjectExpression,
    kProperty,
    kFunctionExpression,
    kSequenceExpression,
    kUnaryExpression,
    kBinaryExpression,
    kAssignmentExpression,
    kUpdateExpression,
    kLogicalExpression,
    kConditionalExpression,
    kNewExpression,
    kCallExpression,
    kMemberExpression,
    kSwitchCase,
    kCatchClause,
    kIdentifier,
    kNullLiteral,
    kBooleanLiteral,
    kNumberLiteral,
    kStringLiteral,
    kRegExpLiteral
  };
// -->

 public:
  virtual ~SyntaxNode() {}

  const SyntaxNodeType type;
  SourceLocation loc;

 protected:
  SyntaxNode(SyntaxNodeType type, const SourceLocation& loc) : type(type), loc(loc) {}
};

class Expression : public SyntaxNode {
 protected:
  Expression(SyntaxNodeType type, const SourceLocation& loc) : SyntaxNode(type, loc) {}
};

class Statement : public SyntaxNode {
 protected:
  Statement(SyntaxNodeType type, const SourceLocation& loc) : SyntaxNode(type, loc) {}
};

class Declaration : public Statement {
 protected:
  Declaration(SyntaxNodeType type, const SourceLocation& loc) : Statement(type, loc) {}
};

template <typename T>
class NodeList : public std::vector<T*> {
 public:
  ~NodeList() {
    for (auto it = this->begin(); it != this->end(); ++it) delete *it;
  }
};

template <typename T>
class NodePtr {
 public:
  NodePtr(T* node) : node_(node) {}
  NodePtr() : node_(nullptr) {}
  ~NodePtr() { delete node_; }

  void Reset(T* node) { delete node_; node_ = node; }
  operator T* () const { return node_; }

 private:
  T* node_;
};

class Identifier : public Expression {
 public:
  Identifier(const SourceLocation& loc, int name) : Expression(kIdentifier, loc), name(name) {}

  int name;
};

class NullLiteral : public Expression {
 public:
  NullLiteral(const SourceLocation& loc) : Expression(kNullLiteral, loc) {}
};

class BooleanLiteral : public Expression {
 public:
  BooleanLiteral(const SourceLocation& loc, bool value) : Expression(kBooleanLiteral, loc), value(value) {}

  bool value;
};

class NumberLiteral : public Expression {
 public:
  NumberLiteral(const SourceLocation& loc, double value) : Expression(kNumberLiteral, loc), value(value) {}

  double value;
};

class StringLiteral : public Expression {
 public:
  StringLiteral(const SourceLocation& loc, int value) : Expression(kStringLiteral, loc), value(value) {}

  int value;
};

class RegExpLiteral : public Expression {
 public:
  RegExpLiteral(const SourceLocation& loc, std::string pattern, std::string flags)
    : Expression(kRegExpLiteral, loc), pattern(pattern), flags(flags) {}

  std::string pattern;
  std::string flags;
};

class BlockStatement;
class SwitchCase;
class CatchClause;
class VariableDeclarator;
class PropertyNode;

template <typename T>
void delete_syntax_node_vector(std::vector<T*> v) {
  for (auto it = v.begin(); it != v.end(); ++it) delete *it;
}
  
// <!--
class Program : public SyntaxNode {
 public:
  Program(const SourceLocation& loc, std::vector<Statement*>* body);
  ~Program();
  std::vector<Statement*> body;
};

class FunctionNode : public SyntaxNode {
 public:
  FunctionNode(const SourceLocation& loc, Identifier* id, std::vector<Identifier*>* params, BlockStatement* body);
  ~FunctionNode();
  Identifier* id;
  std::vector<Identifier*> params;
  BlockStatement* body;
};

class EmptyStatement : public Statement {
 public:
  EmptyStatement(const SourceLocation& loc);
  ~EmptyStatement();
};

class BlockStatement : public Statement {
 public:
  BlockStatement(const SourceLocation& loc, std::vector<Statement*>* body);
  ~BlockStatement();
  std::vector<Statement*> body;
};

class ExpressionStatement : public Statement {
 public:
  ExpressionStatement(const SourceLocation& loc, Expression* expression);
  ~ExpressionStatement();
  Expression* expression;
};

class IfStatement : public Statement {
 public:
  IfStatement(const SourceLocation& loc, Expression* test, Statement* consequent, Statement* alternate);
  ~IfStatement();
  Expression* test;
  Statement* consequent;
  Statement* alternate;
};

class LabeledStatement : public Statement {
 public:
  LabeledStatement(const SourceLocation& loc, Identifier* label, Statement* body);
  ~LabeledStatement();
  Identifier* label;
  Statement* body;
};

class BreakStatement : public Statement {
 public:
  BreakStatement(const SourceLocation& loc, Identifier* label);
  ~BreakStatement();
  Identifier* label;
};

class ContinueStatement : public Statement {
 public:
  ContinueStatement(const SourceLocation& loc, Identifier* label);
  ~ContinueStatement();
  Identifier* label;
};

class WithStatement : public Statement {
 public:
  WithStatement(const SourceLocation& loc, Expression* object, Statement* body);
  ~WithStatement();
  Expression* object;
  Statement* body;
};

class SwitchStatement : public Statement {
 public:
  SwitchStatement(const SourceLocation& loc, Expression* discriminant, std::vector<SwitchCase*>* cases);
  ~SwitchStatement();
  Expression* discriminant;
  std::vector<SwitchCase*> cases;
};

class ReturnStatement : public Statement {
 public:
  ReturnStatement(const SourceLocation& loc, Expression* argument);
  ~ReturnStatement();
  Expression* argument;
};

class ThrowStatement : public Statement {
 public:
  ThrowStatement(const SourceLocation& loc, Expression* argument);
  ~ThrowStatement();
  Expression* argument;
};

class TryStatement : public Statement {
 public:
  TryStatement(const SourceLocation& loc, BlockStatement* block, CatchClause* handler, BlockStatement* finalizer);
  ~TryStatement();
  BlockStatement* block;
  CatchClause* handler;
  BlockStatement* finalizer;
};

class WhileStatement : public Statement {
 public:
  WhileStatement(const SourceLocation& loc, Expression* test, Statement* body);
  ~WhileStatement();
  Expression* test;
  Statement* body;
};

class DoWhileStatement : public Statement {
 public:
  DoWhileStatement(const SourceLocation& loc, Statement* body, Expression* test);
  ~DoWhileStatement();
  Statement* body;
  Expression* test;
};

class ForStatement : public Statement {
 public:
  ForStatement(const SourceLocation& loc, SyntaxNode* init, Expression* test, Expression* update, Statement* body);
  ~ForStatement();
  SyntaxNode* init;
  Expression* test;
  Expression* update;
  Statement* body;
};

class ForInStatement : public Statement {
 public:
  ForInStatement(const SourceLocation& loc, SyntaxNode* left, Expression* right, Statement* body);
  ~ForInStatement();
  SyntaxNode* left;
  Expression* right;
  Statement* body;
};

class DebuggerStatement : public Statement {
 public:
  DebuggerStatement(const SourceLocation& loc);
  ~DebuggerStatement();
};

class FunctionDeclaration : public Declaration {
 public:
  FunctionDeclaration(const SourceLocation& loc, FunctionNode* function);
  ~FunctionDeclaration();
  FunctionNode* function;
};

class VariableDeclaration : public Declaration {
 public:
  VariableDeclaration(const SourceLocation& loc, std::vector<VariableDeclarator*>* declarations);
  ~VariableDeclaration();
  std::vector<VariableDeclarator*> declarations;
};

class VariableDeclarator : public SyntaxNode {
 public:
  VariableDeclarator(const SourceLocation& loc, Identifier* id, Expression* init);
  ~VariableDeclarator();
  Identifier* id;
  Expression* init;
};

class ThisExpression : public Expression {
 public:
  ThisExpression(const SourceLocation& loc);
  ~ThisExpression();
};

class ArrayExpression : public Expression {
 public:
  ArrayExpression(const SourceLocation& loc, std::vector<Expression*>* elements);
  ~ArrayExpression();
  std::vector<Expression*> elements;
};

class ObjectExpression : public Expression {
 public:
  ObjectExpression(const SourceLocation& loc, std::vector<PropertyNode*>* properties);
  ~ObjectExpression();
  std::vector<PropertyNode*> properties;
};

class PropertyNode : public SyntaxNode {
 public:
  PropertyNode(const SourceLocation& loc, Expression* key, Expression* value, PropertyKind kind);
  ~PropertyNode();
  Expression* key;
  Expression* value;
  PropertyKind kind;
};

class FunctionExpression : public Expression {
 public:
  FunctionExpression(const SourceLocation& loc, FunctionNode* function);
  ~FunctionExpression();
  FunctionNode* function;
};

class SequenceExpression : public Expression {
 public:
  SequenceExpression(const SourceLocation& loc, std::vector<Expression*>* expressions);
  ~SequenceExpression();
  std::vector<Expression*> expressions;
};

class UnaryExpression : public Expression {
 public:
  UnaryExpression(const SourceLocation& loc, UnaryOperator _operator, bool prefix, Expression* argument);
  ~UnaryExpression();
  UnaryOperator _operator;
  bool prefix;
  Expression* argument;
};

class BinaryExpression : public Expression {
 public:
  BinaryExpression(const SourceLocation& loc, BinaryOperator _operator, Expression* left, Expression* right);
  ~BinaryExpression();
  BinaryOperator _operator;
  Expression* left;
  Expression* right;
};

class AssignmentExpression : public Expression {
 public:
  AssignmentExpression(const SourceLocation& loc, AssignmentOperator _operator, Expression* left, Expression* right);
  ~AssignmentExpression();
  AssignmentOperator _operator;
  Expression* left;
  Expression* right;
};

class UpdateExpression : public Expression {
 public:
  UpdateExpression(const SourceLocation& loc, UpdateOperator _operator, Expression* argument, bool prefix);
  ~UpdateExpression();
  UpdateOperator _operator;
  Expression* argument;
  bool prefix;
};

class LogicalExpression : public Expression {
 public:
  LogicalExpression(const SourceLocation& loc, LogicalOperator _operator, Expression* left, Expression* right);
  ~LogicalExpression();
  LogicalOperator _operator;
  Expression* left;
  Expression* right;
};

class ConditionalExpression : public Expression {
 public:
  ConditionalExpression(const SourceLocation& loc, Expression* test, Expression* consequent, Expression* alternate);
  ~ConditionalExpression();
  Expression* test;
  Expression* consequent;
  Expression* alternate;
};

class NewExpression : public Expression {
 public:
  NewExpression(const SourceLocation& loc, Expression* callee, std::vector<Expression*>* arguments);
  ~NewExpression();
  Expression* callee;
  std::vector<Expression*> arguments;
};

class CallExpression : public Expression {
 public:
  CallExpression(const SourceLocation& loc, Expression* callee, std::vector<Expression*>* arguments);
  ~CallExpression();
  Expression* callee;
  std::vector<Expression*> arguments;
};

class MemberExpression : public Expression {
 public:
  MemberExpression(const SourceLocation& loc, Expression* object, Expression* property, bool computed);
  ~MemberExpression();
  Expression* object;
  Expression* property;
  bool computed;
};

class SwitchCase : public SyntaxNode {
 public:
  SwitchCase(const SourceLocation& loc, Expression* test, std::vector<Statement*>* consequent);
  ~SwitchCase();
  Expression* test;
  std::vector<Statement*> consequent;
};

class CatchClause : public SyntaxNode {
 public:
  CatchClause(const SourceLocation& loc, Identifier* param, BlockStatement* body);
  ~CatchClause();
  Identifier* param;
  BlockStatement* body;
};
// -->

// AST builder

class AstBuilder {
 public:
  AstBuilder();
  int GetStringConstant(const std::string& s);
  std::map<std::u16string, int>& string_map() { return string_map_; }

 private:
  std::map<std::u16string, int> string_map_;
  int last_string_index_;
};

// Parser

class Parser {
 public:
  template <typename T>
  class Result {
   public:
    Result(T* node, std::map<std::u16string, int>& string_map) : node_(node), string_map_(std::move(string_map)) {}
    Result() : node_(nullptr) {}
    ~Result() { delete node_; }
    bool IsSuccess() { return !!node_; }
    T* ReleaseNode() { T* node = node_; node_ = nullptr; return node; }
    std::map<std::u16string, int>& string_map() { return string_map_;}

   private:
    T* node_;
    std::map<std::u16string, int> string_map_;
  };
    
  Result<Program> ParseProgram(const char16_t* s, size_t n, const char16_t* name);

 private:
  AstBuilder builder_;
};

}  // namespace internal
}  // namespace nabla

#endif  // NABLA_AST_HH_
