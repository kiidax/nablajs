function assert(x) { if (!x) { print("error"); quit(1); }}
a = /test/;
assert(a.toString === RegExp.prototype.toString);
assert("asdf".toString !== RegExp.prototype.toString);

print('' + /ab(cd)+e*f/);

var re = /[a-z]+\(([a-z]+)\)/;
print('ok' + re);
re.source = re.global = re.multiline = re.ignoreCase = "NG";
delete re.lastIndex;
print(re.source);
print(re.global);
print(re.lastIndex);
print(re.multiline);

var res = re.exec("    func(xyz)   ");
print(res.length + ":" + res[0] + ":" + res[1]);
try { /a/.exec.apply({}); } catch (e) { print('OK'); }
