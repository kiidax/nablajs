function f(x) {
    try {
	print("---");
	print(x.foo);
	x.foo = 1;
	print(x.foo);
    } catch (e) {
	print("e:" + e);
    }
}

f({
    set foo (x) { print("set: " + x); }
});

f({
    set foo (x) { print("set: " + x); throw "foo"; }
});

f({
    get foo () { print("get"); return 2; }
});

f({
    set foo (x) { print("set: " + x); throw "foo"; },
    get foo () { print("get"); return 2; }
});

function O() {
}

O.prototype = {
    set foo (x) { print("set: " + x); throw "foo"; },
    get foo () { print("get"); return 2; }
};

f(new O());

function P() {
}

P.prototype = new O();

f(new P());
