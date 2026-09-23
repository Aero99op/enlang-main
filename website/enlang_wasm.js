// Enlang Sovereign In-Browser WebAssembly Engine
// Connects Playground and Studio directly to the REAL Enlang compiler running inside Pyodide WebAssembly.
// Zero backend server, zero bridge daemon required.
(function(window) {
  let pyodideInstance = null;
  let isInitializing = false;
  let isReady = false;
  const initCallbacks = [];

  async function initEnlangWasm() {
    if (isReady) return true;
    if (isInitializing) {
      return new Promise(resolve => initCallbacks.push(resolve));
    }
    isInitializing = true;

    try {
      // 1. Ensure Pyodide script is loaded
      if (typeof loadPyodide !== 'function') {
        await new Promise((resolve, reject) => {
          const script = document.createElement('script');
          script.src = 'https://cdn.jsdelivr.net/pyodide/v0.26.2/full/pyodide.js';
          script.onload = resolve;
          script.onerror = reject;
          document.head.appendChild(script);
        });
      }

      console.log('[Enlang WASM] Initializing Pyodide WebAssembly runtime...');
      pyodideInstance = await loadPyodide({
        indexURL: 'https://cdn.jsdelivr.net/pyodide/v0.26.2/full/'
      });

      console.log('[Enlang WASM] Fetching Enlang 6-domain compiler bundle...');
      const resp = await fetch('enlang_bundle.json');
      const bundle = await resp.json();

      console.log(`[Enlang WASM] Unpacking ${Object.keys(bundle).length} compiler files into virtual WebAssembly filesystem...`);
      // Unpack bundle into Pyodide virtual filesystem
      for (const [relPath, content] of Object.entries(bundle)) {
        const fullPath = `/home/pyodide/${relPath}`;
        const parts = fullPath.split('/');
        let cur = '';
        for (let i = 1; i < parts.length - 1; i++) {
          cur += '/' + parts[i];
          try {
            pyodideInstance.FS.mkdir(cur);
          } catch (e) {
            // directory already exists
          }
        }
        pyodideInstance.FS.writeFile(fullPath, content, { encoding: 'utf8' });
      }

      // Initialize Python path and import engine
      await pyodideInstance.runPythonAsync(`
import sys
if '/home/pyodide' not in sys.path:
    sys.path.insert(0, '/home/pyodide')
import enlang_engine
`);

      console.log('[Enlang WASM] Real Enlang Compiler successfully loaded in WebAssembly!');
      isReady = true;
      isInitializing = false;
      
      // Update UI pills if present
      const dot = document.getElementById('bridgeStatusDot');
      const text = document.getElementById('bridgeStatusText');
      const pill = document.getElementById('statusNativeEnginePlayground');
      if (dot && (!dot.style.background || dot.style.background.includes('229') || dot.style.background === 'rgb(229, 192, 123)')) {
        dot.style.background = '#4ec9b0';
        dot.style.boxShadow = '0 0 6px #4ec9b0';
        if (text) text.textContent = 'Engine: Real WASM Compiler';
        if (pill) pill.title = 'Running REAL Enlang 6-domain compiler natively inside WebAssembly!';
      }

      initCallbacks.forEach(cb => cb(true));
      initCallbacks.length = 0;
      return true;
    } catch (err) {
      console.warn('[Enlang WASM] WebAssembly engine init warning:', err);
      isInitializing = false;
      initCallbacks.forEach(cb => cb(false));
      initCallbacks.length = 0;
      return false;
    }
  }

  function escapeHtml(s) {
    return String(s).replace(/&/g, '&amp;').replace(/</g, '&lt;').replace(/>/g, '&gt;').replace(/"/g, '&quot;');
  }

  async function executeEnlangWasm(filename, code, domain) {
    const ready = await initEnlangWasm();
    if (!ready) {
      return null;
    }

    try {
      // 1. Auto-load WebAssembly packages if detected in source code
      const lower = code.toLowerCase();
      const neededPkgs = [];
      if ((lower.includes('numpy') || lower.includes('np.')) && (!pyodideInstance.loadedPackages || !pyodideInstance.loadedPackages['numpy'])) {
        neededPkgs.push('numpy');
      }
      if ((lower.includes('matplotlib') || lower.includes('plt.')) && (!pyodideInstance.loadedPackages || !pyodideInstance.loadedPackages['matplotlib'])) {
        neededPkgs.push('matplotlib');
      }
      if (lower.includes('scipy') && (!pyodideInstance.loadedPackages || !pyodideInstance.loadedPackages['scipy'])) {
        neededPkgs.push('scipy');
      }
      if (neededPkgs.length > 0) {
        console.log('[Enlang WASM] Loading WebAssembly package dependencies:', neededPkgs);
        await pyodideInstance.loadPackage(neededPkgs);
      }

      // If skfuzzy or other pip package is needed in Pyodide WebAssembly
      if (lower.includes('skfuzzy') || lower.includes('scikit-fuzzy')) {
        console.log('[Enlang WASM] Installing scikit-fuzzy via micropip into WebAssembly...');
        await pyodideInstance.loadPackage('micropip');
        await pyodideInstance.runPythonAsync(`
import micropip
try:
    import skfuzzy
except ImportError:
    await micropip.install('scikit-fuzzy')
`);
      }

      pyodideInstance.globals.set('__exec_file', filename);
      pyodideInstance.globals.set('__exec_code', code);
      pyodideInstance.globals.set('__exec_domain', domain || 'enlng');

      const rawJson = await pyodideInstance.runPythonAsync(`
import enlang_engine
enlang_engine.execute_enlang_wasm(__exec_file, __exec_code, __exec_domain)
`);
      const result = JSON.parse(rawJson);

      // 2. Check Pyodide virtual filesystem for any newly generated plot images (.png, .jpg)
      try {
        const rootFiles = pyodideInstance.FS.readdir('/home/pyodide');
        let imageHtml = '';
        for (const fname of rootFiles) {
          if (fname.endsWith('.png') || fname.endsWith('.jpg') || fname.endsWith('.jpeg')) {
            const data = pyodideInstance.FS.readFile('/home/pyodide/' + fname);
            if (data && data.length > 0) {
              const b64 = btoa(Array.from(data, b => String.fromCharCode(b)).join(''));
              const mime = fname.endsWith('.png') ? 'image/png' : 'image/jpeg';
              imageHtml += `<div style="margin-top:14px; text-align:center;"><img src="data:${mime};base64,${b64}" style="max-width:100%; border-radius:8px; border:1px solid rgba(255,255,255,0.15); box-shadow:0 8px 24px rgba(0,0,0,0.5);" alt="${escapeHtml(fname)}"/><div style="font-size:11px; color:#888; margin-top:4px;">📊 Generated Graph: ${escapeHtml(fname)} (${Math.round(data.length / 1024)} KB)</div></div>`;
              try { pyodideInstance.FS.unlink('/home/pyodide/' + fname); } catch (_) {}
            }
          }
        }
        if (imageHtml) {
          result.imageHtml = imageHtml;
        }
      } catch (fsErr) {
        // Virtual FS read optional
      }

      return result;
    } catch (err) {
      return {
        success: false,
        exitCode: 1,
        output: String(err),
        error: String(err),
        executor: 'Enlang Real Compiler (WebAssembly Error)'
      };
    }
  }

  window.EnlangWasm = {
    init: initEnlangWasm,
    execute: executeEnlangWasm,
    isReady: () => isReady
  };

  // Auto-initiate in background on page load
  if (typeof window !== 'undefined' && typeof document !== 'undefined') {
    if (document.readyState === 'complete' || document.readyState === 'interactive') {
      setTimeout(() => initEnlangWasm(), 100);
    } else {
      window.addEventListener('DOMContentLoaded', () => initEnlangWasm());
    }
  }
})(window);
