function test1() {
    print("---");
    function f(a) {
	print("-");
	switch (a + 3) {
	case 1:
	    print("a");
	    print("b");
	case 2:
	    print("c");
	    print("d");
	    break;
	default:
	case 3:
	    print("e");
	    print("f");
	case 4:
	    print("e");
	    print("f");
	    break;
	case 5:
	case 6:
	    print("g");
	case 7:
	    print("h");
	}
    }

    for (var i = -2; i < 6; i++) f(i);
}

function test2() {
    print("---");
    switch ("a") {
    case 1: print("NG"); break;
    case 2: print("NG"); break;
    case "a":
	print("1");
	switch (true) {
	case true:
	    print("1");
	    break;
	case false:
	    break;
	}
	print("2");
	break;
    case 3: print("NG"); break;
    }
}

function test3() {
    print("---");
    foo:
    for (;;) {
	print('1');
	switch (true) {
	case true:
	    print("2");
	    break foo;
	    print('NG');
	case false:
	    print('NG');
	    break;
	}
	print('NG');
    }
    print('3');
}

function test4() {
    print("---");
    for (;;) {
	print('1');
	switch (true) {
	case true:
	    print("2");
	    break;
	    print('NG');
	case false:
	    print('NG');
	    break;
	}
	print('3');
	break;
    }
    print('4');
}

function test5() {
    print("---");
    switch ("a") {
    case 1: print("NG"); break;
    case 2: print("NG"); break;
    case "a":
	print("1");
	switch (true) {
	case true:
	    print("2");
	    break;
	case false: print('NG'); break;
	}
	print("3");
	break;
    case 3: print("NG"); break;
    }
    print('4');
}

function test6() {
    print("---");
    foo:
    switch ("a") {
    case 1: print("NG"); break;
    case 2: print("NG"); break;
    case "a":
	print("1");
	bar:
	switch (true) {
	case true:
	    print("2");
	    break;
	case false: print('NG'); break;
	}
	print("3");
	break;
    case 3: print("NG"); break;
    }
    print('4');
}

function test7() {
    print("---");
    foo:
    switch ("a") {
    case 1: print("NG"); break;
    case 2: print("NG"); break;
    case "a":
	print("1");
	bar:
	switch (true) {
	case true:
	    print("2");
	    break bar;
	case false: print('NG'); break;
	}
	print("3");
	break;
    case 3: print("NG"); break;
    }
    print('4');
}

function test8() {
    print("---");
    foo:
    switch ("a") {
    case 1: print("NG"); break;
    case 2: print("NG"); break;
    case "a":
	print("1");
	bar:
	switch (true) {
	case true:
	    print("2");
	    break foo;
	case false: print('NG'); break;
	}
	print("NG");
	break;
    case 3: print("NG"); break;
    }
    print('3');
}

function test9() {
    print("---");
    foo:
    switch ("a") {
    case 1: print("NG"); break;
    case 2: print("NG"); break;
    case "a":
	print("1");
	switch (true) {
	case true:
	    print("2");
	    break foo;
	case false: print('NG'); break;
	}
	print("NG");
	break;
    case 3: print("NG"); break;
    }
    print('3');
}

function test10() {
    print("---");
    for (var i = 0; i < 2; i++) {
	print('1');
	switch (true) {
	case true:
	    print("2");
	    continue;
	    print('NG');
	case false:
	    print('NG');
	    break;
	}
	print('NG');
	break;
    }
    print('3');
}

try {
    test1();
    test2();
    test3();
    test4();
    test5();
    test6();
    test7();
    test8();
    test9();
    test10();
} catch (e) {
    print('NG: ' + e);
}
    
