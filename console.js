const readline = require('readline');
const DebuggerInterface = require('ed64logjs/dbgif');

const dbgif = new DebuggerInterface();
dbgif.start();

dbgif.on('log', (line) => {
  process.stdout.write(line + '\n');
});

function parseAndSendCommand(cmdText) {
  try {
    dbgif.sendCommand('e', cmdText, {lengthHeader: true});
  } catch (err) {
    console.error(err);
  }
}

const rl = readline.createInterface({
  input: process.stdin,
  output: process.stdout,
  prompt: '> ',
});

rl.prompt();

rl.on('line', (line) => {
  parseAndSendCommand(line.trim());
  rl.prompt();
}).on('close', () => {
  console.log('exiting');
  process.exit(0);
});
