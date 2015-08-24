var a = {};
var b = {};
print(a === a);
print(a === b);
print(a !== a);
print(a !== b);

var a = "a";
var a2 = "a";
var b = "b";
print(a === a);
print(a === a2);
print(a === b);
print(a !== a);
print(a !== a2);
print(a !== b);

print(1 + 2 - 3 ? 1 + 2 : 3 + 4);
print(1 + 2 - 2 ? 1 + 2 : 3 + 4);
print(this ? 1 + 2 : 3 + 4);
print(undefined ? 1 + 2 : 3 + 4);
(true ? this : undefined).print("OK");
var x = { name: "foo", foo: function () { print(this.name); } };
(true ? x : undefined).foo();
(true ? x.foo : undefined)();
print(1 + 2 - 2 ? 1 + 2 : 3 + 4);

function printDesc(o, n) {
    print(n + " in o: " + (n in o));
    var desc = Object.getOwnPropertyDescriptor(o, n);
    if (desc) {
	print([ desc.value, desc.set, desc.get, desc.writable, desc.enumerable, desc.configurable ].join(' '));
    } else {
	print(n + " not in o");
    }
}

var o = { foo: 1, bar: 2 };
printDesc(o, "foo");
printDesc(o, "bar");
printDesc(o, "baz");

o = { set foo(x) {}, get bar() {} };
printDesc(o, "foo");
printDesc(o, "bar");
printDesc(o, "baz");

print("--- in");

function foo() {}
function bar() {}
bar.prototype = new foo();
print(new foo() instanceof Object);
print(new foo() instanceof foo);
print(new foo() instanceof bar);
print(foo instanceof Function);
print(new bar() instanceof Object);
print(new bar() instanceof foo);
print(new bar() instanceof bar);
