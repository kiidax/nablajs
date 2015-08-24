a = this;
print((function () { return this === a; })());
a = 3;
print(this.a);

Object.prototype = 3;
print(typeof Object.prototype);

var oldToString = Object.prototype.toString;
Object.prototype.toString = function () { return "hoge"; };
print({});
Object.prototype.toString = oldToString;
Array.prototype = {};
print(typeof Array.prototype.splice);
