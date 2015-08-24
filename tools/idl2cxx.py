# Nabla JS - A small EMCAScript interpreter with straight-forward implementation.
# Copyright (C) 2014 Katsuya Iida. All rights reserved.

import re

idl_filename = 'astnode.idl'
type_filename = 'astnode_type.cc'
case_filename = 'astnode_case.cc'
decl_filename = 'astnode_impl.hh'
impl_filename = 'astnode_impl.cc'

class NodeType:
    def __init__(self, name, base_name):
        self.name = name
        self.base_name = base_name
        self.fields = []

class Field:
    def __init__(self, name, type_name, is_nullable, is_array):
        self.name = name
        self.type_name = type_name
        self.is_nullable = is_nullable
        self.is_array = is_array

scalar_names = [
    'boolean',
    'PropertyKind',
    'UpdateOperator',
    'AssignmentOperator',
    'LogicalOperator',
    'BinaryOperator',
    'UnaryOperator'
    ]

def is_scalar_name(name):
    return name in scalar_names

def to_c_field(name):
    if name == 'operator':
        return '_operator'
    return name

def to_c_type(name):
    if name == 'Node':
        return 'SyntaxNode'
    elif name == 'boolean':
        return 'bool'
    elif name == 'Function':
        return 'FunctionNode'
    elif name == 'Property':
        return 'PropertyNode'
    return name

nodes = []

start_re = re.compile('^\s*interface\s+(\S+)\s*<:\s*(\S+)\s*{\s*(})?\s*$')
type_re = re.compile('^\s*type\s*:')
field_re = re.compile('^\s*(\S+)\s*:\s*(([^][\s]+)(\s*\|\s*null)?|\[\s*([^][\s]+)(\s*\|\s*null)?\s*\])\s*;\s*$')
end_re = re.compile('^\s*}\s*$')

state = 0
cur_node = None
with open(idl_filename, 'r') as f:
    for line in f:
        if not cur_node:
            result = start_re.match(line)
            if result:
                is_empty = result.group(3)
                if not is_empty:
                    name = result.group(1)
                    base_name = result.group(2)
                    cur_node = NodeType(name, base_name)
                    nodes.append(cur_node)
        else:
            result = end_re.match(line)
            if result:
                cur_node = None
                continue
            result = type_re.match(line)
            if result:
                continue
            result = field_re.match(line)
            if result:
                name = result.group(1)
                type_name = result.group(3)
                if type_name:
                    is_array = False
                else:
                    is_array = True
                    type_name = result.group(5)
                field = Field(name, type_name, False, is_array)
                cur_node.fields.append(field)
            else:
                raise Exception("error" + line)

extnodes = []
for node in nodes:
    extnodes.append(node.name)
extnodes.append('Identifier')
extnodes.append('NullLiteral')
extnodes.append('BooleanLiteral')
extnodes.append('NumberLiteral')
extnodes.append('StringLiteral')
extnodes.append('RegExpLiteral')

with open(type_filename, 'w') as f:
    print >>f, '// Automatically generated from %s' % (idl_filename)
    print >>f

    print >>f, 'enum SyntaxNodeType {'
    for nodename in extnodes:
        print >>f, '  k%s,' % (nodename)
    print >>f, '};'
    print >>f
    print >>f, '// EOF'


with open(case_filename, 'w') as f:
    print >>f, '// Automatically generated from %s' % (idl_filename)
    print >>f

    for nodename in extnodes:
        print >>f, '    case SyntaxNode::k%s:' % (nodename)
        print >>f, '      EvalSyntaxNode(static_cast<%s*>(expr));' % (to_c_type(nodename))
        print >>f, '      break;'

    print >>f
    print >>f, '// EOF'

with open(decl_filename, 'w') as f:
    print >>f, '// Automatically generated from %s' % (idl_filename)
    print >>f

    for node in nodes:
        print >>f, 'class %s : public %s {' % (to_c_type(node.name), to_c_type(node.base_name))
        print >>f, ' public:'
        arg = ''
        init = ''
        for field in node.fields:
            # Skip 'type'. It is not needed.
            if field.name == 'type':
                continue

            c_field_type = to_c_type(field.type_name)
            c_field_name = to_c_field(field.name)

            # Contructor arguments
            if is_scalar_name(field.type_name):
                arg = arg + ', %s %s' % (c_field_type, c_field_name)
            elif field.is_array:
                arg = arg + ', std::vector<%s*>* %s' % (c_field_type, c_field_name)
            else:
                arg = arg + ', %s* %s' % (c_field_type, c_field_name)

        print >>f, '  %s(const SourceLocation& loc%s);' % (to_c_type(node.name), arg)
        
        print >>f, '  ~%s();' % (to_c_type(node.name))

        for field in node.fields:
            if c_field_name == 'type':
                continue

            c_field_type = to_c_type(field.type_name)
            c_field_name = to_c_field(field.name)

            if is_scalar_name(field.type_name):
                print >>f, '  %s %s;' % (c_field_type, c_field_name)
            elif field.is_array:
                print >>f, '  std::vector<%s*> %s;' % (c_field_type, c_field_name)
            else:
                print >>f, '  %s* %s;' % (c_field_type, c_field_name)
        print >>f, '};'
        print >>f
    print >>f, '// EOF'

with open(impl_filename, 'w') as f:
    print >>f, '// Automatically generated from %s' % (idl_filename)
    print >>f

    for node in nodes:
        arg = ''
        init = ''
        for field in node.fields:
            # Skip 'type'. It is not needed.
            if field.name == 'type':
                continue

            c_field_type = to_c_type(field.type_name)
            c_field_name = to_c_field(field.name)

            # Contructor arguments
            if is_scalar_name(field.type_name):
                arg = arg + ', %s %s' % (c_field_type, c_field_name)
            elif field.is_array:
                arg = arg + ', std::vector<%s*>* %s' % (c_field_type, c_field_name)
            else:
                arg = arg + ', %s* %s' % (c_field_type, c_field_name)

        print >>f, '%s::%s(const SourceLocation& loc%s)' % (to_c_type(node.name), to_c_type(node.name), arg)
        print >>f, '    : %s(k%s, loc) {' % (to_c_type(node.base_name), node.name)
        
        for field in node.fields:
            if c_field_name == 'type':
                continue

            c_field_name = to_c_field(field.name)

            # Constructor initializer
            if field.is_array:
                print >>f, '  if (%s) { this->%s = std::move(*%s); delete %s; }' % (c_field_name, c_field_name, c_field_name, c_field_name)
            else:
                print >>f, '  this->%s = %s;' % (c_field_name, c_field_name)
        print >>f, '}'
        print >>f
        
        print >>f, '%s::~%s() {' % (to_c_type(node.name), to_c_type(node.name))
        for field in node.fields:
            if c_field_name == 'type':
                continue

            c_field_name = to_c_field(field.name)

            if field.is_array:
                print >>f, '  delete_syntax_node_vector(%s);' % (c_field_name)
            elif not is_scalar_name(field.type_name):
                print >>f, '  delete %s;' % (c_field_name)
        print >>f, '}'
        print >>f
    print >>f, '// EOF'
