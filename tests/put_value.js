function f1() {
    var e = "a";
    function f2() {
	var e = "b";
	function f3() {
	    e = e + "c";
	}
	f3();
	e = e + "d";
	print(e);
    }
    f2();
    print(e);
}

f1();
