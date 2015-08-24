var str = "test";
print(JSON.stringify(str) === JSON.stringify(str));
str = "[ \"test\" ]";
print(typeof JSON.parse(str) === 'object');
