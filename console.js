#!/usr/bin/env node
const repl = require('repl');
const fs = require('fs');
const DebuggerInterface = require('ed64logjs/dbgif');

const {default: PQueue} = require('p-queue');
const queue = new PQueue({concurrency: 1});

const dbgif = new DebuggerInterface();
dbgif.start();

dbgif.debugLog = false;

const MAX_BODY_SIZE = 506;

function delay(time) {
  return new Promise((resolve) => setTimeout(resolve, time));
}

/*
commands:
e: eval and return result
x: execute, don't return result
*/
async function sendEvalCommand(cmd, evalText) {
  const totalChunks = Math.ceil(
    (evalText.length + 1) /* +1 for null-termination char*/ / MAX_BODY_SIZE
  );
  if (dbgif.debugLog) {
    console.log({evalText, totalChunks});
  }
  for (let chunkIdx = 0; chunkIdx < totalChunks; chunkIdx++) {
    const isLastChunk = chunkIdx === totalChunks - 1;
    const chunk = evalText.slice(
      chunkIdx * MAX_BODY_SIZE,
      (chunkIdx + 1) * MAX_BODY_SIZE
    );
    // last chunk needs to be null-terminated string
    const messageBody = isLastChunk ? chunk + '\0' : chunk;
    // console.log('sending', messageBody, 'chunk', chunkIdx);

    // promise resolves when ready for next message
    await dbgif.sendCommand(cmd, Buffer.from(messageBody, 'utf8'), {
      lengthHeader: true,
    });
  }
}

function parseReplCommand(cmd) {
  let evalText = cmd;
  const matches = cmd.trim().match(/^[.](\w+)\s+([\s\S]*)$/);

  if (matches && matches[1]) {
    return {cmd: matches[1], arg: matches[2]};
  }
  return null;
}

function runCommand(cmd, evalText, callback) {
  queue.add(async () => {
    try {
      await sendEvalCommand(cmd, evalText);
    } catch (err) {
      callback(err);
      return;
    }

    // don't prompt again until last command has succeeded or failed
    callback();
  });
}

function replEval(cmd, context, filename, callback) {
  // eval wrapped in a closure, returning value to be printed back

  if (cmd.trim() === '') {
    callback();
    return;
  }
  const cmdResult = new Promise((resolve, reject) => {
    runCommand(
      'e',
      `(function() {
    return ${cmd}
  })()`,
      (err) => (err ? reject(err) : resolve())
    );
  });
  // TODO: actually wait for result properly lol
  Promise.all([cmdResult.catch(callback), delay(400)]).then(() => callback());
}

const replServer = repl.start({prompt: '> ', eval: replEval});
replServer.setupHistory('./consolehistory.txt', (err, repl) => {
  if (err) {
    console.error(err);
  }
});
replServer.defineCommand('load', {
  help: 'load and eval a file',
  async action(name) {
    this.clearBufferedCommand();

    try {
      const source = fs.readFileSync(name.trim());
      evalText = source;
    } catch (err) {
      console.error('.load failed');
      throw err;
    }
    try {
      await new Promise((resolve, reject) => {
        runCommand('x', evalText, (err) => (err ? reject(err) : resolve()));
      });
    } catch (err) {
      console.error(err);
      throw err;
    }
    this.displayPrompt();
  },
});

dbgif.on('log', (line) => {
  process.stdout.write(line + '\n');
});
