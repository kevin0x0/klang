def f():
    a = 0
    i = 0
    j = 1
    while i < 1000000:
        a += i * j 
        i += 1
        j = -j
    print(a)
f()
# import dis
# def f(n : int) -> int:
#     return n if n <= 1 else f(n - 1) + f(n - 2)
# 
# print(f(36));
# 
# dis.dis(f);
