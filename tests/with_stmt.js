var z = 3;
var a = { x: 1, y: 2 };
a.foo = function (z) {
    return this.x + z;
};
try {
    with (a) {
	print(x);
	print(y);
	print(z);
	print(foo(4));
    }
} catch (e) {
    print(e);
}
