/* Nabla JS - A small EMCAScript interpreter with straight-forward implementation.
 * Copyright (C) 2014 Katsuya Iida. All rights reserved.
 */

#pragma once

#ifndef NABLA_ASTEVAL_HH_
#define NABLA_ASTEVAL_HH_

#include "ast.hh"
#include "context.hh"
#include "data.hh"

namespace nabla {
namespace internal {

// 8.9 The Completion Specification Type
struct CompletionSpecification {
  enum Type { kNormal, kBreak, kContinue, kReturn, kThrow };
  
  any_ref value;
  Type type;
  u16string target;
};

struct LabelList {
  u16string label;
  const LabelList *next;
};

struct PropertyReference {
  any_ref base;
  u16string name;
  bool strict;
};

class AstEvaluator {
 public:
  static any_ref EvalScript(Context *context, Script* script);
  static any_ref CallFunction(Context* context, Script* script, Environment* scope, FunctionNode* expr, bool strict, any_ref this_val, size_t argc, const any_ref* argv);

 protected:
  AstEvaluator(Context *context, Script* script, any_ref this_val, bool strict);
  virtual ~AstEvaluator() {}
  any_ref EvalScript();
  any_ref CallFunction_(Environment* scope, FunctionNode* expr, size_t argc, const any_ref* argv);

 private:
  any_ref EvalProgram(Program* expr);
  Object* EvalFunctionNode(FunctionNode* expr);

  void EvalStatement(Statement* stmt) { EvalStatementWithLabel(stmt, nullptr); }
  void EvalStatementWithLabel(Statement* stmt, const LabelList* label_list);
  void EvalStatement_(EmptyStatement* stmt);
  void EvalStatement_(BlockStatement* stmt);
  void EvalStatement_(ExpressionStatement* stmt);
  void EvalStatement_(IfStatement* stmt);
  void EvalStatementWithLabel_(LabeledStatement* stmt, const LabelList* label_list);
  void EvalStatement_(BreakStatement* stmt);
  void EvalStatement_(ContinueStatement* stmt);
  void EvalStatement_(WithStatement* stmt);
  void EvalStatementWithLabel_(SwitchStatement* stmt, const LabelList* label_list);
  void EvalStatement_(ReturnStatement* stmt);
  void EvalStatement_(ThrowStatement* stmt);
  void EvalStatement_(TryStatement* stmt);
  void EvalStatementWithLabel_(WhileStatement* stmt, const LabelList* label_list);
  void EvalStatementWithLabel_(DoWhileStatement* stmt, const LabelList* label_list);
  void EvalStatementWithLabel_(ForStatement* stmt, const LabelList* label_list);
  void EvalStatementWithLabel_(ForInStatement* stmt, const LabelList* label_list);
  void EvalStatement_(DebuggerStatement* stmt);
  void EvalStatement_(VariableDeclaration* stmt);

  bool EvalVariableDeclarator(VariableDeclarator* decl);

  any_ref EvalExpressionToValue(Expression* expr);
  any_ref EvalExpressionToValue_(ThisExpression* expr);
  any_ref EvalExpressionToValue_(ArrayExpression* expr);
  any_ref EvalExpressionToValue_(ObjectExpression* expr);
  any_ref EvalExpressionToValue_(PropertyNode* expr);
  any_ref EvalExpressionToValue_(FunctionExpression* expr);
  any_ref EvalExpressionToValue_(SequenceExpression* expr);
  any_ref EvalExpressionToValue_(UnaryExpression* expr);
  any_ref EvalExpressionToValue_(BinaryExpression* expr);
  any_ref EvalExpressionToValue_(AssignmentExpression* expr);
  any_ref EvalExpressionToValue_(UpdateExpression* expr);
  any_ref EvalExpressionToValue_(LogicalExpression* expr);
  any_ref EvalExpressionToValue_(ConditionalExpression* expr);
  any_ref EvalExpressionToValue_(NewExpression* expr);
  any_ref EvalExpressionToValue_(CallExpression* expr);
  any_ref EvalExpressionToValue_(MemberExpression* expr);
  any_ref EvalExpressionToValue_(Identifier* expr);
  any_ref EvalExpressionToValue_(NullLiteral* expr);
  any_ref EvalExpressionToValue_(BooleanLiteral* expr);
  any_ref EvalExpressionToValue_(NumberLiteral* expr);
  any_ref EvalExpressionToValue_(StringLiteral* expr);
  any_ref EvalExpressionToValue_(RegExpLiteral* expr);

  u16string EvalExpressionToString(Expression* expr);
  any_ref EvalExpressionAndGetCompletion(Expression* expr);

  u16string EvalIdentifierToName(Identifier* expr) const {
    return script_->string_table()[expr->name];
  }
  bool ResolveIdentifierAndPutValue(u16string n, any_ref v);
  bool DeleteIdentifier(u16string n);
  any_ref ResolveIdentifierAndGetValueAndEnvironment(u16string n, bool do_throw, Environment*& resolved_env);
  bool PutValueWithEnvironment(Environment* env, u16string n, any_ref v);
  bool EvalMemberExpressionToReference(MemberExpression* expr, PropertyReference& ref);

  void EvalSwitchCase(SwitchCase* expr);
  void EvalCatchClause(CatchClause* expr);

  any_ref ApplyDeleteOperator(Expression* expr);
  any_ref ApplyTypeOfOperator(Expression* expr);

  void InitFunctionBindings(std::vector<Statement*>& body);
  void InitFunctionBindings(Statement* stmt);
  void InitFunctionBindings(TryStatement* stmt);
  void InitFunctionBindings(FunctionDeclaration* stmt);
  
  void InitVariableBindings(std::vector<Statement*>& body);
  void InitVariableBindings(Statement* stmt);
  void InitVariableBindings(TryStatement* stmt);
  void InitVariableBindings(ForStatement* stmt);
  void InitVariableBindings(ForInStatement* stmt);
  void InitVariableBindings(VariableDeclaration* stmt);

  bool CheckIfLabelMatch(const LabelList* ll);
  bool BreakOrContinueIfLabelMatch(const LabelList* ll);
  void SetCurValue(any_ref r) { cv_.value = r; }
  any_ref GetCurValue() { return cv_.value; }
  void DefineVariable(u16string n, any_ref v);

  Context* context_;
  Script* script_;
  Environment* cur_env_;
  CompletionSpecification cv_;
  bool strict_;
  any_ref this_val_;
};

}  // namespace internal
}  // namespace nabla

#endif  // NABLA_ASTEVAL_HH_
