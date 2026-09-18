const fs = require('fs');
const path = require('path');

const repoRoot = path.join(__dirname, '..');
const websiteDir = path.join(repoRoot, 'website');
const uiDir = path.join(__dirname, 'ui');
const binDir = path.join(__dirname, 'bin');

// 1. Sync UI assets
fs.mkdirSync(uiDir, { recursive: true });

const uiFiles = [
  'studio.html',
  'studio.css',
  'studio.js',
  'app.js',
  'enlang_wasm.js',
  'favicon.svg',
  'logo.svg',
  'enlang_bundle.json'
];

for (const file of uiFiles) {
  const src = path.join(websiteDir, file);
  const dest = path.join(uiDir, file);
  if (fs.existsSync(src)) {
    fs.copyFileSync(src, dest);
    console.log(`[prep-ui] Synced UI asset: ${file}`);
  }
}

// Copy examples folder
const examplesSrc = path.join(websiteDir, 'examples');
const examplesDest = path.join(uiDir, 'examples');
if (fs.existsSync(examplesSrc)) {
  fs.cpSync(examplesSrc, examplesDest, { recursive: true });
  console.log('[prep-ui] Synced examples directory');
}

// 2. Sync Native Compilers into desktop/bin
fs.mkdirSync(binDir, { recursive: true });

const compilers = [
  'enlangg.exe',
  'enlng.exe',
  'enlngdb.exe',
  'enlngf.exe',
  'enlngd.exe',
  'enlngs.exe',
  'enlngm.exe'
];

for (const exe of compilers) {
  const src = path.join(repoRoot, exe);
  const dest = path.join(binDir, exe);
  if (fs.existsSync(src)) {
    fs.copyFileSync(src, dest);
    console.log(`[prep-ui] Synced native compiler: ${exe}`);
  }
}

console.log('[prep-ui] Complete: All desktop UI and native compiler assets are synchronized.');
