function Foo() {
    this.foo = 123;
}

Foo.prototype = {
    bar: true
};

var a = new Foo();
print(a.foo);
print(a.bar);

delete a.bar; // print(delete a.bar);
print(a.foo);
print(a.bar);
delete a.foo; // print(delete a.foo);
print(a.foo);
print(a.bar);

(function () {
    var a = 1;
    var b = 2;
    (function (a) {
	print(a);
	print(delete a);
	print(a);
	print(typeof b);
	print(delete b);
	print(typeof b);
	b = 3;
	print(b);
	function b() {}
    }(4));
}());

