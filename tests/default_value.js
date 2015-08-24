function check(x, y) {
    print(x + 3);
    print(y + 3);
    print(x + "def");
    print(y + "def");
}

var x, y;

x = { valueOf: function () { return 5; } };
y = { valueOf: function () { return "abc"; } };
check(x, y);
x = { toString: function () { return 4; } };
y = { toString: function () { return "ABC"; } };
check(x, y);
x = {
    valueOf: function () { return 5; },
    toString: function () { return 4; }
};
y = {
    valueOf: function () { return "abc"; },
    toString: function () { return "ABC"; }
};
check(x, y);
