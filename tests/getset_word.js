print({ 0: "OK" }[0]);
print({ 0x1: "OK" }[1]);
print({ 1: "OK" }[1]);
print({ "a": "OK" }.a);
print({ if: "OK" }.if);
print({ if: "NG", else: 'OK' }.else);
print({ set: "OK" }.set);
({ set a(x) { print("OK"); }}).a = 1;
({ set set(x) { print("OK"); }}).set = 1;
({ get get() { print("OK"); }}).get;
({ a:1, set set(x) { print("OK"); }}).set = 1;
({ a:1, get get() { print("OK"); }}).get;
({ get if() { print("OK"); }}).if;
//({ "get" if() { print("OK"); }}).if;

function get() {
}

function set() {
}

var set = 1;
var get = 1;

var a, set = 1;
var a, get = 1;

print("END");
