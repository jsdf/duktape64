const fanout = 2;

function doStuff(maxDepth, depth) {
  const a = {};
  for (var i = 0; i < fanout; i++) {
    a[i] = depth == maxDepth ? {} : doStuff(maxDepth, depth + 1);
  }
  return a;
}
