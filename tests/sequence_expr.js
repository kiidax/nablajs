print((1,2,3));
var a = 1;
var b = 2;
var c = 3;
print((a,b,c));
print((a=4,b=5,c=6));
print(a);
print(b);
print(c);
a = { get foo () { print("a.foo"); return 1; } };
b = { get foo () { print("b.foo"); return 2; } };
c = { get foo () { print("c.foo"); return 3; } };
print((a.foo,b.foo,c.foo));
