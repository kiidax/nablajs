var o = new Object();
print("Object.prototype = " + Object);
print("Object.prototype = " + Object.prototype);
print("Object.prototype.toString = " + Object.prototype.toString);

print("---");

print("o = " + o);
print("o.prototype = " + o.prototype);
print("o.toString = " + o.toString);
print("o.toString() = " + o.toString());

print("---");

var MyClass = function () {
};

MyClass.prototype.toString = function () {
    return "[MyClass]";
};

o = new MyClass();
print("o = " + o);
print("o.prototype = " + o.prototype);
print("o.toString = " + o.toString);
print("o.toString() = " + o.toString());
