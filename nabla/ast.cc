/* Nabla JS - A small EMCAScript interpreter with straight-forward implementation.
 * Copyright (C) 2014 Katsuya Iida. All rights reserved.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "ast.hh"

#include <iostream>

#include "context.hh"
#include "data.hh"
#include "debug.hh"

nabla::internal::Program* yy_parse_string(nabla::internal::AstBuilder* builder, const char16_t* s, size_t n, const char16_t* name);

namespace nabla {
namespace internal {

AstBuilder::AstBuilder() : last_string_index_(0) {
}

int AstBuilder::GetStringConstant(const std::string& s) {
  if (s.empty()) return 0;
  std::u16string u16s(s.begin(), s.end());
  int& index = string_map_[u16s];
  if (index == 0) index = ++last_string_index_;
  return index;
}


Parser::Result<Program> Parser::ParseProgram(const char16_t *s, size_t n, const char16_t* name) {
  Program* program = yy_parse_string(&builder_, s, n, name);
  if (!program) {
    return Result<Program>();
  }
  auto string_map = builder_.string_map();
#if 0
  std::cout << "string map size: " << string_map.size() << std::endl;
  for (auto it = string_map.begin(); it != string_map.end(); ++it) {
    const std::u16string& s = (*it).first;
    int i = (*it).second;
    any_ref v = u16string_data::alloc(s.data(), s.length());
    std::cout << i << " -> " << v << std::endl;
  }
#endif
  return Result<Program>(program, string_map);
}

// <!--
Program::Program(const SourceLocation& loc, std::vector<Statement*>* body)
    : SyntaxNode(kProgram, loc) {
  if (body) { this->body = std::move(*body); delete body; }
}

Program::~Program() {
  delete_syntax_node_vector(body);
}

FunctionNode::FunctionNode(const SourceLocation& loc, Identifier* id, std::vector<Identifier*>* params, BlockStatement* body)
    : SyntaxNode(kFunction, loc) {
  this->id = id;
  if (params) { this->params = std::move(*params); delete params; }
  this->body = body;
}

FunctionNode::~FunctionNode() {
  delete id;
  delete_syntax_node_vector(params);
  delete body;
}

EmptyStatement::EmptyStatement(const SourceLocation& loc)
    : Statement(kEmptyStatement, loc) {
}

EmptyStatement::~EmptyStatement() {
}

BlockStatement::BlockStatement(const SourceLocation& loc, std::vector<Statement*>* body)
    : Statement(kBlockStatement, loc) {
  if (body) { this->body = std::move(*body); delete body; }
}

BlockStatement::~BlockStatement() {
  delete_syntax_node_vector(body);
}

ExpressionStatement::ExpressionStatement(const SourceLocation& loc, Expression* expression)
    : Statement(kExpressionStatement, loc) {
  this->expression = expression;
}

ExpressionStatement::~ExpressionStatement() {
  delete expression;
}

IfStatement::IfStatement(const SourceLocation& loc, Expression* test, Statement* consequent, Statement* alternate)
    : Statement(kIfStatement, loc) {
  this->test = test;
  this->consequent = consequent;
  this->alternate = alternate;
}

IfStatement::~IfStatement() {
  delete test;
  delete consequent;
  delete alternate;
}

LabeledStatement::LabeledStatement(const SourceLocation& loc, Identifier* label, Statement* body)
    : Statement(kLabeledStatement, loc) {
  this->label = label;
  this->body = body;
}

LabeledStatement::~LabeledStatement() {
  delete label;
  delete body;
}

BreakStatement::BreakStatement(const SourceLocation& loc, Identifier* label)
    : Statement(kBreakStatement, loc) {
  this->label = label;
}

BreakStatement::~BreakStatement() {
  delete label;
}

ContinueStatement::ContinueStatement(const SourceLocation& loc, Identifier* label)
    : Statement(kContinueStatement, loc) {
  this->label = label;
}

ContinueStatement::~ContinueStatement() {
  delete label;
}

WithStatement::WithStatement(const SourceLocation& loc, Expression* object, Statement* body)
    : Statement(kWithStatement, loc) {
  this->object = object;
  this->body = body;
}

WithStatement::~WithStatement() {
  delete object;
  delete body;
}

SwitchStatement::SwitchStatement(const SourceLocation& loc, Expression* discriminant, std::vector<SwitchCase*>* cases)
    : Statement(kSwitchStatement, loc) {
  this->discriminant = discriminant;
  if (cases) { this->cases = std::move(*cases); delete cases; }
}

SwitchStatement::~SwitchStatement() {
  delete discriminant;
  delete_syntax_node_vector(cases);
}

ReturnStatement::ReturnStatement(const SourceLocation& loc, Expression* argument)
    : Statement(kReturnStatement, loc) {
  this->argument = argument;
}

ReturnStatement::~ReturnStatement() {
  delete argument;
}

ThrowStatement::ThrowStatement(const SourceLocation& loc, Expression* argument)
    : Statement(kThrowStatement, loc) {
  this->argument = argument;
}

ThrowStatement::~ThrowStatement() {
  delete argument;
}

TryStatement::TryStatement(const SourceLocation& loc, BlockStatement* block, CatchClause* handler, BlockStatement* finalizer)
    : Statement(kTryStatement, loc) {
  this->block = block;
  this->handler = handler;
  this->finalizer = finalizer;
}

TryStatement::~TryStatement() {
  delete block;
  delete handler;
  delete finalizer;
}

WhileStatement::WhileStatement(const SourceLocation& loc, Expression* test, Statement* body)
    : Statement(kWhileStatement, loc) {
  this->test = test;
  this->body = body;
}

WhileStatement::~WhileStatement() {
  delete test;
  delete body;
}

DoWhileStatement::DoWhileStatement(const SourceLocation& loc, Statement* body, Expression* test)
    : Statement(kDoWhileStatement, loc) {
  this->body = body;
  this->test = test;
}

DoWhileStatement::~DoWhileStatement() {
  delete body;
  delete test;
}

ForStatement::ForStatement(const SourceLocation& loc, SyntaxNode* init, Expression* test, Expression* update, Statement* body)
    : Statement(kForStatement, loc) {
  this->init = init;
  this->test = test;
  this->update = update;
  this->body = body;
}

ForStatement::~ForStatement() {
  delete init;
  delete test;
  delete update;
  delete body;
}

ForInStatement::ForInStatement(const SourceLocation& loc, SyntaxNode* left, Expression* right, Statement* body)
    : Statement(kForInStatement, loc) {
  this->left = left;
  this->right = right;
  this->body = body;
}

ForInStatement::~ForInStatement() {
  delete left;
  delete right;
  delete body;
}

DebuggerStatement::DebuggerStatement(const SourceLocation& loc)
    : Statement(kDebuggerStatement, loc) {
}

DebuggerStatement::~DebuggerStatement() {
}

FunctionDeclaration::FunctionDeclaration(const SourceLocation& loc, FunctionNode* function)
    : Declaration(kFunctionDeclaration, loc) {
  this->function = function;
}

FunctionDeclaration::~FunctionDeclaration() {
  delete function;
}

VariableDeclaration::VariableDeclaration(const SourceLocation& loc, std::vector<VariableDeclarator*>* declarations)
    : Declaration(kVariableDeclaration, loc) {
  if (declarations) { this->declarations = std::move(*declarations); delete declarations; }
}

VariableDeclaration::~VariableDeclaration() {
  delete_syntax_node_vector(declarations);
}

VariableDeclarator::VariableDeclarator(const SourceLocation& loc, Identifier* id, Expression* init)
    : SyntaxNode(kVariableDeclarator, loc) {
  this->id = id;
  this->init = init;
}

VariableDeclarator::~VariableDeclarator() {
  delete id;
  delete init;
}

ThisExpression::ThisExpression(const SourceLocation& loc)
    : Expression(kThisExpression, loc) {
}

ThisExpression::~ThisExpression() {
}

ArrayExpression::ArrayExpression(const SourceLocation& loc, std::vector<Expression*>* elements)
    : Expression(kArrayExpression, loc) {
  if (elements) { this->elements = std::move(*elements); delete elements; }
}

ArrayExpression::~ArrayExpression() {
  delete_syntax_node_vector(elements);
}

ObjectExpression::ObjectExpression(const SourceLocation& loc, std::vector<PropertyNode*>* properties)
    : Expression(kObjectExpression, loc) {
  if (properties) { this->properties = std::move(*properties); delete properties; }
}

ObjectExpression::~ObjectExpression() {
  delete_syntax_node_vector(properties);
}

PropertyNode::PropertyNode(const SourceLocation& loc, Expression* key, Expression* value, PropertyKind kind)
    : SyntaxNode(kProperty, loc) {
  this->key = key;
  this->value = value;
  this->kind = kind;
}

PropertyNode::~PropertyNode() {
  delete key;
  delete value;
}

FunctionExpression::FunctionExpression(const SourceLocation& loc, FunctionNode* function)
    : Expression(kFunctionExpression, loc) {
  this->function = function;
}

FunctionExpression::~FunctionExpression() {
  delete function;
}

SequenceExpression::SequenceExpression(const SourceLocation& loc, std::vector<Expression*>* expressions)
    : Expression(kSequenceExpression, loc) {
  if (expressions) { this->expressions = std::move(*expressions); delete expressions; }
}

SequenceExpression::~SequenceExpression() {
  delete_syntax_node_vector(expressions);
}

UnaryExpression::UnaryExpression(const SourceLocation& loc, UnaryOperator _operator, bool prefix, Expression* argument)
    : Expression(kUnaryExpression, loc) {
  this->_operator = _operator;
  this->prefix = prefix;
  this->argument = argument;
}

UnaryExpression::~UnaryExpression() {
  delete argument;
}

BinaryExpression::BinaryExpression(const SourceLocation& loc, BinaryOperator _operator, Expression* left, Expression* right)
    : Expression(kBinaryExpression, loc) {
  this->_operator = _operator;
  this->left = left;
  this->right = right;
}

BinaryExpression::~BinaryExpression() {
  delete left;
  delete right;
}

AssignmentExpression::AssignmentExpression(const SourceLocation& loc, AssignmentOperator _operator, Expression* left, Expression* right)
    : Expression(kAssignmentExpression, loc) {
  this->_operator = _operator;
  this->left = left;
  this->right = right;
}

AssignmentExpression::~AssignmentExpression() {
  delete left;
  delete right;
}

UpdateExpression::UpdateExpression(const SourceLocation& loc, UpdateOperator _operator, Expression* argument, bool prefix)
    : Expression(kUpdateExpression, loc) {
  this->_operator = _operator;
  this->argument = argument;
  this->prefix = prefix;
}

UpdateExpression::~UpdateExpression() {
  delete argument;
}

LogicalExpression::LogicalExpression(const SourceLocation& loc, LogicalOperator _operator, Expression* left, Expression* right)
    : Expression(kLogicalExpression, loc) {
  this->_operator = _operator;
  this->left = left;
  this->right = right;
}

LogicalExpression::~LogicalExpression() {
  delete left;
  delete right;
}

ConditionalExpression::ConditionalExpression(const SourceLocation& loc, Expression* test, Expression* consequent, Expression* alternate)
    : Expression(kConditionalExpression, loc) {
  this->test = test;
  this->consequent = consequent;
  this->alternate = alternate;
}

ConditionalExpression::~ConditionalExpression() {
  delete test;
  delete alternate;
  delete consequent;
}

NewExpression::NewExpression(const SourceLocation& loc, Expression* callee, std::vector<Expression*>* arguments)
    : Expression(kNewExpression, loc) {
  this->callee = callee;
  if (arguments) { this->arguments = std::move(*arguments); delete arguments; }
}

NewExpression::~NewExpression() {
  delete callee;
  delete_syntax_node_vector(arguments);
}

CallExpression::CallExpression(const SourceLocation& loc, Expression* callee, std::vector<Expression*>* arguments)
    : Expression(kCallExpression, loc) {
  this->callee = callee;
  if (arguments) { this->arguments = std::move(*arguments); delete arguments; }
}

CallExpression::~CallExpression() {
  delete callee;
  delete_syntax_node_vector(arguments);
}

MemberExpression::MemberExpression(const SourceLocation& loc, Expression* object, Expression* property, bool computed)
    : Expression(kMemberExpression, loc) {
  this->object = object;
  this->property = property;
  this->computed = computed;
}

MemberExpression::~MemberExpression() {
  delete object;
  delete property;
}

SwitchCase::SwitchCase(const SourceLocation& loc, Expression* test, std::vector<Statement*>* consequent)
    : SyntaxNode(kSwitchCase, loc) {
  this->test = test;
  if (consequent) { this->consequent = std::move(*consequent); delete consequent; }
}

SwitchCase::~SwitchCase() {
  delete test;
  delete_syntax_node_vector(consequent);
}

CatchClause::CatchClause(const SourceLocation& loc, Identifier* param, BlockStatement* body)
    : SyntaxNode(kCatchClause, loc) {
  this->param = param;
  this->body = body;
}

CatchClause::~CatchClause() {
  delete param;
  delete body;
}
// -->

}  // namespace internal
}  // namespace nabla
