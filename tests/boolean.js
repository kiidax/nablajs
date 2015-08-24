var a = Boolean(true);
print(typeof a);
var a = Boolean(true);
print(typeof a);
var a = Boolean();
print(typeof a);
var a = Boolean({});
print(typeof a);

a = new Boolean(true);
print(typeof a);
print(a.valueOf());
print(typeof a.valueOf());

a = new Boolean(false);
print(typeof a);
print(a.valueOf());
print(typeof a.valueOf());
