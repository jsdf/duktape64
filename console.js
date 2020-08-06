#!/usr/bin/env node
const repl = require('repl');
const fs = require('fs');
const readline = require('readline');
const DebuggerInterface = require('ed64logjs/dbgif');

const {default: PQueue} = require('p-queue');
const queue = new PQueue({concurrency: 1});

const dbgif = new DebuggerInterface();
dbgif.start();

dbgif.debugLog = false;

dbgif.print = (...args) => {
  clearLine();
  console.log(...args);
  replServer.displayPrompt();
};

const ED64IO_BUFFER_SIZE = 512;
const ED64IO_CMD_SIZE = 4;
const EVAL_CMD_LENGTH_SIZE = 2;
const EVAL_CMD_FLAGS_SIZE = 2;
const MAX_BODY_SIZE =
  ED64IO_BUFFER_SIZE -
  ED64IO_CMD_SIZE -
  EVAL_CMD_LENGTH_SIZE -
  EVAL_CMD_FLAGS_SIZE;

const FLAG_HAS_MORE_CHUNKS = 1;

function delay(time) {
  return new Promise((resolve) => setTimeout(resolve, time));
}

function clearLine() {
  // readline.clearLine(process.stdout, 0);
  // readline.cursorTo(process.stdout, 0);
}

/*
commands:
e: eval and return result
x: execute, don't return result
*/
async function sendEvalCommand(cmd, jsEvalText) {
  const evalText = Buffer.from(jsEvalText + '\0', 'utf8'); // null terminated
  const totalChunks = Math.ceil(evalText.length / MAX_BODY_SIZE);
  if (dbgif.debugLog) {
    console.log({evalText: evalText.toString('utf8'), totalChunks});
  }
  for (let chunkIdx = 0; chunkIdx < totalChunks; chunkIdx++) {
    const isLastChunk = chunkIdx === totalChunks - 1;
    const bodyBuf = evalText.slice(
      chunkIdx * MAX_BODY_SIZE,
      (chunkIdx + 1) * MAX_BODY_SIZE
    );
    const lengthBuf = Buffer.alloc(EVAL_CMD_LENGTH_SIZE);
    lengthBuf.writeUInt16BE(bodyBuf.length);
    let flags = 0;
    if (!isLastChunk) {
      flags &= FLAG_HAS_MORE_CHUNKS;
    }
    const flagsBuf = Buffer.alloc(EVAL_CMD_FLAGS_SIZE);
    flagsBuf.writeUInt16BE(flags);
    if (dbgif.debugLog) {
      console.log('sending', {
        chunkIdx,
        flags,
        body: bodyBuf.toString('utf8'),
      });
    }

    const data = Buffer.concat([lengthBuf, flagsBuf, bodyBuf]);

    // promise resolves when ready for next message
    await dbgif.sendCommand(cmd, data);
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

let evalResult = null;
let evalCallback = null;
function replEval(cmd, context, filename, callback) {
  // eval wrapped in a closure, returning value to be printed back

  if (cmd.trim() === '') {
    callback();
    return;
  }
  const evalResultPromise = new Promise((resolve) => {
    if (evalCallback) {
      console.error(
        'setting another evalCallback when an evalCallback already set'
      );
    }
    evalCallback = resolve;
  });
  const cmdResult = new Promise((resolve, reject) => {
    runCommand('e', `(function() { return ${cmd} })()`, (err) =>
      err ? reject(err) : resolve()
    );
  }).catch(callback);

  Promise.race([
    Promise.all([evalResultPromise, cmdResult]),
    delay(3000), // 3 sec timeout
  ])
    .then((results) => {
      if (results) {
        const [result] = results;
        clearLine();
        process.stdout.write('=> ' + result + '\n');
        evalCallback = null;
        callback();
      } else {
        evalCallback = null;
        callback(new Error('timed out waiting for result'));
      }
    })
    .catch(callback)
    .finally(() => {
      // evalCallback = null;
    });
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
replServer.defineCommand('sig', {
  help: 'signal',
  async action(name) {
    this.clearBufferedCommand();

    try {
      await new Promise((resolve, reject) => {
        runCommand('s', name, (err) => (err ? reject(err) : resolve()));
      });
    } catch (err) {
      console.error(err);
      throw err;
    }
    this.displayPrompt();
  },
});

dbgif.on('log', (lineRaw) => {
  // erase the prompt
  clearLine();

  const line = lineRaw
    .split('')
    .filter((c) => c != '\0') // wtf
    .join('');

  // handle special output lines denoting start and end of eval output to buffer
  switch (line) {
    case '$EVAL$START': {
      if (evalResult) {
        console.error('unfinished eval result', evalResult);
      }
      evalResult = [];
      break;
    }
    case '$EVAL$END': {
      if (!evalResult) {
        console.error('eval result ended without start');
      } else {
        if (!evalCallback) {
          console.error('eval result ended without callback');
        } else {
          const resultString = evalResult.join('\n');
          evalResult = null;
          evalCallback(resultString);
        }
      }
      evalResult = null;
      break;
    }
    default: {
      if (evalResult) {
        evalResult.push(line);
      } else {
        process.stdout.write(line + '\n');
      }
    }
  }

  replServer.displayPrompt();
});
