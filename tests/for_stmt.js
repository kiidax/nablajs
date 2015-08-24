test1();
test2();
test3();
test4();

function test1() {
    function f() {
	print(o.i);
    }
    
    f.foo = "a";
    f.bar = 2;
    f.baz = true;
    f.quz = undefined;
    
    var o = {};
    
    for (o.i in f) {
	f();
    }
    print('OK');
}

function test2() {
    var o = { foo: 1, bar: 2, baz: 3 };
    var i = 'OK';
    function a() {
	for (var i in o) {
	    print(i);
	}
    }

    a();
    print(i);
}

function test3() {
    var i = 'OK';
    function a() {
	for (var i = 1, j = -1; i <= 4; i++,j--) {
	    print(i + "," + j);
	}
    }

    a();
    print(i);
}

function test4() {
    var i = '*';
    function a() {
	for (i = 1, j = -1; i <= 4; i++,j--) {
	    print(i + "," + j);
	}
    }

    a();
    print(i == 5 ? 'OK' : 'NG');
}
