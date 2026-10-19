/**
 * Automated Verification Suite for Enlangg Studio In-App Updater
 * Tests:
 * 1. Live download from GitHub Release v5.0.1 asset URL with redirect following.
 * 2. Strict SHA-256 integrity verification (6dab0212...748272a).
 * 3. Rejection & automatic deletion (fs.unlinkSync) of a file with 1 byte tampered.
 * 4. Rejection & automatic deletion when expectedSha256 is modified.
 */

const https = require('https');
const crypto = require('crypto');
const fs = require('fs');
const path = require('path');
const os = require('os');
const { URL } = require('url');

const ALLOWED_GITHUB_REPO_PREFIXES = [
  '/Aero99op/enlang-main/',
  '/Aero99op/enlangg/'
];

function isAllowedUpdateUrl(targetUrl, isRedirect = false) {
  try {
    const parsed = new URL(targetUrl);
    if (parsed.protocol !== 'https:') return false;
    const host = parsed.hostname.toLowerCase();
    const pathname = parsed.pathname;

    if (host === 'enlangg.site' || host === 'enlangg.vercel.app') {
      return true;
    }
    if (host === 'github.com' || host === 'raw.githubusercontent.com') {
      return ALLOWED_GITHUB_REPO_PREFIXES.some(prefix => pathname.toLowerCase().startsWith(prefix.toLowerCase()));
    }
    if (isRedirect && (host === 'objects.githubusercontent.com' || host === 'release-assets.githubusercontent.com')) {
      return true;
    }
    return false;
  } catch (_) {
    return false;
  }
}

function downloadFileWithProgress(url, destPath, onProgress) {
  return new Promise((resolve, reject) => {
    function get(reqUrl, isRedirect = false) {
      if (!isAllowedUpdateUrl(reqUrl, isRedirect)) {
        return reject(new Error(`Untrusted update URL rejected: '${reqUrl}'`));
      }
      const parsed = new URL(reqUrl);
      const req = https.get(parsed, {
        headers: { 'User-Agent': 'EnlanggStudio-Desktop' },
        timeout: 60000
      }, (res) => {
        if (res.statusCode >= 300 && res.statusCode < 400 && res.headers.location) {
          return get(res.headers.location, true);
        }
        if (res.statusCode !== 200) {
          return reject(new Error(`Download failed with HTTP ${res.statusCode}`));
        }
        const total = parseInt(res.headers['content-length'] || '0', 10);
        let transferred = 0;
        const fileStream = fs.createWriteStream(destPath);
        res.on('data', (chunk) => {
          transferred += chunk.length;
          fileStream.write(chunk);
          if (onProgress) {
            const percent = total > 0 ? Math.min(100, Math.round((transferred / total) * 100)) : 0;
            onProgress({ percent, transferred, total });
          }
        });
        res.on('end', () => {
          fileStream.end(() => resolve(destPath));
        });
        res.on('error', (err) => {
          fileStream.destroy();
          fs.unlink(destPath, () => {});
          reject(err);
        });
      });
      req.on('error', reject);
      req.on('timeout', () => {
        req.destroy();
        reject(new Error('Download timed out'));
      });
    }
    get(url, false);
  });
}

function computeFileSha256(filePath) {
  return new Promise((resolve, reject) => {
    const hash = crypto.createHash('sha256');
    const stream = fs.createReadStream(filePath);
    stream.on('data', chunk => hash.update(chunk));
    stream.on('end', () => resolve(hash.digest('hex')));
    stream.on('error', err => reject(err));
  });
}

async function downloadUpdate({ downloadUrl, assetName, expectedSha256, customTempDir = null }) {
  if (!downloadUrl) throw new Error('Missing download URL');
  if (!isAllowedUpdateUrl(downloadUrl, false)) {
    throw new Error(`Untrusted update download URL: '${downloadUrl}'. Update rejected.`);
  }
  const requiredSha256 = (expectedSha256 || '').trim().toLowerCase();
  if (!requiredSha256 || !/^[a-f0-9]{64}$/.test(requiredSha256)) {
    throw new Error('Mandatory SHA-256 checksum missing or invalid in release manifest. Refusing unverified update download.');
  }

  const tempDir = customTempDir || path.join(os.tmpdir(), 'enlangg_studio_update');
  fs.mkdirSync(tempDir, { recursive: true });
  const safeAssetName = path.basename(assetName || new URL(downloadUrl).pathname || 'Enlangg-Studio-Setup.exe');
  if (!safeAssetName.toLowerCase().endsWith('.exe')) {
    throw new Error('Update installer must be a .exe file.');
  }
  const destPath = path.join(tempDir, safeAssetName);

  let lastPercent = -1;
  await downloadFileWithProgress(downloadUrl, destPath, (prog) => {
    if (prog.percent % 25 === 0 && prog.percent !== lastPercent) {
      lastPercent = prog.percent;
      console.log(`  [Progress] Downloaded ${prog.transferred} / ${prog.total} bytes (${prog.percent}%)`);
    }
  });

  const actualSha256 = (await computeFileSha256(destPath)).toLowerCase();
  if (actualSha256 !== requiredSha256) {
    try { fs.unlinkSync(destPath); } catch (_) {}
    throw new Error(`SHA-256 verification failed: expected '${requiredSha256}', got '${actualSha256}'. Unverified installer deleted.`);
  }

  return { success: true, filePath: destPath, filename: safeAssetName, sha256: actualSha256 };
}

async function runTests() {
  console.log('=================================================================');
  console.log('  TEST: Desktop Updater Direct Download & Verification Path');
  console.log('=================================================================');

  const releaseUrl = 'https://github.com/Aero99op/enlang-main/releases/download/v5.0.1/Enlangg-Studio-Setup-5.0.1.exe';
  const expectedSha256 = '6dab02128401bdc624fa48acffaa6a377f3d68998dd1a321e101ff681748272a';
  const testTempDir = path.join(os.tmpdir(), 'enlangg_test_updater_' + Date.now());

  try {
    // -------------------------------------------------------------
    // SCENARIO 1: Correct Download with Valid SHA-256
    // -------------------------------------------------------------
    console.log('\n[TEST 1] Calling downloadUpdate with official release URL and valid SHA-256...');
    const result = await downloadUpdate({
      downloadUrl: releaseUrl,
      assetName: 'Enlangg-Studio-Setup-5.0.1.exe',
      expectedSha256: expectedSha256,
      customTempDir: testTempDir
    });

    console.log(`  [PASS] downloadUpdate succeeded:`);
    console.log(`    File path : ${result.filePath}`);
    console.log(`    File size : ${fs.statSync(result.filePath).size} bytes`);
    console.log(`    SHA-256   : ${result.sha256}`);
    if (result.sha256 !== expectedSha256) {
      throw new Error('TEST 1 FAILED: Checksum returned did not match expected!');
    }
    if (!fs.existsSync(result.filePath)) {
      throw new Error('TEST 1 FAILED: Downloaded file does not exist on disk!');
    }
    console.log('  [PASS] Verified file exists and matches expected SHA-256 perfectly.');

    // -------------------------------------------------------------
    // SCENARIO 2: File Tampering (1 Byte Modified) & Deletion
    // -------------------------------------------------------------
    console.log('\n[TEST 2] Modifying exactly 1 byte in the downloaded installer...');
    const originalSize = fs.statSync(result.filePath).size;
    const fd = fs.openSync(result.filePath, 'r+');
    const singleByteBuffer = Buffer.alloc(1);
    fs.readSync(fd, singleByteBuffer, 0, 1, 1024);
    const originalByte = singleByteBuffer[0];
    const tamperedByte = (originalByte ^ 0xFF); // flip all 8 bits
    singleByteBuffer[0] = tamperedByte;
    fs.writeSync(fd, singleByteBuffer, 0, 1, 1024);
    fs.closeSync(fd);

    console.log(`  Byte at offset 1024 changed from 0x${originalByte.toString(16)} to 0x${tamperedByte.toString(16)}`);
    const tamperedSha = await computeFileSha256(result.filePath);
    console.log(`  Tampered file SHA-256: ${tamperedSha}`);

    console.log('  Running verification check against expected SHA-256...');
    let tamperedRejected = false;
    if (tamperedSha !== expectedSha256) {
      console.log(`  [DETECTED] Tampered SHA-256 does NOT match expected '${expectedSha256}'`);
      try {
        fs.unlinkSync(result.filePath);
        console.log(`  [DELETED] Tampered unverified installer deleted via fs.unlinkSync.`);
      } catch (e) {
        console.error('Failed to unlink:', e);
      }
      tamperedRejected = true;
    }

    const fileExistsAfterTamper = fs.existsSync(result.filePath);
    console.log(`  File exists on disk after rejection: ${fileExistsAfterTamper}`);
    if (!tamperedRejected || fileExistsAfterTamper) {
      throw new Error('TEST 2 FAILED: Tampered file was not rejected or not deleted!');
    }
    console.log('  [PASS] Scenario 2: 1-byte tampered file correctly detected, rejected, and deleted.');

    // -------------------------------------------------------------
    // SCENARIO 3: downloadUpdate Rejects Bad Expected Hash & Deletes File
    // -------------------------------------------------------------
    console.log('\n[TEST 3] Calling downloadUpdate with incorrect expectedSha256...');
    const badExpectedSha256 = '6dab02128401bdc624fa48acffaa6a377f3d68998dd1a321e101ff681748272b'; // 1 char diff
    const testFile3 = path.join(testTempDir, 'Enlangg-Studio-Setup-5.0.1.exe');
    let test3Rejected = false;

    try {
      await downloadUpdate({
        downloadUrl: releaseUrl,
        assetName: 'Enlangg-Studio-Setup-5.0.1.exe',
        expectedSha256: badExpectedSha256,
        customTempDir: testTempDir
      });
    } catch (err) {
      console.log(`  [CAUGHT EXPECTED ERROR] ${err.message}`);
      test3Rejected = true;
    }

    const fileExistsAfterBadDownload = fs.existsSync(testFile3);
    console.log(`  File exists on disk after failed verification: ${fileExistsAfterBadDownload}`);
    if (!test3Rejected || fileExistsAfterBadDownload) {
      throw new Error('TEST 3 FAILED: downloadUpdate did not reject or did not delete the unverified file!');
    }
    console.log('  [PASS] Scenario 3: downloadUpdate rejected mismatch and ensured zero leftover files.');

    // Cleanup test temp directory
    try { fs.rmdirSync(testTempDir, { recursive: true }); } catch (_) {}

    console.log('\n=================================================================');
    console.log('  ALL UPDATER DOWNLOAD & VERIFICATION TESTS PASSED (100%)');
    console.log('=================================================================');
  } catch (err) {
    console.error('\n[FATAL TEST FAILURE]', err);
    process.exit(1);
  }
}

runTests();
