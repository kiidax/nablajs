g = function k(e) { return 12; };
print(g());

function f(x) {
    var w;
    var y = x * 2;
    (function (x) {
	z = 7;
	var w;
	w = 12;
    })();
    return y;
}
x = 4;
v = f(5);
print(f(5));
print(x);
print(v);
try {
    print(y);
} catch (e) {
    print('OK');
}
print(z);
try {
    print(w);
} catch (e) {
    print('OK');
}
/*
print('--- FunctionExpression');

(function functionName() {
    print(typeof functionName);
    functionName = 'NG';
    print(typeof functionName);
}());
print(typeof functionName);
print('OK');
*/
