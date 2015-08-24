/abc/;
a = /def/;
print(a);
/ghi/ + 3;
a = /jkl/ + 3;
print(a);
printre(/abc/);
printre(/abc/g);
printre(/abc/i);
printre(/abc/m);
printre(/abc/gi);
printre(/abc/im);
printre(/abc/mg);
try {
    /abc/q;
} catch (e) {
    print('OK');
}
try {
    /abc/gmg;
} catch (e) {
    print('OK');
}

printre(new RegExp('abc'));
printre(new RegExp('abc', 'g'));
printre(new RegExp('abc', 'i'));
printre(new RegExp('abc', 'm'));
printre(new RegExp('abc', 'gi'));
printre(new RegExp('abc', 'im'));
printre(new RegExp('abc', 'mg'));
try {
    new RegExp('abc', 'q');
} catch (e) {
    print('OK');
}
try {
    new RegExp('abc', 'gmg');
} catch (e) {
    print('OK');
}

function printre(re) {
    print([re.global, re.ignoreCase, re.multiline, re.lastIndex].join(','));
}
