print(new Error());
print(new Error(undefined));
print(new Error("foo"));
print(new Error("foo", "bar"));
var e = new Error("foo");
print(e.name);
print(e.message);
