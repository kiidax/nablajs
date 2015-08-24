function a(x) {
    function b() {
	print(x);
	x = x + 1;
	print(x);
    }
    function c() {
	print(x);
	x = x + 2;
	print(x);
    }
    return [b,c];
}

var d = a(1);
d[0]();
d[1]();
