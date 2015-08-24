print(Object.prototype.constructor === Object);
print(Object.prototype.constructor === Array);

a = { foo: 1, bar: 2 };
Object.keys(a).forEach(function (x) { print(x); });

print(a.hasOwnProperty("foo"));
print(a.hasOwnProperty("bar"));
print(a.hasOwnProperty("baz"));

print('--- Object.keys');

a = { foo: 1, get bar () { return 1; } };
print(Object.keys(a));
a = { foo: 1, get bar () { return 1; }, set bar (x) {} };
print(Object.keys(a));
print(a.bar);
if (0) {
    Object.defineProperty(a, {
	name: "bar",
	value: 123,
	enumerable: false
    });
    print(a.bar);
    print(Object.keys(a));
}

print('--- valueOf');
function printval(x) {
    print(typeof x + ":" + x);
}
printval((123).valueOf());
printval((3.14).valueOf());
printval(("abc").valueOf());
printval((true).valueOf());
printval((false).valueOf());
printval(([]).valueOf());
printval(({}).valueOf());
print(typeof (print.valueOf()));
printval((/abc/).valueOf());

printval(Object.prototype.valueOf.apply(123));
printval(Object.prototype.valueOf.apply(3.14));
printval(Object.prototype.valueOf.apply("abc"));
printval(Object.prototype.valueOf.apply(true));
printval(Object.prototype.valueOf.apply(false));
printval(Object.prototype.valueOf.apply([]));
printval(Object.prototype.valueOf.apply({}));
print(typeof (Object.prototype.valueOf.apply(print)));
printval(Object.prototype.valueOf.apply(/abc/));
