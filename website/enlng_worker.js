// Runs Enlng programs in the browser. This is the same interpreter as enlng.exe,
// built to WebAssembly (tools/build_wasm.ps1). It lives in a Web Worker so that a
// program that never ends can be stopped by closing the worker.
importScripts('enlng_engine.js', 'enlngdb_engine.js');

var stdlib = null;
var saved = {}; // files a program wrote, kept for the next run (a database, a text file)

function loadStdlib() {
  if (stdlib) return Promise.resolve(stdlib);
  return fetch('enlng_stdlib.json').then(function (r) { return r.json(); }).then(function (j) {
    stdlib = j;
    return j;
  });
}

function runProgram(msg) {
  return loadStdlib().then(function (lib) {
    var isDb = msg.domain === 'enlngdb';
    var factory = isDb ? EnlngdbEngine : EnlngEngine;
    var out = '';
    var err = '';
    var input = String(msg.stdin || '');
    var pos = 0;
    return factory({
      print: function (s) { out += s + '\n'; },
      printErr: function (s) { err += s + '\n'; },
      // `ask` reads from the box under the editor; when it is used up, the input ends
      stdin: function () { return pos < input.length ? input.charCodeAt(pos++) & 255 : null; }
    }).then(function (m) {
      var FS = m.FS;
      FS.mkdir('/work');
      FS.chdir('/work');
      Object.keys(lib).forEach(function (name) { FS.writeFile(name, lib[name]); });
      Object.keys(saved).forEach(function (name) { FS.writeFile(name, saved[name]); });
      var main = isDb ? 'query.enlngdb' : 'main.enlng';
      FS.writeFile(main, msg.code);
      var code;
      var t0 = performance.now();
      try {
        code = m.callMain(isDb ? [main] : ['run', main]);
      } catch (e) {
        if (e instanceof RangeError) {
          err += 'The program went too deep: a function called itself too many times. Check that it has a way to stop.\n';
        } else {
          err += String(e && e.message ? e.message : e) + '\n';
        }
        code = 1;
      }
      var ms = performance.now() - t0;
      // keep what the program wrote, except the files that are always put there
      var next = {};
      FS.readdir('/work').forEach(function (name) {
        if (name === '.' || name === '..' || name === main || lib[name] !== undefined) return;
        var st = FS.stat('/work/' + name);
        if (FS.isFile(st.mode)) next[name] = FS.readFile('/work/' + name);
      });
      saved = next;
      return { code: code, out: out, err: err, ms: ms };
    });
  });
}

self.onmessage = function (ev) {
  var msg = ev.data;
  if (msg.type === 'reset') {
    saved = {};
    self.postMessage({ id: msg.id, reset: true });
    return;
  }
  runProgram(msg).then(function (r) {
    r.id = msg.id;
    self.postMessage(r);
  }, function (e) {
    self.postMessage({ id: msg.id, code: 1, out: '', err: String(e && e.message ? e.message : e), ms: 0 });
  });
};
