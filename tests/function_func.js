function f(a, b) {
    return this.toString() + "," + a + "," + b;
}

var o = { toString: function () { return "foo"; } };
print(f.call(o, "bar", "baz"));
print(f.call(o, "bar"));
print(f.call(o));

print(f.apply(o, [ "bar", "baz" ]));
print(f.apply(o, [ "bar" ]));
print(f.apply(o, [ ]));
print(f.apply(o, undefined));
print(f.apply(o, null));
print(f.apply(o));
print(f.apply());
