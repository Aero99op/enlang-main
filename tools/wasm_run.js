// Runs a .enlng file with the WebAssembly build of the interpreter (website/enlng_engine.*),
// the way the browser does, and prints what it prints. Exit code = the program's.
//   node tools/wasm_run.js file.enlng
// Used by sync_wasm.py to check that the browser engine gives the same output as enlng.exe.
const fs = require('fs');
const path = require('path');
const site = path.join(__dirname, '..', 'website');
const EnlngEngine = require(path.join(site, 'enlng_engine.js'));

(async () => {
  const file = process.argv[2];
  const lib = JSON.parse(fs.readFileSync(path.join(site, 'enlng_stdlib.json'), 'utf8'));
  let out = '';
  let err = '';
  const m = await EnlngEngine({
    print: (s) => { out += s + '\n'; },
    printErr: (s) => { err += s + '\n'; },
    stdin: () => null,
  });
  m.FS.mkdir('/work');
  m.FS.chdir('/work');
  for (const name of Object.keys(lib)) m.FS.writeFile(name, lib[name]);
  // files next to the program, such as a module it uses
  for (const f of fs.readdirSync(path.dirname(file))) {
    if (f.endsWith('.enlng') && !(f in lib)) m.FS.writeFile(f, fs.readFileSync(path.join(path.dirname(file), f)));
  }
  const main = path.basename(file);
  let code;
  try {
    code = m.callMain(['run', main]);
  } catch (e) {
    code = 1;
    err += String(e) + '\n';
  }
  process.stdout.write(out);
  process.stderr.write(err);
  process.exit(code);
})();
