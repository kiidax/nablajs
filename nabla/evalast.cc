/* Nabla JS - A small EMCAScript interpreter with straight-forward implementation.
 * Copyright (C) 2014 Katsuya Iida. All rights reserved.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "evalast.hh"

#include <cassert>
#include <iostream>

#include "ast.hh"
#include "debug.hh"

namespace nabla {
namespace internal {

AstEvaluator::AstEvaluator(Context *context, Script* script, any_ref this_val, bool strict)
    : context_(context), script_(script), cur_env_(nullptr), strict_(strict) {
  this_val_ = this_val;
  cv_.value = undefined_data::alloc();
  cv_.type = CompletionSpecification::kNormal;
}

any_ref AstEvaluator::EvalScript(Context *context, Script* script) {
  AstEvaluator evaluator(context, script, context->global_obj(), false);
  return evaluator.EvalProgram(evaluator.script_->program());
}

void AstEvaluator::EvalStatementWithLabel(Statement* stmt, const LabelList* label_list) {
  assert(stmt);
  switch (stmt->type) {
    case SyntaxNode::kEmptyStatement:
      EvalStatement_(static_cast<EmptyStatement*>(stmt));
      break;
    case SyntaxNode::kBlockStatement:
      EvalStatement_(static_cast<BlockStatement*>(stmt));
      break;
    case SyntaxNode::kExpressionStatement:
      EvalStatement_(static_cast<ExpressionStatement*>(stmt));
      break;
    case SyntaxNode::kIfStatement:
      EvalStatement_(static_cast<IfStatement*>(stmt));
      break;
    case SyntaxNode::kLabeledStatement:
      EvalStatementWithLabel_(static_cast<LabeledStatement*>(stmt), label_list);
      break;
    case SyntaxNode::kBreakStatement:
      EvalStatement_(static_cast<BreakStatement*>(stmt));
      break;
    case SyntaxNode::kContinueStatement:
      EvalStatement_(static_cast<ContinueStatement*>(stmt));
      break;
    case SyntaxNode::kWithStatement:
      EvalStatement_(static_cast<WithStatement*>(stmt));
      break;
    case SyntaxNode::kSwitchStatement:
      EvalStatementWithLabel_(static_cast<SwitchStatement*>(stmt), label_list);
      break;
    case SyntaxNode::kReturnStatement:
      EvalStatement_(static_cast<ReturnStatement*>(stmt));
      break;
    case SyntaxNode::kThrowStatement:
      EvalStatement_(static_cast<ThrowStatement*>(stmt));
      break;
    case SyntaxNode::kTryStatement:
      EvalStatement_(static_cast<TryStatement*>(stmt));
      break;
    case SyntaxNode::kWhileStatement:
      EvalStatementWithLabel_(static_cast<WhileStatement*>(stmt), label_list);
      break;
    case SyntaxNode::kDoWhileStatement:
      EvalStatementWithLabel_(static_cast<DoWhileStatement*>(stmt), label_list);
      break;
    case SyntaxNode::kForStatement:
      EvalStatementWithLabel_(static_cast<ForStatement*>(stmt), label_list);
      break;
    case SyntaxNode::kForInStatement:
      EvalStatementWithLabel_(static_cast<ForInStatement*>(stmt), label_list);
      break;
    case SyntaxNode::kDebuggerStatement:
      EvalStatement_(static_cast<DebuggerStatement*>(stmt));
      break;
    case SyntaxNode::kFunctionDeclaration:
      // no-op
      break;
    case SyntaxNode::kVariableDeclaration:
      EvalStatement_(static_cast<VariableDeclaration*>(stmt));
      break;
    default:
      assert(false);
      break;
  }
}

void AstEvaluator::DefineVariable(u16string n, any_ref v) {
  Environment* env = cur_env_;
  assert(!!env);
  if (cur_env_->is<DeclarativeEnvironment>()) {
    DeclarativeEnvironment* decl_env = static_cast<DeclarativeEnvironment*>(cur_env_);
    decl_env->CreateBinding(n, v, false, false);
  } else {
    assert(env->is<ObjectEnvironment>());
    ObjectEnvironment* obj_env = static_cast<ObjectEnvironment*>(env);
    obj_env->CreateMutableBinding(context_, n, v, false);
  }
}

// Programs

any_ref AstEvaluator::EvalProgram(Program* expr) {
  ObjectEnvironment* env = ObjectEnvironment::Alloc(nullptr);
  env->bindings_obj = context_->global_obj();
  cur_env_ = env;

  InitFunctionBindings(expr->body);
  InitVariableBindings(expr->body);
  for (auto it = expr->body.begin(); it != expr->body.end(); ++it) {
    EvalStatement(*it);
    if (cv_.type != CompletionSpecification::kNormal) return nullptr;
  }
  return GetCurValue();
}

// Functions

Object* AstEvaluator::EvalFunctionNode(FunctionNode* expr) {
  // 13.2 Creating Function Objects
  Object* o = Object::Alloc(context_->function_proto());
  Function* fn = Function::Alloc();
  fn->script = script_;
  fn->code = expr;
  fn->context = context_;
  fn->scope = cur_env_;
  fn->strict = false;
  o->host_data = fn;

  // prototype
  Object* proto = Object::Alloc(context_->object_proto());
  o->Put(context_, "prototype", proto, false);

  // constructor
  proto->Put(context_, "constructor", o, false);

  return o;
}

// Statements

void AstEvaluator::EvalStatement_(EmptyStatement* expr) {
  // 12.3 Empty Statement
}

void AstEvaluator::EvalStatement_(BlockStatement* expr) {
  // 12.1 Block
  for (auto it = expr->body.begin(); it != expr->body.end(); ++it) {
    EvalStatement(*it);
    if (cv_.type != CompletionSpecification::kNormal) return;
  }
}

void AstEvaluator::EvalStatement_(ExpressionStatement* expr) {
  // 12.4 Expression Statement
  any_ref v = EvalExpressionAndGetCompletion(expr->expression);
  if (!v) return;
  cv_.value = v;
}

void AstEvaluator::EvalStatement_(IfStatement* expr) {
  // 12.5 The if Statement
  any_ref tval = EvalExpressionAndGetCompletion(expr->test);
  if (!tval) return;
  if (ToBoolean(tval)) {
    EvalStatement(expr->consequent);
  } else if (expr->alternate) {
    EvalStatement(expr->alternate);
  }
}

void AstEvaluator::EvalStatementWithLabel_(LabeledStatement* stmt, const LabelList* label_list) {
  // 12.12 Labelled Statements
  LabelList ll;
  ll.label = EvalIdentifierToName(stmt->label);
  ll.next = label_list;
  EvalStatementWithLabel(stmt->body, &ll);
  if (cv_.type == CompletionSpecification::kBreak) {
    if (ll.label == cv_.target) {
      cv_.type = CompletionSpecification::kNormal;
      cv_.target = nullptr;
    }
  }
}

void AstEvaluator::EvalStatement_(BreakStatement* stmt) {
  // 12.8 The break Statement
  if (!!stmt->label) {
    u16string n = EvalIdentifierToName(stmt->label);
    cv_.target = n;
  }
  cv_.type = CompletionSpecification::kBreak;
}

void AstEvaluator::EvalStatement_(ContinueStatement* stmt) {
  // 12.7 The continue Statement
  if (!!stmt->label) {
    u16string n = EvalIdentifierToName(stmt->label);
    cv_.target = n;
  }
  cv_.type = CompletionSpecification::kContinue;
}

void AstEvaluator::EvalStatement_(WithStatement* stmt) {
  // 12.10 The with Statement
  any_ref obj_val = EvalExpressionAndGetCompletion(stmt->object);
  if (!obj_val) return;
  Object* obj = ToObject(context_, obj_val);
  if (!obj) {
    cv_.type = CompletionSpecification::kThrow;
    return;
  }
  ObjectEnvironment* obj_env = ObjectEnvironment::Alloc(cur_env_);
  obj_env->bindings_obj = obj;
  obj_env->provide_this = true;
  Environment* old_env = cur_env_;
  cur_env_ = obj_env;
  EvalStatement(stmt->body);
  cur_env_ = old_env;
}

void AstEvaluator::EvalStatementWithLabel_(SwitchStatement* expr, const LabelList* label_list) {
  any_ref rval = EvalExpressionAndGetCompletion(expr->discriminant);
  if (!rval) return;
  auto it = expr->cases.begin();
  for (; it != expr->cases.end(); ++it) {
    SwitchCase* sc = *it;
    if (sc->test) {
      any_ref tval = EvalExpressionAndGetCompletion(sc->test);
      if (!tval) return;
      if (IsStrictSameValue(tval, rval))
        break;
    }
  }

  if (it == expr->cases.end()) {
    it = expr->cases.begin();
    for (; it != expr->cases.end(); ++it) {
      SwitchCase* sc = *it;
      if (!sc->test) break;
    }
  }
  
  for (; it != expr->cases.end(); ++it) {
    SwitchCase* sc = *it;
    EvalSwitchCase(sc);
    if (cv_.type == CompletionSpecification::kBreak) {
      if (CheckIfLabelMatch(label_list)) {
        cv_.type = CompletionSpecification::kNormal;
        cv_.target = nullptr;
        return;
      }
      return;
    } else if (cv_.type != CompletionSpecification::kNormal) {
      return;
    }
  }
}

void AstEvaluator::EvalStatement_(ReturnStatement* expr) {
  if (expr->argument) {
    any_ref rval = EvalExpressionAndGetCompletion(expr->argument);
    if (!rval) return;
    SetCurValue(rval);
  } else {
    SetCurValue(undefined_data::alloc());
  }
  cv_.type = CompletionSpecification::kReturn;
}

void AstEvaluator::EvalStatement_(ThrowStatement* expr) {
  any_ref v = EvalExpressionAndGetCompletion(expr->argument);
  if (!v) return;
  Throw(v);
  cv_.type = CompletionSpecification::kThrow;
}

void AstEvaluator::EvalStatement_(TryStatement* expr) {
  EvalStatement(expr->block);
  if (cv_.type == CompletionSpecification::kThrow) {
    cv_.type = CompletionSpecification::kNormal;
    CatchClause* handler = expr->handler;
    if (handler) EvalCatchClause(handler);
  }
  if (expr->finalizer) {
    CompletionSpecification::Type old_type = cv_.type;
    EvalStatement(expr->finalizer);
    cv_.type = old_type;
  }
}

void AstEvaluator::EvalStatementWithLabel_(WhileStatement* expr, const LabelList* label_list) {
  while (true) {
    any_ref tval = EvalExpressionAndGetCompletion(expr->test);
    if (!tval) return;
    if (!ToBoolean(tval)) break;
    EvalStatement(expr->body);
    if (!BreakOrContinueIfLabelMatch(label_list)) return;
  }
}

void AstEvaluator::EvalStatementWithLabel_(DoWhileStatement* expr, const LabelList* label_list) {
  while (true) {
    EvalStatement(expr->body);
    if (!BreakOrContinueIfLabelMatch(label_list)) return;    
    any_ref tval = EvalExpressionAndGetCompletion(expr->test);
    if (!tval) return;
    if (!ToBoolean(tval)) break;
  }
}

void AstEvaluator::EvalStatementWithLabel_(ForStatement* expr, const LabelList* label_list) {
  if (expr->init) {
    if (expr->init->type == SyntaxNode::kVariableDeclaration) {
      VariableDeclaration* decl = reinterpret_cast<VariableDeclaration*>(expr->init);
      EvalStatement(decl);
      if (cv_.type != CompletionSpecification::kNormal) return;
    } else {
      Expression* init = reinterpret_cast<Expression*>(expr->init);
      any_ref v = EvalExpressionAndGetCompletion(init);
      if (!v) return;
    }
  }

  while (true) {
    if (expr->test) {
      any_ref tval = EvalExpressionAndGetCompletion(expr->test);
      if (!tval) return;
      if (!ToBoolean(tval))
        break;
    }
    EvalStatement(expr->body);
    if (!BreakOrContinueIfLabelMatch(label_list)) return;
    if (expr->update) {
      any_ref v = EvalExpressionAndGetCompletion(expr->update);
      if (!v) return;
    }
  }
}

void AstEvaluator::EvalStatementWithLabel_(ForInStatement* expr, const LabelList* label_list) {
  // 12.6.4 The for-in Statement
  any_ref v = EvalExpressionAndGetCompletion(expr->right);
  if (!v) return;
  if (v.is_null() || v.is_undefined()) return;
  Object* o = ToObject(context_, v);
  if (!o) return;
  vector<u16string> names;
  names.init();
  for (auto it = o->own_props().begin(); it != o->own_props().end(); ++it) {
    Property* desc = &(*it).second;
    if (desc->flags & Property::kEnumerable) {
      names.push_back((*it).first);
    }
  }

  for (auto it = names.begin(); it != names.end(); ++it) {
    any_ref val = *it;
    if (expr->left->type == SyntaxNode::kVariableDeclaration) {
      VariableDeclaration* decl = reinterpret_cast<VariableDeclaration*>(expr->left);
      for (auto it = decl->declarations.begin(); it != decl->declarations.end(); ++it) {
        u16string name = EvalIdentifierToName((*it)->id);
        if (!ResolveIdentifierAndPutValue(name, val)) {
          cv_.type = CompletionSpecification::kThrow;
          return;
        }
      }
    } else if (expr->left->type == SyntaxNode::kIdentifier) {
      Identifier* ident = static_cast<Identifier*>(expr->left);
      u16string name = EvalIdentifierToName(ident);
      if (!ResolveIdentifierAndPutValue(name, val)) {
        cv_.type = CompletionSpecification::kThrow;
        return;
      }
    } else if (expr->left->type == SyntaxNode::kMemberExpression) {
      MemberExpression *member = static_cast<MemberExpression*>(expr->left);
      PropertyReference ref;
      if (!EvalMemberExpressionToReference(member, ref)) return;
      any_ref base_val = ref.base;
      Object* base_obj = ToObject(context_, base_val);
      if (!base_obj) return;
      if (!base_obj->Put(context_, ref.name, val, strict_)) return;
    }

    EvalStatement(expr->body);
    if (!BreakOrContinueIfLabelMatch(label_list)) return;
  }
}

void AstEvaluator::EvalStatement_(DebuggerStatement* expr) {
  assert(false);
}

// Declarations

void AstEvaluator::EvalStatement_(VariableDeclaration* expr) {
  for (auto it = expr->declarations.begin(); it != expr->declarations.end(); ++it) {
    if (!EvalVariableDeclarator(*it)) {
      cv_.type = CompletionSpecification::kThrow;
      break;
    }
  }
}

bool AstEvaluator::EvalVariableDeclarator(VariableDeclarator* decl) {
  if (decl->init) {
    u16string n = EvalIdentifierToName(decl->id);
    any_ref val = EvalExpressionToValue(decl->init);
    if (!val) return false;
    if (!PutValueWithEnvironment(cur_env_, n, val)) return false;
  }
  return true;
}

// Expressions

any_ref AstEvaluator::EvalExpressionToValue_(ThisExpression* expr) {
  // 11.1.1 The this Keyword
  return this_val_;
}

any_ref AstEvaluator::EvalExpressionToValue_(ArrayExpression* expr) {
  Object* robj = NewArrayObject(context_);
  int i = 0;
  for (auto it = expr->elements.begin(); it != expr->elements.end(); ++it) {
    any_ref v = EvalExpressionToValue(*it);
    if (!v) return nullptr;
    if (!robj->Put(context_, i, v, false)) return nullptr;
    i++;
  }
  return robj;
}

any_ref AstEvaluator::EvalExpressionToValue_(ObjectExpression* expr) {
  Object* o = Object::Alloc(context_->object_proto());
  for (auto it = expr->properties.begin(); it != expr->properties.end(); ++it) {
    PropertyNode* pa = *it;
    u16string n;
    if (pa->key->type == SyntaxNode::kIdentifier) {
      Identifier* ident = static_cast<Identifier*>(pa->key);
      n = EvalIdentifierToName(ident);
    } else {
      n = EvalExpressionToString(pa->key);
    }
    // pa->key is Identifier or Literal.
    assert(!!n);
    any_ref v = EvalExpressionToValue(pa->value);
    if (!v) return nullptr;
    switch (pa->kind) {
      case SyntaxNode::kPropertySet: {
        Property& desc = o->own_props()[n];
        desc.set = v;
        desc.flags = Property::kAccessor | Property::kEnumerable | Property::kConfigurable;
        break;
      }
      case SyntaxNode::kPropertyGet: {
        Property& desc = o->own_props()[n];
        desc.value_or_get = v;
        desc.flags = Property::kAccessor | Property::kEnumerable | Property::kConfigurable;
        break;
      }
      case SyntaxNode::kPropertyInit:
        if (!o->Put(context_, n, v, false)) return nullptr;
        break;
    }
  }
  return o;
}

any_ref AstEvaluator::EvalExpressionToValue_(FunctionExpression* expr) {
  return EvalFunctionNode(expr->function);
}

any_ref AstEvaluator::EvalExpressionToValue_(SequenceExpression* expr) {
  auto it = expr->expressions.begin();
  auto end = expr->expressions.end();
  any_ref v;
  while (it != end) {
    v = EvalExpressionToValue(*it);
    if (!v) return nullptr;
    ++it;
  }
  return v;
}

any_ref AstEvaluator::ApplyDeleteOperator(Expression* expr) {
  if (expr->type == SyntaxNode::kIdentifier) {
    Identifier* ident = static_cast<Identifier*>(expr);
    u16string n = EvalIdentifierToName(ident);
    return DeleteIdentifier(n);
  } else if (expr->type == SyntaxNode::kMemberExpression) {
    MemberExpression *member = static_cast<MemberExpression*>(expr);
    PropertyReference ref;
    if (!EvalMemberExpressionToReference(member, ref)) return nullptr;
    any_ref base_val = ref.base;
    Object* base_obj = ToObject(context_, base_val);
    if (!base_obj) return nullptr;
    any_ref val = false;
    if (!base_obj->Delete(context_, ref.name, strict_)) return nullptr;
    return val;
  } else {
    ThrowReferenceError(context_);
    return nullptr;
  }
}

any_ref AstEvaluator::ApplyTypeOfOperator(Expression* expr) {
  if (expr->type == SyntaxNode::kIdentifier) {
    Identifier* ident = static_cast<Identifier*>(expr);
    u16string n = EvalIdentifierToName(ident);
    Environment* env;
    any_ref rval = ResolveIdentifierAndGetValueAndEnvironment(n, false, env);
    if (!rval) return nullptr;
    return TypeOf(rval);
  } else {
    any_ref rval = EvalExpressionToValue(expr);
    if (!rval) return nullptr;
    return TypeOf(rval);
  }
}

any_ref AstEvaluator::EvalExpressionToValue_(UnaryExpression* expr) {
  if (expr->_operator == SyntaxNode::kUnaryDelete) {
    return ApplyDeleteOperator(expr->argument);
  } else if (expr->_operator == SyntaxNode::kUnaryTypeOf) {
    return ApplyTypeOfOperator(expr->argument);
  }

  any_ref val = EvalExpressionToValue(expr->argument);
  if (!val) return nullptr;
  switch (expr->_operator) {
    case SyntaxNode::kUnaryVoid:
      val = undefined_data::alloc();
      break;
    case SyntaxNode::kUnaryPositive: {
      double d;
      if (!ToNumber(context_, val, d)) return nullptr;
      val = d;
      break;
    }
    case SyntaxNode::kUnaryNegative: {
      double d;
      if (!ToNumber(context_, val, d)) return nullptr;
      val = -d;
      break;
    }
    case SyntaxNode::kUnaryBitwiseNot: {
      double d;
      if (!ToNumber(context_, val, d)) return nullptr;
      val = ~static_cast<int32_t>(d);
      break;
    }
    case SyntaxNode::kUnaryLogicalNot:
      val = !ToBoolean(val);
      break;
    default:
      assert(false);
  }
  return val;
}

inline static any_ref ApplyBinaryOperator(SyntaxNode::BinaryOperator optype, int l, int r) {
  any_ref val;
  switch (optype) {
    case SyntaxNode::kBinaryAddition:
      val = l + r;
      break;
    case SyntaxNode::kBinarySubtraction:
      val = l - r;
      break;
    case SyntaxNode::kBinaryMultiplication:
      val = l * r;
      break;
    case SyntaxNode::kBinaryRemainder:
      val = l % r;
      break;
    case SyntaxNode::kBinaryDivision:
      val = l / r;
      break;
    case SyntaxNode::kBinaryLeftShift:
      val = l << r;
      break;
    case SyntaxNode::kBinarySignedRightShift:
      val = l >> r;
      break;
    case SyntaxNode::kBinaryUnsignedRightShift:
      val = l >> r;
      break;
    case SyntaxNode::kBinaryLessThan:
      val = l < r;
      break;
    case SyntaxNode::kBinaryGreaterThan:
      val = l > r;
      break;
    case SyntaxNode::kBinaryLessThanOrEqual:
      val = l <= r;
      break;
    case SyntaxNode::kBinaryGreaterThanOrEqual:
      val = l >= r;
      break;
    case SyntaxNode::kBinaryEquals:
    case SyntaxNode::kBinaryStrictEquals:
      val = l == r;
      break;
    case SyntaxNode::kBinaryDoesNotEqual:
    case SyntaxNode::kBinaryStrictDoesNotEqual:
      val = l != r;
      break;
    case SyntaxNode::kBinaryBitwiseAnd:
      val = l & r;
      break;
    case SyntaxNode::kBinaryBitwiseXor:
      val = l ^ r;
      break;
    case SyntaxNode::kBinaryBitwiseOr:
      val = l | r;
      break;
    default:
      assert(false);
      val = 0;
      break;
  }
  return val;
}

inline static any_ref ApplyBinaryOperator(SyntaxNode::BinaryOperator optype, double l, double r) {
  any_ref val;
  switch (optype) {
    case SyntaxNode::kBinaryAddition:
      val = double_data::alloc(l + r);
      break;
    case SyntaxNode::kBinarySubtraction:
      val = double_data::alloc(l - r);
      break;
    case SyntaxNode::kBinaryMultiplication:
      val = double_data::alloc(l * r);
      break;
    case SyntaxNode::kBinaryDivision:
      val = double_data::alloc(l / r);
      break;
    case SyntaxNode::kBinaryRemainder:
      val = double_data::alloc(static_cast<int>(l) % static_cast<int>(r));
      break;
    case SyntaxNode::kBinaryLeftShift:
      val = double_data::alloc(static_cast<int>(l) << static_cast<int>(r));
      break;
    case SyntaxNode::kBinarySignedRightShift:
      val = double_data::alloc(static_cast<int>(l) >> static_cast<int>(r));
      break;
    case SyntaxNode::kBinaryUnsignedRightShift:
      val = double_data::alloc(static_cast<int>(l) >> static_cast<int>(r));
      break;
    case SyntaxNode::kBinaryLessThan:
      val = l < r;
      break;
    case SyntaxNode::kBinaryGreaterThan:
      val = l > r;
      break;
    case SyntaxNode::kBinaryLessThanOrEqual:
      val = l <= r;
      break;
    case SyntaxNode::kBinaryGreaterThanOrEqual:
      val = l >= r;
      break;
    case SyntaxNode::kBinaryEquals:
    case SyntaxNode::kBinaryStrictEquals:
      val = l == r;
      break;
    case SyntaxNode::kBinaryDoesNotEqual:
    case SyntaxNode::kBinaryStrictDoesNotEqual:
      val = l != r;
      break;
    case SyntaxNode::kBinaryBitwiseAnd:
      val = double_data::alloc(static_cast<int>(l) & static_cast<int>(r));
      break;
    case SyntaxNode::kBinaryBitwiseXor:
      val = double_data::alloc(static_cast<int>(l) ^ static_cast<int>(r));
      break;
    case SyntaxNode::kBinaryBitwiseOr:
      val = double_data::alloc(static_cast<int>(l) | static_cast<int>(r));
      break;
    case SyntaxNode::kBinaryInstanceOf:
    case SyntaxNode::kBinaryIn:
    default:
      assert(false);
      val = 0;
      break;
  }
  return val;
}

any_ref AstEvaluator::EvalExpressionToValue_(BinaryExpression* expr) {
  any_ref lval = EvalExpressionToValue(expr->left);
  if (!lval) return nullptr;
  any_ref rval = EvalExpressionToValue(expr->right);
  if (!rval) return nullptr;

  if (expr->_operator == SyntaxNode::kBinaryStrictEquals) {
    return IsStrictSameValue(lval, rval);
  } else if (expr->_operator == SyntaxNode::kBinaryStrictDoesNotEqual) {
    return !IsStrictSameValue(lval, rval);
  } else if (expr->_operator == SyntaxNode::kBinaryEquals) {
    bool bval;
    if (!IsAbstractSameValue(context_, lval, rval, bval)) return nullptr;
    return bval;
  } else if (expr->_operator == SyntaxNode::kBinaryDoesNotEqual) {
    bool bval;
    if (!IsAbstractSameValue(context_, lval, rval, bval)) return nullptr;
    return !bval;
  } else if (expr->_operator == SyntaxNode::kBinaryInstanceOf) {
    Object* lobj = ToObject(context_, lval);
    Object* robj = ToObject(context_, rval);
    any_ref robj_proto_val = robj->Get("prototype");
    Object* robj_proto = robj_proto_val.as<Object>();
    Object* proto = lobj->proto();
    while (proto) {
      if (proto == robj_proto) {
        return true;
      }
      proto = proto->proto();
    }
    return false;
  } else if (expr->_operator == SyntaxNode::kBinaryIn) {
    u16string n = ToString(context_, lval);
    Object* o = ToObject(context_, rval);
    bool bval = (o->own_props().find(n) != o->own_props().end());
    return bval;
  } else {
    any_ref val;
    lval = ToPrimitive(context_, lval);
    if (!lval) return nullptr;
    rval = ToPrimitive(context_, rval);
    if (!rval) return nullptr;

    if (expr->_operator == SyntaxNode::kBinaryAddition) {
      if (lval.is_u16string() || rval.is_u16string()) {
        u16string lstr = ToString(context_, lval);
        if (!lstr) return nullptr;
        u16string rstr = ToString(context_, rval);
        if (!rstr) return nullptr;
        return lstr + rstr;
      }
    }

    if (lval.is_smi() && rval.is_smi()) {
      val = ApplyBinaryOperator(expr->_operator, lval.smi(), rval.smi());
    } else if (lval.is_double() && rval.is_double()) {
      double lnum = lval.as_double();
      double rnum = rval.as_double();
      val = ApplyBinaryOperator(expr->_operator, lnum, rnum);
    } else {
      double lnum, rnum;
      if (!ToNumber(context_, lval, lnum) || !ToNumber(context_, rval, rnum)) return nullptr;
      val = ApplyBinaryOperator(expr->_operator, lnum, rnum);
    }
    return val;
  }
}

static any_ref ApplyAssignmentOperator(Context* c, SyntaxNode::AssignmentOperator op, any_ref lval, any_ref rval) {
  SyntaxNode::BinaryOperator bin_op;
  switch (op) {
    case SyntaxNode::kAssignmentMultiplication:
      bin_op = SyntaxNode::kBinaryMultiplication;
      break;
    case SyntaxNode::kAssignmentDivision:
      bin_op = SyntaxNode::kBinaryDivision;
      break;
    case SyntaxNode::kAssignmentRemainder:
      bin_op = SyntaxNode::kBinaryRemainder;
      break;
    case SyntaxNode::kAssignmentAddition:
      bin_op = SyntaxNode::kBinaryAddition;
      break;
    case SyntaxNode::kAssignmentSubtraction:
      bin_op = SyntaxNode::kBinarySubtraction;
      break;
    case SyntaxNode::kAssignmentLeftShift:
      bin_op = SyntaxNode::kBinaryLeftShift;
      break;
    case SyntaxNode::kAssignmentSignedRightShift:
      bin_op = SyntaxNode::kBinarySignedRightShift;
      break;
    case SyntaxNode::kAssignmentUnsignedRightShift:
      bin_op = SyntaxNode::kBinaryUnsignedRightShift;
      break;
    case SyntaxNode::kAssignmentBitwiseAnd:
      bin_op = SyntaxNode::kBinaryBitwiseAnd;
      break;
    case SyntaxNode::kAssignmentBitwiseXor:
      bin_op = SyntaxNode::kBinaryBitwiseXor;
      break;
    case SyntaxNode::kAssignmentBitwiseOr:
      bin_op = SyntaxNode::kBinaryBitwiseOr;
      break;
    case SyntaxNode::kAssignmentNone:
    default:
      assert(false);
      return nullptr;
  }
  any_ref val;

  if (bin_op == SyntaxNode::kBinaryAddition) {
    if (lval.is_u16string() || rval.is_u16string()) {
      u16string lstr = ToString(c, lval);
      if (!lstr) return nullptr;
      u16string rstr = ToString(c, rval);
      if (!rstr) return nullptr;
      return lstr + rstr;
    }
  }

  if (lval.is_smi() && rval.is_smi()) {
    return ApplyBinaryOperator(bin_op, lval.smi(), rval.smi());
  } else if (lval.is_double() && rval.is_double()) {
    double lnum = lval.as_double();
    double rnum = rval.as_double();
    return ApplyBinaryOperator(bin_op, lnum, rnum);
  } else {
    double lnum, rnum;
    if (!ToNumber(c, lval, lnum) || !ToNumber(c, rval, rnum)) {
      return nullptr;
    }
    return ApplyBinaryOperator(bin_op, lnum, rnum);
  }
}

any_ref AstEvaluator::EvalExpressionToValue_(AssignmentExpression* expr) {
  if (expr->left->type == SyntaxNode::kIdentifier) {
    Identifier* ident = static_cast<Identifier*>(expr->left);
    u16string n = EvalIdentifierToName(ident);
    if (expr->_operator == SyntaxNode::kAssignmentNone) {
      any_ref rval = EvalExpressionToValue(expr->right);
      if (!rval) return nullptr;
      if (!ResolveIdentifierAndPutValue(n, rval)) return nullptr;
      return rval;
    } else {
      Environment* env;
      any_ref lval = ResolveIdentifierAndGetValueAndEnvironment(n, true, env);
      if (!lval) return nullptr;
      any_ref rval = EvalExpressionToValue(expr->right);
      if (!rval) return nullptr;
      any_ref val = ApplyAssignmentOperator(context_, expr->_operator, lval, rval);
      if (!val) return nullptr;
      if (!PutValueWithEnvironment(env, n, val)) return nullptr;
      return val;
    }
  } else if (expr->left->type == SyntaxNode::kMemberExpression) {
    MemberExpression *member = static_cast<MemberExpression*>(expr->left);
    PropertyReference ref;
    if (!EvalMemberExpressionToReference(member, ref)) return nullptr;
    any_ref base_val = ref.base;
    Object* base_obj = ToObject(context_, base_val);
    if (!base_obj) return nullptr;
    any_ref val;
    if (expr->_operator == SyntaxNode::kAssignmentNone) {
      any_ref rval = EvalExpressionToValue(expr->right);
      if (!rval) return nullptr;
      val = rval;
    } else {
      any_ref lval = base_obj->Get(ref.name);
      if (!lval) return nullptr;
      any_ref rval = EvalExpressionToValue(expr->right);
      if (!rval) return nullptr;
      val = ApplyAssignmentOperator(context_, expr->_operator, lval, rval);
      if (!val) return nullptr;
    }
    if (!base_obj->Put(context_, ref.name, val, strict_)) return nullptr;
    return val;
  } else {
    ThrowReferenceError(context_);
    return nullptr;
  }
}

static any_ref ApplyUpdateOperator(SyntaxNode::UpdateOperator op, any_ref val) {
  if (val.is_smi()) {
    switch (op) {
      case SyntaxNode::kUpdateIncrement:
        val = val.smi() + 1;
        break;
      case SyntaxNode::kUpdateDecrement:
        val = val.smi() - 1;
        break;
      default:
        assert(false);
        val = 0;
        break;
    }
  } else {
    double num = val.as_double();
    switch (op) {
      case SyntaxNode::kUpdateIncrement:
        val = double_data::alloc(num + 1);
        break;
      case SyntaxNode::kUpdateDecrement:
        val = double_data::alloc(num - 1);
        break;
      default:
        assert(false);
        val = 0;
        break;
    }
  }
  return val;
}

any_ref AstEvaluator::EvalExpressionToValue_(UpdateExpression* expr) {
  if (expr->argument->type == SyntaxNode::kIdentifier) {
    Identifier* ident = static_cast<Identifier*>(expr->argument);
    u16string n = EvalIdentifierToName(ident);
    Environment* env;
    any_ref old_val = ResolveIdentifierAndGetValueAndEnvironment(n, true, env);
    if (!old_val) return nullptr;
    any_ref val = ApplyUpdateOperator(expr->_operator, old_val);
    if (!val) return nullptr;
    if (!PutValueWithEnvironment(env, n, val)) return nullptr;
    return expr->prefix ? val : old_val;
  } else if (expr->argument->type == SyntaxNode::kMemberExpression) {
    MemberExpression *member = static_cast<MemberExpression*>(expr->argument);
    PropertyReference ref;
    if (!EvalMemberExpressionToReference(member, ref)) return nullptr;
    any_ref base_val = ref.base;
    Object* base_obj = ToObject(context_, base_val);
    if (!base_obj) return nullptr;
    any_ref old_val = base_obj->Get(ref.name);
    if (!old_val) return nullptr;
    any_ref val = ApplyUpdateOperator(expr->_operator, old_val);
    if (!val) return nullptr;
    if (!base_obj->Put(context_, ref.name, val, strict_)) return nullptr;
    return expr->prefix ? val : old_val;
  } else {
    return ThrowReferenceError(context_);
  }
}

any_ref AstEvaluator::EvalExpressionToValue_(LogicalExpression* expr) {
  // 11.11 Binary Logical Operators
  assert(expr->_operator == SyntaxNode::kLogicalAnd ||
         expr->_operator == SyntaxNode::kLogicalOr);
  any_ref lval = EvalExpressionToValue(expr->left);
  if (!lval) return nullptr;
  bool bval = ToBoolean(lval);
  if (expr->_operator == SyntaxNode::kLogicalAnd) bval = !bval;
  if (bval) return lval;
  return EvalExpressionToValue(expr->right);
}
      
any_ref AstEvaluator::EvalExpressionToValue_(ConditionalExpression* expr) {
  // 11.12 Conditional Operator ( ? : )
  any_ref tval = EvalExpressionToValue(expr->test);
  if (!tval) return nullptr;
  if (ToBoolean(tval)) {
    return EvalExpressionToValue(expr->consequent);
  } else {
    return EvalExpressionToValue(expr->alternate);
  }
}

any_ref AstEvaluator::EvalExpressionToValue_(NewExpression* expr) {
  // 11.2.2 The new Operator
  any_ref func_val = EvalExpressionToValue(expr->callee);
  if (!func_val) return nullptr;
  if (!func_val.is<Object>()) {
    return ThrowTypeError(context_);
  }
  Object* func_obj = func_val.as<Object>();
  if (!IsCallable(func_obj)) {
    return ThrowTypeError(context_);
  }

  // 11.2.4 Argument Lists
  any_vector args;
  args.init();
  args.push_back(nullptr);
  for (auto it = expr->arguments.begin(); it != expr->arguments.end(); ++it) {
    any_ref val = EvalExpressionToValue(*it);
    if (!val) return nullptr;
    args.push_back(val);
  }

  return func_obj->Construct(args.size(), args.data());
}

any_ref AstEvaluator::EvalExpressionToValue_(CallExpression* expr) {
  // 11.2.3 Function Calls
  assert(expr->callee);
  any_ref this_val;
  any_ref func_val;
  if (expr->callee->type == SyntaxNode::kIdentifier) {
    Identifier* ident = static_cast<Identifier*>(expr->callee);
    u16string n = EvalIdentifierToName(ident);
    Environment* env;
    func_val = ResolveIdentifierAndGetValueAndEnvironment(n, true, env);
    if (!func_val) return nullptr;
    if (cur_env_->is<DeclarativeEnvironment>()) {
      this_val = context_->global_obj();
    } else {
      assert(env->is<ObjectEnvironment>());
      ObjectEnvironment* obj_env = static_cast<ObjectEnvironment*>(env);
      if (obj_env->provide_this) {
        this_val = obj_env->bindings_obj;
      } else {
        this_val = undefined_data::alloc();
      }
    }
  } else if (expr->callee->type == SyntaxNode::kMemberExpression) {
    MemberExpression *member = static_cast<MemberExpression*>(expr->callee);
    PropertyReference ref;
    if (!EvalMemberExpressionToReference(member, ref)) return nullptr;
    this_val = ref.base;
    Object* this_obj = ToObject(context_, this_val);
    if (!this_obj) return nullptr;
    func_val = this_obj->Get(ref.name);
    if (!func_val) return nullptr;
  } else {
    func_val = EvalExpressionToValue(expr->callee);
    if (!func_val) return nullptr;
    this_val = undefined_data::alloc();
  }
  
  if (!func_val.is<Object>()) return ThrowTypeError(context_);
  Object* func_obj = func_val.as<Object>();
  if (!IsCallable(func_obj)) return ThrowTypeError(context_);

  // 11.2.4 Argument Lists
  any_vector args;
  args.init();
  args.push_back(this_val);
  for (auto it = expr->arguments.begin(); it != expr->arguments.end(); ++it) {
    any_ref val = EvalExpressionToValue(*it);
    if (!val) return nullptr;
    args.push_back(val);
  }

  return func_obj->Call(args.size(), args.data());
}

any_ref AstEvaluator::EvalExpressionToValue_(MemberExpression* expr)
{
  PropertyReference ref;
  if (!EvalMemberExpressionToReference(expr, ref)) return nullptr;
  any_ref base_val = ref.base;
  Object* base_obj = ToObject(context_, base_val);
  if (!base_obj) return nullptr;
  return base_obj->Get(ref.name);
}

// Clauses

void AstEvaluator::EvalSwitchCase(SwitchCase* expr) {
  for (auto it = expr->consequent.begin(); it != expr->consequent.end(); ++it) {
    EvalStatement(*it);
    if (cv_.type != CompletionSpecification::kNormal)
      return;
  }
}

void AstEvaluator::EvalCatchClause(CatchClause* expr) {
  any_ref rval = Catch();
  assert(!!rval);
  cv_.value = rval;
  u16string id = EvalIdentifierToName(expr->param);
  DefineVariable(id, rval);
  EvalStatement(expr->body);
}

// Literals

/**
 * @returns false for exception.
 */
bool AstEvaluator::ResolveIdentifierAndPutValue(u16string n, any_ref v) {
  // 10.3.1 Identifier Resolution
  // 10.2.2.1 GetIdentifierReference (lex, name, strict)
  Environment* env = cur_env_;
  while (env) {
    if (env->is<DeclarativeEnvironment>()) {
      DeclarativeEnvironment* decl_env = static_cast<DeclarativeEnvironment*>(env);
      bool found;
      if (!decl_env->SetMutableBindingIfFound(context_, n, v, strict_, found)) return false;
      if (found) return true;
    } else {
      assert(env->is<ObjectEnvironment>());
      ObjectEnvironment* obj_env = static_cast<ObjectEnvironment*>(env);
      bool found;
      if (!obj_env->SetMutableBindingIfFound(context_, n, v, strict_, found)) return false;
      if (found) return true;
    }
    env = env->outer;
  }
  // Do PutValue()
  if (strict_) {
    return ThrowReferenceError(context_);
  } else {
    Object* obj = context_->global_obj();
    return obj->Put(context_, n, v, false);
  }
}

bool AstEvaluator::DeleteIdentifier(u16string n) {
  Environment* env = cur_env_;
  if (env->is<DeclarativeEnvironment>()) {
    DeclarativeEnvironment* decl_env = static_cast<DeclarativeEnvironment*>(env);
    return decl_env->DeleteBinding(n);
  } else {
    assert(env->is<ObjectEnvironment>());
    ObjectEnvironment* obj_env = static_cast<ObjectEnvironment*>(env);
    return obj_env->DeleteBinding(n);
  }
}

any_ref AstEvaluator::ResolveIdentifierAndGetValueAndEnvironment(u16string n, bool do_throw, Environment*& resolved_env) {
  // 10.3.1 Identifier Resolution
  // 10.2.2.1 GetIdentifierReference (lex, name, strict)
  Environment* env = cur_env_;
  while (env) {
    if (env->is<DeclarativeEnvironment>()) {
      DeclarativeEnvironment* decl_env = static_cast<DeclarativeEnvironment*>(env);
      Binding* b = decl_env->GetBinding(n);
      if (!!b) {
        // HasBinding() is true.
        resolved_env = env;
        // Do GetBindingValue().
        return b->value;
      }
    } else {
      assert(env->is<ObjectEnvironment>());
      ObjectEnvironment* obj_env = static_cast<ObjectEnvironment*>(env);
      Object* bindings_obj = obj_env->bindings_obj;
      Property* prop = bindings_obj->GetProperty(n);
      if (!!prop) {
        // HasBinding() is true.
        resolved_env = env;
        // Do GetBindingValue().
        return bindings_obj->Get(n);
      }
    }
    env = env->outer;
  }
  if (do_throw) {
    return ThrowReferenceError(context_);
  } else {
    // IsUnresolvableReference(V) is true.
    // GetValue(V) throws a ReferenceError.
    return undefined_data::alloc();
  }
}

bool AstEvaluator::PutValueWithEnvironment(Environment* env, u16string n, any_ref v) {
  if (env->is<DeclarativeEnvironment>()) {
    DeclarativeEnvironment* decl_env = static_cast<DeclarativeEnvironment*>(env);
    bool found;
    return decl_env->SetMutableBindingIfFound(context_, n, v, strict_, found);
  } else {
    assert(env->is<ObjectEnvironment>());
    ObjectEnvironment* obj_env = static_cast<ObjectEnvironment*>(env);
    return obj_env->SetMutableBinding(context_, n, v, strict_);
  }
}

bool AstEvaluator::EvalMemberExpressionToReference(MemberExpression* expr, PropertyReference& ref) {
  // 11.2.1 Property Accessors
  any_ref object_val = EvalExpressionToValue(expr->object);
  if (!object_val) return false;
  u16string property_str;
  if (expr->computed) {
    property_str = EvalExpressionToString(expr->property);
    if (!property_str) return false;
  } else {
    assert(expr->property->type == SyntaxNode::kIdentifier);
    Identifier* ident = static_cast<Identifier*>(expr->property);
    property_str = EvalIdentifierToName(ident);
    assert(!!property_str);
  }
  if (!CheckObjectCoercible(context_, object_val)) return false;
  ref.base = object_val;
  ref.name = property_str;
  ref.strict = false;
  return true;
}

any_ref AstEvaluator::EvalExpressionToValue_(Identifier* expr) {
  // 11.1.2 Identifier Reference
  u16string n = EvalIdentifierToName(expr);
  Environment* env;
  return ResolveIdentifierAndGetValueAndEnvironment(n, true, env);
}

any_ref AstEvaluator::EvalExpressionToValue_(NullLiteral* expr) {
  return null_data::alloc();
}

any_ref AstEvaluator::EvalExpressionToValue_(BooleanLiteral* expr) {
  return expr->value;
}

any_ref AstEvaluator::EvalExpressionToValue_(NumberLiteral* expr) {
  int n = static_cast<int>(expr->value);
  if (any_ref(n).smi() == expr->value) {
    return n;
  } else {
    return expr->value;
  }
}

any_ref AstEvaluator::EvalExpressionToValue_(StringLiteral* expr) {
  if (expr->value == 0) {
    char16_t ch = 0;
    return u16string(&ch, 0);
  } else {
    return u16string(script_->string_table()[expr->value]);
  }
}

any_ref AstEvaluator::EvalExpressionToValue_(RegExpLiteral* expr) {
  u16string pattern_str = u16string(expr->pattern.data(), expr->pattern.length());
  u16string flags_str = u16string(expr->flags.data(), expr->flags.length());
  return NewRegExpObject(context_, pattern_str, flags_str);
}

// Misc

any_ref AstEvaluator::EvalExpressionToValue(Expression* expr) {
  assert(expr);
  any_ref v;
  switch (expr->type) {
    case SyntaxNode::kThisExpression:
      v = EvalExpressionToValue_(static_cast<ThisExpression*>(expr));
      break;
    case SyntaxNode::kArrayExpression:
      v = EvalExpressionToValue_(static_cast<ArrayExpression*>(expr));
      break;
    case SyntaxNode::kObjectExpression:
      v = EvalExpressionToValue_(static_cast<ObjectExpression*>(expr));
      break;
    case SyntaxNode::kFunctionExpression:
      v = EvalExpressionToValue_(static_cast<FunctionExpression*>(expr));
      break;
    case SyntaxNode::kSequenceExpression:
      v = EvalExpressionToValue_(static_cast<SequenceExpression*>(expr));
      break;
    case SyntaxNode::kUnaryExpression:
      v = EvalExpressionToValue_(static_cast<UnaryExpression*>(expr));
      break;
    case SyntaxNode::kBinaryExpression:
      v = EvalExpressionToValue_(static_cast<BinaryExpression*>(expr));
      break;
    case SyntaxNode::kAssignmentExpression:
      v = EvalExpressionToValue_(static_cast<AssignmentExpression*>(expr));
      break;
    case SyntaxNode::kUpdateExpression:
      v = EvalExpressionToValue_(static_cast<UpdateExpression*>(expr));
      break;
    case SyntaxNode::kLogicalExpression:
      v = EvalExpressionToValue_(static_cast<LogicalExpression*>(expr));
      break;
    case SyntaxNode::kConditionalExpression:
      v = EvalExpressionToValue_(static_cast<ConditionalExpression*>(expr));
      break;
    case SyntaxNode::kNewExpression:
      v = EvalExpressionToValue_(static_cast<NewExpression*>(expr));
      break;
    case SyntaxNode::kCallExpression:
      v = EvalExpressionToValue_(static_cast<CallExpression*>(expr));
      break;
    case SyntaxNode::kMemberExpression:
      v = EvalExpressionToValue_(static_cast<MemberExpression*>(expr));
      break;
    case SyntaxNode::kIdentifier:
      v = EvalExpressionToValue_(static_cast<Identifier*>(expr));
      break;
    case SyntaxNode::kNullLiteral:
      v = EvalExpressionToValue_(static_cast<NullLiteral*>(expr));
      break;
    case SyntaxNode::kBooleanLiteral:
      v = EvalExpressionToValue_(static_cast<BooleanLiteral*>(expr));
      break;
    case SyntaxNode::kNumberLiteral:
      v = EvalExpressionToValue_(static_cast<NumberLiteral*>(expr));
      break;
    case SyntaxNode::kStringLiteral:
      v = EvalExpressionToValue_(static_cast<StringLiteral*>(expr));
      break;
    case SyntaxNode::kRegExpLiteral:
      v = EvalExpressionToValue_(static_cast<RegExpLiteral*>(expr));
      break;
    default:
      assert(false);
      break;
  }

  return v;
}

u16string AstEvaluator::EvalExpressionToString(Expression *expr) {
  any_ref v = EvalExpressionToValue(expr);
  if (!v) return nullptr;
  return ToString(context_, v);
}

any_ref AstEvaluator::EvalExpressionAndGetCompletion(Expression* expr) {
  any_ref v = EvalExpressionToValue(expr);
  if (!v) {
    cv_.type = CompletionSpecification::kThrow;
  }
  return v;
}

any_ref AstEvaluator::CallFunction(Context* context, Script* script, Environment* scope, FunctionNode* expr, bool strict, any_ref this_val, size_t argc, const any_ref* argv) {
  AstEvaluator evaluator(context, script, this_val, strict);
  return evaluator.CallFunction_(scope, expr, argc, argv);
}

void AstEvaluator::InitFunctionBindings(std::vector<Statement*>& body) {
  for (auto it = body.begin(); it != body.end(); ++it) {
    InitFunctionBindings(*it);
  }
}

void AstEvaluator::InitFunctionBindings(Statement* stmt) {
  if (!stmt) return;
  switch (stmt->type) {
    case SyntaxNode::kBlockStatement:
      InitFunctionBindings(static_cast<BlockStatement*>(stmt)->body);
      break;
    case SyntaxNode::kIfStatement:
      InitFunctionBindings(static_cast<IfStatement*>(stmt)->consequent);
      InitFunctionBindings(static_cast<IfStatement*>(stmt)->alternate);
      break;
    case SyntaxNode::kWithStatement:
      InitFunctionBindings(static_cast<WithStatement*>(stmt)->body);
      break;
    case SyntaxNode::kSwitchStatement:
      // InitFunctionBindings(static_cast<SwitchStatement*>(stmt));
      break;
    case SyntaxNode::kTryStatement:
      InitFunctionBindings(static_cast<TryStatement*>(stmt));
      break;
    case SyntaxNode::kWhileStatement:
      InitFunctionBindings(static_cast<WhileStatement*>(stmt)->body);
      break;
    case SyntaxNode::kDoWhileStatement:
      InitFunctionBindings(static_cast<DoWhileStatement*>(stmt)->body);
      break;
    case SyntaxNode::kForStatement:
      InitFunctionBindings(static_cast<ForStatement*>(stmt)->body);
      break;
    case SyntaxNode::kForInStatement:
      InitFunctionBindings(static_cast<ForInStatement*>(stmt)->body);
      break;
    case SyntaxNode::kFunctionDeclaration: {
      InitFunctionBindings(static_cast<FunctionDeclaration*>(stmt));
      break;
    }
    default:
      break;
  }
}

void AstEvaluator::InitFunctionBindings(TryStatement* stmt) {
  InitFunctionBindings(stmt->block);
  if (stmt->handler) {
    InitFunctionBindings(stmt->handler->body);
  }
  InitFunctionBindings(stmt->finalizer);
}

void AstEvaluator::InitFunctionBindings(FunctionDeclaration* stmt) {
  FunctionNode *node = stmt->function;
  any_ref val = EvalFunctionNode(node);
  assert(!!val);
  u16string n = EvalIdentifierToName(node->id);
  assert(!!n);
  DefineVariable(n, val);
}

void AstEvaluator::InitVariableBindings(std::vector<Statement*>& body) {
  for (auto it = body.begin(); it != body.end(); ++it) {
    InitVariableBindings(*it);
  }
}

void AstEvaluator::InitVariableBindings(Statement* stmt) {
  if (!stmt) return;
  switch (stmt->type) {
    case SyntaxNode::kBlockStatement:
      InitVariableBindings(static_cast<BlockStatement*>(stmt)->body);
      break;
    case SyntaxNode::kLabeledStatement:
      InitVariableBindings(static_cast<LabeledStatement*>(stmt)->body);
      break;
    case SyntaxNode::kIfStatement:
      InitVariableBindings(static_cast<IfStatement*>(stmt)->consequent);
      InitVariableBindings(static_cast<IfStatement*>(stmt)->alternate);
      break;
    case SyntaxNode::kWithStatement:
      InitVariableBindings(static_cast<WithStatement*>(stmt)->body);
      break;
    case SyntaxNode::kSwitchStatement:
      // InitVariableBindings(static_cast<SwitchStatement*>(stmt));
      break;
    case SyntaxNode::kTryStatement:
      InitVariableBindings(static_cast<TryStatement*>(stmt));
      break;
    case SyntaxNode::kWhileStatement:
      InitVariableBindings(static_cast<WhileStatement*>(stmt)->body);
      break;
    case SyntaxNode::kDoWhileStatement:
      InitVariableBindings(static_cast<DoWhileStatement*>(stmt)->body);
      break;
    case SyntaxNode::kForStatement:
      InitVariableBindings(static_cast<ForStatement*>(stmt));
      break;
    case SyntaxNode::kForInStatement:
      InitVariableBindings(static_cast<ForInStatement*>(stmt));
      break;
    case SyntaxNode::kVariableDeclaration:
      InitVariableBindings(static_cast<VariableDeclaration*>(stmt));
      break;
    default:
      break;
  }
}

void AstEvaluator::InitVariableBindings(TryStatement* stmt) {
  InitVariableBindings(stmt->block);
  if (stmt->handler) {
    InitVariableBindings(stmt->handler->body);
  }
  InitVariableBindings(stmt->finalizer);
}

void AstEvaluator::InitVariableBindings(ForStatement* stmt) {
  if (stmt->init) {
    if (stmt->init->type == SyntaxNode::kVariableDeclaration) {
      VariableDeclaration* decl = static_cast<VariableDeclaration*>(stmt->init);
      InitVariableBindings(decl);
    }
  }
  InitVariableBindings(stmt->body);
}

void AstEvaluator::InitVariableBindings(ForInStatement* stmt) {
  if (stmt->left) {
    if (stmt->left->type == SyntaxNode::kVariableDeclaration) {
      VariableDeclaration* decl = static_cast<VariableDeclaration*>(stmt->left);
      InitVariableBindings(decl);
    }
  }
  InitVariableBindings(stmt->body);
}

void AstEvaluator::InitVariableBindings(VariableDeclaration* stmt) {
  for (auto it = stmt->declarations.begin(); it != stmt->declarations.end(); ++it) {
    VariableDeclarator* decl = *it;
    u16string n = EvalIdentifierToName(decl->id);
    DefineVariable(n, undefined_data::alloc());
  }
}

bool AstEvaluator::CheckIfLabelMatch(const LabelList* ll) {
  if (!cv_.target) return true;

  while (!!ll) {
    if (ll->label == cv_.target) return true;
    ll = ll->next;
  }

  return false;
}

bool AstEvaluator::BreakOrContinueIfLabelMatch(const LabelList* ll) {
  if (cv_.type == CompletionSpecification::kBreak) {
    if (CheckIfLabelMatch(ll)) {
      cv_.type = CompletionSpecification::kNormal;
      cv_.target = nullptr;
      return false;
    }
  } else if (cv_.type == CompletionSpecification::kContinue) {
    if (CheckIfLabelMatch(ll)) {
      cv_.type = CompletionSpecification::kNormal;
      cv_.target = nullptr;
      return true;
    }
    return false;
  } else if (cv_.type != CompletionSpecification::kNormal) {
    return false;
  }
  return true;
}

any_ref AstEvaluator::CallFunction_(Environment* scope, FunctionNode* expr, size_t argc, const any_ref* argv) {
  // 10.4.3 Entering Function Code
  DeclarativeEnvironment* env = DeclarativeEnvironment::Alloc(scope);
  cur_env_ = env;

  // 10.5 Declaration Binding Instantiation
  Object* arguments_obj = Object::Alloc(context_->object_proto());

  auto it = expr->params.begin();
  auto end = expr->params.end();
  size_t i = 0;
  for (; it != end && i < argc; ++it, i++) {
    u16string n = EvalIdentifierToName(*it);
    // CreateBinding() does nothing if there's already a binding.
    env->CreateBinding(n, argv[i], false, false);
    arguments_obj->Put(context_, i, argv[i], false);
  }
  for (; i < argc; i++) {
    arguments_obj->Put(context_, i, argv[i], false);
  }
  for (; it != end; ++it) {
    u16string n = EvalIdentifierToName(*it);
    env->CreateBinding(n, undefined_data::alloc(), false, false);
  }

  arguments_obj->Put(context_, "length", (int)argc, false);
  // arguments is immutable in strict mode.
  env->CreateBinding("arguments", arguments_obj, strict_, false);

  InitFunctionBindings(expr->body);
  InitVariableBindings(expr->body);

  EvalStatement(expr->body);

  if (cv_.type == CompletionSpecification::kNormal) {
    return undefined_data::alloc();
  } else if (cv_.type == CompletionSpecification::kReturn) {
    return GetCurValue();
  } else if (cv_.type == CompletionSpecification::kThrow) {
    return nullptr;
  } else {
    // kContinue or kBreak;
    return ThrowSyntaxError(context_);
  }
}

}  // namespace internal
}  // namespace nabla
