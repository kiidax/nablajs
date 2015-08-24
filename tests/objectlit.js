foo = {};
foo = ({});
while (false) {}
bar = {
    prop1: "hello",
};
baz = {
    prop1: "world",
    prop2: 12,
    "prop3": true
};
print(foo.prop1);
print(bar.prop1);
print(baz.prop1);
print(baz.prop2);
print(bar.prop2);
print(baz.prop3);

print("---");

foo = {};
bar = new Object();
print(foo === foo);
print(foo === bar);
print(foo.toString === bar.toString);
print(foo.toString === Object.prototype.toString);
