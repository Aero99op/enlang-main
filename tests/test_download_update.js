/**
 * Updater integration test against the REAL desktop/main.js.
 *
 * No updater logic is duplicated here. The test:
 *   - mocks the `electron` module (ipcMain, app, BrowserWindow, dialog, shell, Menu, screen),
 *   - replaces `child_process.spawn` *only for main.js* with a recorder, so the installer is never executed,
 *   - wraps `https` *only for main.js* to count requests and optionally flip one byte in transit,
 *   - loads desktop/main.js, captures the registered IPC handlers and calls them directly.
 *
 * Scenarios:
 *   1. Non-allowed host (https://evil.example/x.exe) is rejected before any network request.
 *   2. Valid release URL + correct expectedSha256 downloads and verifies.
 *   3. installUpdate on the verified file calls spawn (recorded, NOT executed) with the verified path.
 *   4. One byte changed on disk after verification -> installUpdate refuses, deletes file, spawn not called.
 *   5. One byte flipped in transit -> downloadUpdate rejects and deletes the file.
 *   6. Wrong expectedSha256 -> downloadUpdate rejects and deletes the file.
 */
'use strict';

const Module = require('module');
const path = require('path');
const fs = require('fs');
const os = require('os');
const crypto = require('crypto');
const realHttps = require('https');
const realChildProcess = require('child_process');

const REPO_ROOT = path.resolve(__dirname, '..');
const MAIN_JS = path.resolve(REPO_ROOT, 'desktop', 'main.js');
const RELEASE_URL = 'https://github.com/Aero99op/enlang-main/releases/download/v5.0.1/Enlangg-Studio-Setup-5.0.1.exe';
const ASSET_NAME = 'Enlangg-Studio-Setup-5.0.1.exe';
const GOOD_SHA = '6dab02128401bdc624fa48acffaa6a377f3d68998dd1a321e101ff681748272a';
const BAD_SHA = '6dab02128401bdc624fa48acffaa6a377f3d68998dd1a321e101ff681748272b';
const EXPECTED_SIZE = 82703683;

// ---------------------------------------------------------------------------
// Isolate the updater temp dir: main.js uses os.tmpdir()/enlangg_studio_update.
// ---------------------------------------------------------------------------
const TEST_TMP = fs.mkdtempSync(path.join(os.tmpdir(), 'enlangg_updater_it_'));
process.env.TEMP = TEST_TMP;
process.env.TMP = TEST_TMP;
process.env.TMPDIR = TEST_TMP;
const UPDATE_DIR = path.join(os.tmpdir(), 'enlangg_studio_update');
const DEST = path.join(UPDATE_DIR, ASSET_NAME);

// ---------------------------------------------------------------------------
// Sanitizer: strip usernames, machine profiles, and absolute temp paths
// so printed output is completely generic and safe for public sharing.
// ---------------------------------------------------------------------------
function sanitize(val) {
  if (typeof val !== 'string') return val;
  let s = val;
  if (UPDATE_DIR) s = s.split(UPDATE_DIR).join('<temp>/enlangg_studio_update');
  if (TEST_TMP) s = s.split(TEST_TMP).join('<temp>');
  if (REPO_ROOT) s = s.split(REPO_ROOT).join('<repo>');
  const home = os.homedir();
  if (home) s = s.split(home).join('<home>');
  const user = process.env.USERNAME || process.env.USER;
  if (user) s = s.split(user).join('<user>');
  return s.replace(/\\/g, '/');
}

// ---------------------------------------------------------------------------
// Electron mock
// ---------------------------------------------------------------------------
const handlers = new Map();
const appQuitCalls = [];
const electronMock = {
  ipcMain: {
    handle(channel, fn) { handlers.set(channel, fn); },
    on() {}
  },
  app: {
    getVersion: () => '5.0.0',
    isPackaged: false,
    requestSingleInstanceLock: () => true,
    whenReady: () => new Promise(() => {}), // never resolves: no window is created
    on() {},
    quit: () => { appQuitCalls.push(Date.now()); },
    getPath: () => TEST_TMP
  },
  BrowserWindow: function BrowserWindow() { throw new Error('BrowserWindow must not be constructed in test'); },
  dialog: { showOpenDialog: async () => ({ canceled: true }), showSaveDialog: async () => ({ canceled: true }) },
  shell: { openExternal: async () => {}, showItemInFolder: () => {}, openPath: async () => '' },
  Menu: { setApplicationMenu() {} },
  screen: { getPrimaryDisplay: () => ({ workAreaSize: { width: 1920, height: 1080 } }) }
};

// ---------------------------------------------------------------------------
// spawn recorder (main.js only) — guarantees the installer is never executed.
// ---------------------------------------------------------------------------
const spawnCalls = [];
const childProcessForMain = Object.assign({}, realChildProcess, {
  spawn(cmd, args, opts) {
    spawnCalls.push({ cmd, args, opts });
    return { unref() {}, on() {}, stdout: { on() {} }, stderr: { on() {} } };
  }
});

// ---------------------------------------------------------------------------
// https wrapper (main.js only) — counts requests, optional 1-byte in-transit flip.
// ---------------------------------------------------------------------------
const httpsLog = [];
let tamperInTransit = false;
const httpsForMain = Object.assign({}, realHttps, {
  get(url, options, callback) {
    httpsLog.push(String(url && url.href ? url.href : url));
    if (typeof options === 'function') { callback = options; options = {}; }
    return realHttps.get(url, options, (res) => {
      if (tamperInTransit && res.statusCode === 200) {
        const origOn = res.on.bind(res);
        let offset = 0;
        let flipped = false;
        res.on = (ev, fn) => {
          if (ev !== 'data') return origOn(ev, fn);
          return origOn('data', (chunk) => {
            if (!flipped && offset + chunk.length > 1024) {
              chunk = Buffer.from(chunk);
              chunk[1024 - offset] ^= 0xff;
              flipped = true;
            }
            offset += chunk.length;
            fn(chunk);
          });
        };
      }
      callback(res);
    });
  }
});

const origLoad = Module._load;
Module._load = function (request, parent, isMain) {
  const fromMain = parent && parent.filename && path.resolve(parent.filename) === MAIN_JS;
  if (request === 'electron') return electronMock;
  if (fromMain && request === 'child_process') return childProcessForMain;
  if (fromMain && request === 'https') return httpsForMain;
  return origLoad.apply(this, arguments);
};

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
let failures = 0;
function pass(msg) { console.log(`  [PASS] ${msg}`); }
function fail(msg) { console.log(`  [FAIL] ${msg}`); failures++; }
function check(cond, msg) { (cond ? pass : fail)(msg); }
function sha256File(p) { return crypto.createHash('sha256').update(fs.readFileSync(p)).digest('hex'); }
function flipByte(p, offset) {
  const fd = fs.openSync(p, 'r+');
  const b = Buffer.alloc(1);
  fs.readSync(fd, b, 0, 1, offset);
  const before = b[0];
  b[0] ^= 0xff;
  fs.writeSync(fd, b, 0, 1, offset);
  fs.closeSync(fd);
  return [before, b[0]];
}
const fakeEvent = {};

async function expectReject(promise) {
  try { await promise; return null; } catch (e) { return e; }
}

// ---------------------------------------------------------------------------
async function main() {
  console.log('=================================================================');
  console.log('  TEST: Updater handlers loaded from the REAL desktop/main.js');
  console.log('=================================================================');
  console.log(`  main.js path   : ${sanitize(MAIN_JS)}`);
  console.log(`  main.js sha256 : ${sha256File(MAIN_JS)}`);
  console.log(`  update dir     : ${sanitize(UPDATE_DIR)}`);

  require(MAIN_JS);

  const download = handlers.get('updater:downloadUpdate');
  const install = handlers.get('updater:installUpdate');
  console.log(`  IPC handlers registered by main.js: ${handlers.size}`);
  check(typeof download === 'function', "captured 'updater:downloadUpdate' from main.js");
  check(typeof install === 'function', "captured 'updater:installUpdate' from main.js");
  if (typeof download !== 'function' || typeof install !== 'function') throw new Error('handlers missing');

  // -------------------------------------------------------------------------
  console.log('\n[1] Non-allowed host https://evil.example/x.exe');
  httpsLog.length = 0;
  const e1 = await expectReject(download(fakeEvent, { downloadUrl: 'https://evil.example/x.exe', assetName: 'x.exe', expectedSha256: GOOD_SHA }));
  console.log(`  error: ${e1 ? sanitize(e1.message) : '(none)'}`);
  check(e1 && /Untrusted update download URL/.test(e1.message), 'rejected with Untrusted update download URL');
  check(httpsLog.length === 0, `no network request was made (https.get calls: ${httpsLog.length})`);
  check(!fs.existsSync(path.join(UPDATE_DIR, 'x.exe')), 'no file written');

  // -------------------------------------------------------------------------
  console.log('\n[2] Valid release URL + correct expectedSha256');
  httpsLog.length = 0;
  const r2 = await download(fakeEvent, { downloadUrl: RELEASE_URL, assetName: ASSET_NAME, expectedSha256: GOOD_SHA });
  const sanitizedResult2 = Object.assign({}, r2, { filePath: sanitize(r2.filePath) });
  console.log(`  result: ${JSON.stringify(sanitizedResult2)}`);
  console.log(`  requests: ${httpsLog.map(u => new URL(u).hostname).join(' -> ')}`);
  check(r2 && r2.success === true, 'downloadUpdate returned success');
  check(r2.sha256 === GOOD_SHA, 'returned sha256 matches expected');
  check(fs.existsSync(DEST) && fs.statSync(DEST).size === EXPECTED_SIZE, `file on disk, ${EXPECTED_SIZE} bytes`);
  check(sha256File(DEST) === GOOD_SHA, 'independent re-hash of file on disk matches');

  // -------------------------------------------------------------------------
  console.log('\n[3] installUpdate on verified file (spawn is a recorder; installer NOT executed)');
  spawnCalls.length = 0;
  const r3 = await install(fakeEvent, { filePath: DEST });
  const sanitizedSpawnCalls = spawnCalls.map(c => ({
    cmd: sanitize(c.cmd),
    detached: c.opts && c.opts.detached
  }));
  console.log(`  result: ${JSON.stringify(r3)}`);
  console.log(`  spawn calls: ${JSON.stringify(sanitizedSpawnCalls)}`);
  check(r3 && r3.action === 'spawned_installer', "returned action 'spawned_installer'");
  check(spawnCalls.length === 1 && path.resolve(spawnCalls[0].cmd) === path.resolve(DEST), 'spawn called once with the verified path');

  // -------------------------------------------------------------------------
  console.log('\n[4] One byte changed on disk AFTER verification, then installUpdate');
  spawnCalls.length = 0;
  const [b0, b1] = flipByte(DEST, 1024);
  console.log(`  offset 1024: 0x${b0.toString(16)} -> 0x${b1.toString(16)}`);
  const e4 = await expectReject(install(fakeEvent, { filePath: DEST }));
  console.log(`  error: ${e4 ? sanitize(e4.message) : '(none)'}`);
  check(e4 && /checksum changed before execution/.test(e4.message), 'installUpdate refused the modified file');
  check(!fs.existsSync(DEST), 'modified file deleted');
  check(spawnCalls.length === 0, `spawn not called (calls: ${spawnCalls.length})`);
  const e4b = await expectReject(install(fakeEvent, {}));
  console.log(`  follow-up install error: ${e4b ? sanitize(e4b.message) : '(none)'}`);
  check(e4b && /no verified update package/.test(e4b.message), 'verified state cleared; a retry is also refused');

  // -------------------------------------------------------------------------
  console.log('\n[5] One byte flipped IN TRANSIT during downloadUpdate');
  tamperInTransit = true;
  const e5 = await expectReject(download(fakeEvent, { downloadUrl: RELEASE_URL, assetName: ASSET_NAME, expectedSha256: GOOD_SHA }));
  tamperInTransit = false;
  console.log(`  error: ${e5 ? sanitize(e5.message) : '(none)'}`);
  check(e5 && /SHA-256 verification failed/.test(e5.message), 'downloadUpdate rejected tampered download');
  check(!fs.existsSync(DEST), 'tampered file deleted');

  // -------------------------------------------------------------------------
  console.log('\n[6] Wrong expectedSha256');
  const e6 = await expectReject(download(fakeEvent, { downloadUrl: RELEASE_URL, assetName: ASSET_NAME, expectedSha256: BAD_SHA }));
  console.log(`  error: ${e6 ? sanitize(e6.message) : '(none)'}`);
  check(e6 && /SHA-256 verification failed/.test(e6.message), 'downloadUpdate rejected mismatched hash');
  check(!fs.existsSync(DEST), 'unverified file deleted');
  const e6b = await expectReject(install(fakeEvent, {}));
  check(e6b && /no verified update package/.test(e6b.message), 'installUpdate refuses after failed verification');
}

main()
  .catch((err) => { console.log(`\n[FATAL] ${sanitize(err && err.stack ? err.stack : String(err))}`); failures++; })
  .finally(() => {
    try { fs.rmSync(TEST_TMP, { recursive: true, force: true }); } catch (_) {}
    console.log('\n=================================================================');
    console.log(failures === 0 ? '  ALL SCENARIOS PASSED' : `  ${failures} CHECK(S) FAILED`);
    console.log('=================================================================');
    // main.js schedules app.quit() 500 ms after install; exit explicitly.
    setTimeout(() => process.exit(failures === 0 ? 0 : 1), 700);
  });
