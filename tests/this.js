x = 3;
print(this.x);
var a = { f: function () { print(this.x); }, x: 2 };
f = function () { print(0); };
a.f();
