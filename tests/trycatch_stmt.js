try {
    print("a");
    print("b");
} catch (e) {
    print(e[1]);
}

print("---");

try {
    print("a");
    throw [1,2,3];
    print("b");
} catch (e) {
    print(e[1]);
}

print("---");

try {
    print("a");
    print("b");
} catch (e) {
    print(e[1]);
} finally {
    print("c");
}

print("---");

try {
    print("a");
    throw [1,2,3];
    print("b");
} catch (e) {
    print(e[1]);
} finally {
    print("c");
}
