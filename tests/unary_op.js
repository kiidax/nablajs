var a = [ undefined, true, false, null, 12, 3.4, "a", "", {} ];

try {
    print(void print("OK"));
    a.forEach(function (x) { print(typeof x); });
    a.forEach(function (x) { print(void x); });
    a.forEach(function (x) { print(+x); });
    a.forEach(function (x) { print(-x); });
    a.forEach(function (x) { print(!x); });
    a.forEach(function (x) { print(~x); });
} catch (e) {
    print(e);
}

