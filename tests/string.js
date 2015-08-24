var a = String(123.3);
print(typeof a);
var a = String();
print(typeof a);
var a = String({});
print(typeof a);
a = new String(123.3);
print(typeof a);
print(a.valueOf());
print(typeof a.valueOf());

print("abc".length);
print("abc".charCodeAt(0));
print("abc".charCodeAt(1));
print("abc".charCodeAt(2));

print('--- indexOf');

print("abcdef".indexOf("cde"));
print("abcdef".indexOf("cdf"));
print("abcdef".indexOf("0"));

print('---');

print("a, b, c, d, e, f".split(", "));
print(", b, c, d, e, f".split(", "));
print("a, b, c, d, e, ".split(", "));
print("12345678".split(/(?=1)/));
print("12345678".split(/(?=5)/));
print("12345678".split(/$/));
// print("12345678".split(/\b/));
print("".split(/^/).length);
print("".split(/a/).length);
print("".split(/a/)[0].length);
print("".split("").length);
print("".split("a").length);
print("".split("a")[0].length);
print("127.0.0.1".split("."));

print('--- substr');
var s = "0123456789";
print(s.substr(3,5));
print(s.substr(3,10));
print(s.substr(3,undefined));
print(s.substr(3));
print(s.substr());

print('--- substring');

var s = "0123456789";
print(s.substring(4, 8));
print(s.substring(4));
print(s.substring());
print(s.substring(0, 10));
print(s.substring(-1, 5));
print(s.substring(5, 11));
print(s.substring(7, 3));
print(s.substring(11, 5));
print(s.substring(5, -1));
print(s.substring(5, undefined));

print('--- replace');

print('abcABCdefABCghi'.replace('ABC', '(ABC)'));
print('abcABCdefABC'.replace('ABC', '(ABC)'));
print(''.replace('ABC', '(ABC)'));
print('abcABCdefABCghi'.replace('', '(ABC)'));
print('abcABCdefABCghi'.replace('ABC', ''));
