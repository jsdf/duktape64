function fib(num) {
  if (num <= 1) return 1;

  return fib(num - 1) + fib(num - 2);
}
