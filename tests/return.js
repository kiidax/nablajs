function fib(x) {
    if (x < 3) return 1;
    return fib(x-1) + fib(x-2);
}

function noreturn(x) {
    x + 2;
}

print(fib(6));
print(noreturn(3));
