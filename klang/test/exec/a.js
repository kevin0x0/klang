function f(n) {
  return n <= 1 && n || f(n - 1) + f(n - 2);
}

console.log(f(36))
