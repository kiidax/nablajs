function test1() {
    foo:
    if (true) {
	continue foo;
    }
}

function test2() {
    print("---");
    for (var i = 0; i < 5; i++) {
	if (i == 2) {
	    print("1");
	    continue;
	    print("2");
	}
	print("3");
    }
    print("4");
}

function test3() {
    print("---");
    for (var i = 0; i < 2; i++) {
	for (var j = 0; j < 5; j++) {
	    if (j == 2) {
		print("1");
		continue;
		print("2");
	    }
	    print("3");
	}
    }
    print("4");
}

function test4() {
    print("---");
    for (var i = 0; i < 2; i++) {
	foo:
	for (var j = 0; j < 5; j++) {
	    if (j == 2) {
		print("1");
		continue foo;
		print("2");
	    }
	    print("3");
	}
    }
    print("4");
}

function test5() {
    print("---");
    foo:
    for (var i = 0; i < 2; i++) {
	for (var j = 0; j < 5; j++) {
	    if (j == 2) {
		print("1");
		continue foo;
		print("2");
	    }
	    print("3");
	}
    }
    print("4");
}


function test6() {
    print("---");
    foo:
    for (var i = 0; i < 2; i++) {
	for (var j = 0; j < 2; j++) {
	    bar:
	    for (var k = 0; k < 5; k++) {
		if (k == 2) {
		    print("1");
		    continue foo;
		    print("2");
		}
		print("3");
	    }
	    print("4");
	}
	print("5");
    }
    print("6");
}

function test7() {
    print("---");
    foo:
    for (var i = 0; i < 2; i++) {
	for (var j = 0; j < 2; j++) {
	    bar:
	    for (var k = 0; k < 5; k++) {
		if (k == 2) {
		    print("1");
		    continue bar;
		    print("2");
		}
		print("3");
	    }
	    print("4");
	}
	print("5");
    }
    print("6");
}

try {
    try {
	test1();
    } catch (e) {
	print('OK');
    }
    test2();
    test3();
    test4();
    test5();
    test6();
    test7();
} catch (e) {
    print('NG: ' + e);
}

