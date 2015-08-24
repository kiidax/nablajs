function f(a, b) {
    print(a + "," + b);
    if (a) if (b) print("a"); else print("b");
}

f(false, false);
f(false, true);
f(true, false);
f(true, true);
