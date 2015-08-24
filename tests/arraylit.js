foo = /*[]*/[1];
bar = [
    "hello"/*,*/
];
baz = [
    "world",
    12,
    true
];
print(foo[0]);
print(bar[0]);
print(baz[0]);
print(baz[1]);
print(bar[1]);
print(baz[2]);
print(baz["0"]);

print("---");

foo = [];
bar = new Array();
print(foo === foo);
print(foo === bar);
print(foo.toString === bar.toString);
print(foo.toString === Array.prototype.toString);
