// Enlangg in the browser: the real Enlng interpreter and the EnlngDB engine, built
// to WebAssembly from the same C sources as enlng.exe and enlngdb.exe
// (tools/build_wasm.ps1). Nothing is sent to a server, and no other runtime is loaded.
//
//   window.EnlangWasm.execute(filename, code, domain, stdin) -> Promise of
//     { success, exitCode, output, error, timeMs, executor }
(function (window) {
  'use strict';

  var RUN_LIMIT_MS = 15000;
  var worker = null;
  var ready = false;
  var nextId = 1;
  var pending = {};
  var DOMAINS = { enlng: 'enlng', enlngdb: 'enlngdb' };
  var EXECUTOR = 'Enlng (WebAssembly)';

  function setStatus(ok) {
    var dot = document.getElementById('statusNativeEngineDot');
    var text = document.getElementById('statusNativeEngineText');
    var pill = document.getElementById('statusNativeEnginePlayground');
    if (dot) dot.style.background = ok ? '#4ec9b0' : '#e5c07b';
    if (text) text.textContent = ok ? 'Engine: Enlng WebAssembly' : 'Engine: not loaded';
    if (pill) pill.title = ok ? 'Running the Enlng interpreter as WebAssembly in this page.' : 'The WebAssembly engine could not be loaded.';
  }

  function start() {
    if (worker) return worker;
    try {
      worker = new Worker('enlng_worker.js');
    } catch (e) {
      worker = null;
      return null;
    }
    worker.onmessage = function (ev) {
      var r = ev.data;
      var p = pending[r.id];
      if (!p) return;
      delete pending[r.id];
      clearTimeout(p.timer);
      ready = true;
      setStatus(true);
      p.resolve(r);
    };
    worker.onerror = function () {
      Object.keys(pending).forEach(function (id) {
        var p = pending[id];
        delete pending[id];
        clearTimeout(p.timer);
        p.resolve({ code: 1, out: '', err: 'The WebAssembly engine could not be loaded.', ms: 0 });
      });
      worker = null;
      ready = false;
      setStatus(false);
    };
    return worker;
  }

  function send(message, limit) {
    return new Promise(function (resolve) {
      var w = start();
      if (!w) {
        resolve({ code: 1, out: '', err: 'This browser cannot run the WebAssembly engine.', ms: 0 });
        return;
      }
      var id = nextId++;
      message.id = id;
      var entry = { resolve: resolve };
      entry.timer = setTimeout(function () {
        // the program did not end: close its worker; the next run starts a new one
        delete pending[id];
        if (worker) worker.terminate();
        worker = null;
        ready = false;
        resolve({ code: 124, out: '', err: 'The program was stopped after ' + (limit / 1000) + ' seconds.', ms: limit, stopped: true });
      }, limit);
      pending[id] = entry;
      w.postMessage(message);
    });
  }

  function init() {
    if (!start()) return Promise.resolve(false);
    // the engine is loaded by the first run; a small run now warms it up
    return send({ type: 'run', domain: 'enlng', code: 'type enlng\n', stdin: '' }, RUN_LIMIT_MS).then(function (r) {
      return r.code === 0;
    });
  }

  function execute(filename, code, domain, stdin) {
    var d = DOMAINS[String(domain || 'enlng').toLowerCase()];
    if (!d) {
      return Promise.resolve({
        success: false,
        exitCode: 1,
        output: 'Files of this kind run in Enlangg Studio on your computer, or with the installed enlangg tools. The browser runs .enlng and .enlngdb files.',
        error: 'Not available in the browser',
        timeMs: '0.00',
        executor: EXECUTOR
      });
    }
    return send({ type: 'run', domain: d, filename: filename, code: code, stdin: stdin || '' }, RUN_LIMIT_MS).then(function (r) {
      var ok = r.code === 0;
      return {
        success: ok,
        exitCode: r.code,
        output: ok ? (r.out || '// [Execution completed with 0 output]') : (r.err || r.out || 'Unknown error'),
        error: ok ? '' : (r.err || r.out),
        stdout: r.out,
        stderr: r.err,
        timeMs: (r.ms || 0).toFixed(2),
        executor: EXECUTOR
      };
    });
  }

  function resetDatabase() {
    return send({ type: 'reset' }, 5000).then(function () { return true; });
  }

  window.EnlangWasm = {
    init: init,
    execute: execute,
    resetDatabase: resetDatabase,
    isReady: function () { return ready; }
  };

  if (typeof window !== 'undefined' && typeof document !== 'undefined') {
    if (document.readyState === 'complete' || document.readyState === 'interactive') {
      setTimeout(init, 100);
    } else {
      window.addEventListener('DOMContentLoaded', function () { init(); });
    }
  }
})(window);
