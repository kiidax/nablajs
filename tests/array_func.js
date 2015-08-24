function makeArray() {
    var arr = [];
    for (var i = 0; i < 3; i++) {
	arr.push(i);
	arr.push("str-" + i);
	arr.push((i % 2) == 0);
    }
    return arr;
}

// Construtor

print('---');
var a = new Array(3);
print(a.length);
print(a[0]);
print(a[1]);
print(a[2]);
print(a.length);
a = Array(3);
print(a.length);
print(a[0]);
print(a[1]);
print(a[2]);
print(a.length);
a = Array(2,4,5);
print(a);
print(a.length);

// forEach
print('---');
a = [ 5, "hello", true ];
global = this;
callback = function (x,i,o) { print((this === global) + ":" + i + ":" + x + ":" + o); };
r = a.forEach(callback);
print(r);
a = {};
a[2] = 4;
a[3] = 1;
a.length = 4;
Array.prototype.forEach.apply(a, [ callback, "foo" ]);
try {
    Array.prototype.forEach.apply(a, [ ]);
} catch (e) {
    print('OK');
}

print('---');
var arr = makeArray();
print(arr.length);
print(arr);
print(arr.splice(4, 2));
print(arr);
arr = makeArray();
print(arr.splice(4, 2, 'n-1', 'n-2', 'n-3'));
print(arr);
arr = makeArray();
print(arr.splice(4, 2, 'n-1'));
print(arr);
print(arr.length);
print(arr[arr.length - 1]);
print(arr[arr.length]);
print(arr[arr.length + 1]);
print(arr[arr.length + 2]);

// slice
print('---');
arr = makeArray();
print(arr.slice(5));
print(arr.slice(3, 5));
print(arr.slice(3, -4));

// splice
print('--- splice');
arr = makeArray();
print(arr.splice(0,0));
print(arr);
arr = makeArray();
print(arr.splice(8,0));
print(arr);
arr = makeArray();
print(arr.splice(9,0));
print(arr);
arr = makeArray();
print(arr.splice(10,0));
print(arr);
arr = makeArray();
print(arr.splice(5,5));
print(arr);
arr = makeArray();
print(arr.splice(5,2,1,2,3));
print(arr);
arr = makeArray();
print(arr.splice(5,3,1,2));
print(arr);
arr = makeArray();
print(arr.splice(-5,0,1,2,3));
print(arr);
arr = makeArray();
print(arr.splice(5,-1,1,2,3));
print(arr);

// concat
print('---');
arr = makeArray();
var rarr = arr.concat("test");
print(arr);
print(rarr);
print(arr.concat([8,true,"test"]));
print(arr.concat(4, [8,true,"test"]));
print(arr.concat([8,true,"test"], 4));
