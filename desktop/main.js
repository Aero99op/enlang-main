const { app, BrowserWindow, ipcMain, dialog, shell, Menu, screen } = require('electron');
const path = require('path');
const fs = require('fs');
const os = require('os');
const { spawn } = require('child_process');

let mainWindow = null;

function findBinary(name) {
  const isWin = process.platform === 'win32';
  const targetName = isWin ? `${name}.exe` : name;

  // 1. Packaged App resources/bin directory (Standalone installer installation)
  if (process.resourcesPath) {
    const packagedBin = path.join(process.resourcesPath, 'bin', targetName);
    if (fs.existsSync(packagedBin)) return packagedBin;
  }

  // 2. Desktop bin directory (development / local bundled)
  const localBin = path.join(__dirname, 'bin', targetName);
  if (fs.existsSync(localBin)) return localBin;

  // 3. User installed ~/.enlangg/bin
  const homeDir = process.env.USERPROFILE || process.env.HOME || '';
  const userBin = path.join(homeDir, '.enlangg', 'bin', targetName);
  if (fs.existsSync(userBin)) return userBin;

  // 4. Repo root binary
  const repoBin = path.join(__dirname, '..', targetName);
  if (fs.existsSync(repoBin)) return repoBin;

  // 5. Fall back to targetName for PATH lookup
  return targetName;
}

function createWindow() {
  Menu.setApplicationMenu(null); // Remove default retro menu bar

  const iconPath = path.join(__dirname, 'icon.ico');

  let winWidth = 1440;
  let winHeight = 900;
  try {
    const primaryDisplay = screen.getPrimaryDisplay();
    if (primaryDisplay && primaryDisplay.workAreaSize) {
      winWidth = Math.min(1440, Math.max(1024, Math.floor(primaryDisplay.workAreaSize.width * 0.92)));
      winHeight = Math.min(900, Math.max(640, Math.floor(primaryDisplay.workAreaSize.height * 0.9)));
    }
  } catch (_) {}

  mainWindow = new BrowserWindow({
    width: winWidth,
    height: winHeight,
    minWidth: 1024,
    minHeight: 640,
    center: true,
    title: 'Enlangg Studio',
    icon: fs.existsSync(iconPath) ? iconPath : undefined,
    backgroundColor: '#0d1117',
    show: true, // Immediately show window to prevent hidden process hang
    titleBarStyle: 'hidden',
    titleBarOverlay: {
      color: '#18181b',
      symbolColor: '#e4e4e7',
      height: 35
    },
    webPreferences: {
      preload: path.join(__dirname, 'preload.js'),
      contextIsolation: true,
      nodeIntegration: false,
      sandbox: false,
      spellcheck: false
    }
  });

  const bundledStudioPath = path.join(__dirname, 'ui', 'studio.html');
  const devStudioPath = path.join(__dirname, '..', 'website', 'studio.html');
  const studioHtmlPath = fs.existsSync(bundledStudioPath) ? bundledStudioPath : devStudioPath;
  mainWindow.loadFile(studioHtmlPath);

  // Guarantee window is shown and focused even if ready-to-show event is delayed
  mainWindow.once('ready-to-show', () => {
    if (mainWindow && !mainWindow.isVisible()) {
      mainWindow.show();
    }
  });

  mainWindow.webContents.on('did-finish-load', () => {
    if (mainWindow && !mainWindow.isVisible()) {
      mainWindow.show();
    }
  });

  mainWindow.webContents.on('did-fail-load', (event, errorCode, errorDescription, validatedURL) => {
    console.error(`[Enlangg Studio] Failed to load ${validatedURL}: ${errorCode} - ${errorDescription}`);
    if (mainWindow && !mainWindow.isVisible()) {
      mainWindow.show();
    }
  });

  mainWindow.webContents.setWindowOpenHandler(({ url }) => {
    shell.openExternal(url);
    return { action: 'deny' };
  });

  mainWindow.on('closed', () => {
    mainWindow = null;
  });
}

// --- IPC: Direct Native Compiler Execution ---
ipcMain.handle('compiler:run', async (event, { filename, content, domain }) => {
  return new Promise((resolve) => {
    const t0 = performance.now();
    const safeName = path.basename(filename || 'sandbox.enlng');
    const tempDir = fs.mkdtempSync(path.join(os.tmpdir(), 'enlang_studio_'));
    const tempFile = path.join(tempDir, safeName);

    try {
      fs.writeFileSync(tempFile, content, 'utf8');

      const isDb = domain === 'enlngdb' || safeName.endsWith('.enlngdb');
      const binName = isDb ? 'enlngdb' : 'enlng';
      const binPath = findBinary(binName);

      const proc = spawn(binPath, [tempFile]);
      let stdout = '';
      let stderr = '';

      proc.stdout.on('data', (data) => {
        stdout += data.toString();
      });

      proc.stderr.on('data', (data) => {
        stderr += data.toString();
      });

      proc.on('close', (code) => {
        const timeMs = (performance.now() - t0).toFixed(2);
        const output = code === 0 ? stdout : (stderr || stdout);
        
        try {
          fs.unlinkSync(tempFile);
          fs.rmdirSync(tempDir);
        } catch (_) {}

        resolve({
          success: code === 0,
          exitCode: code,
          output: output || (code === 0 ? '// [Execution completed with 0 output]' : 'Unknown error'),
          stdout,
          stderr,
          timeMs,
          executor: binPath
        });
      });

      proc.on('error', (err) => {
        const timeMs = (performance.now() - t0).toFixed(2);
        resolve({
          success: false,
          exitCode: 1,
          output: `Failed to spawn native compiler binary '${binPath}': ${err.message}`,
          stdout: '',
          stderr: err.message,
          timeMs,
          executor: binPath
        });
      });
    } catch (e) {
      const timeMs = (performance.now() - t0).toFixed(2);
      resolve({
        success: false,
        exitCode: 1,
        output: `File I/O error: ${e.message}`,
        stdout: '',
        stderr: e.message,
        timeMs,
        executor: 'Native Host'
      });
    }
  });
});

// --- IPC: Native File System Operations ---
ipcMain.handle('fs:openDirectoryDialog', async () => {
  const result = await dialog.showOpenDialog(mainWindow, {
    title: 'Open Enlang Project Folder',
    properties: ['openDirectory', 'createDirectory']
  });
  if (result.canceled || !result.filePaths || result.filePaths.length === 0) {
    return null;
  }
  return result.filePaths[0];
});

ipcMain.handle('fs:openFileDialog', async (event, options = {}) => {
  const result = await dialog.showOpenDialog(mainWindow, {
    title: options.title || 'Open Enlang File',
    filters: [
      { name: 'Enlang Files', extensions: ['enlng', 'enlngdb', 'enlngf', 'enlngd', 'enlngs', 'enlngm'] },
      { name: 'All Files', extensions: ['*'] }
    ],
    properties: ['openFile']
  });
  if (result.canceled || !result.filePaths || result.filePaths.length === 0) {
    return null;
  }
  const filePath = result.filePaths[0];
  const content = fs.readFileSync(filePath, 'utf8');
  return { path: filePath, name: path.basename(filePath), content };
});

ipcMain.handle('fs:saveFileDialog', async (event, options = {}) => {
  const result = await dialog.showSaveDialog(mainWindow, {
    title: options.title || 'Save Enlang File As',
    defaultPath: options.defaultPath || 'script.enlng',
    filters: [
      { name: 'Enlang Files', extensions: ['enlng', 'enlngdb', 'enlngf', 'enlngd', 'enlngs', 'enlngm'] },
      { name: 'All Files', extensions: ['*'] }
    ]
  });
  if (result.canceled || !result.filePath) {
    return null;
  }
  return result.filePath;
});

ipcMain.handle('fs:listDirectory', async (event, dirPath) => {
  if (!fs.existsSync(dirPath)) return [];
  try {
    const entries = fs.readdirSync(dirPath, { withFileTypes: true });
    const items = [];
    for (const ent of entries) {
      if (ent.name.startsWith('.') || ent.name === 'node_modules' || ent.name === '__pycache__') {
        continue;
      }
      const fullPath = path.join(dirPath, ent.name);
      items.push({
        name: ent.name,
        path: fullPath,
        isDirectory: ent.isDirectory(),
        size: ent.isDirectory() ? 0 : fs.statSync(fullPath).size
      });
    }
    // Sort directories first, then alphabetical
    items.sort((a, b) => {
      if (a.isDirectory && !b.isDirectory) return -1;
      if (!a.isDirectory && b.isDirectory) return 1;
      return a.name.localeCompare(b.name);
    });
    return items;
  } catch (err) {
    return [];
  }
});

ipcMain.handle('fs:readFile', async (event, filePath) => {
  try {
    return fs.readFileSync(filePath, 'utf8');
  } catch (err) {
    throw new Error(`Cannot read file: ${err.message}`);
  }
});

ipcMain.handle('fs:writeFile', async (event, filePath, content) => {
  try {
    fs.mkdirSync(path.dirname(filePath), { recursive: true });
    fs.writeFileSync(filePath, content, 'utf8');
    return true;
  } catch (err) {
    throw new Error(`Cannot write file: ${err.message}`);
  }
});

ipcMain.handle('fs:createFile', async (event, filePath) => {
  try {
    fs.mkdirSync(path.dirname(filePath), { recursive: true });
    if (!fs.existsSync(filePath)) {
      fs.writeFileSync(filePath, '', 'utf8');
    }
    return true;
  } catch (err) {
    throw new Error(`Cannot create file: ${err.message}`);
  }
});

ipcMain.handle('fs:createFolder', async (event, dirPath) => {
  try {
    fs.mkdirSync(dirPath, { recursive: true });
    return true;
  } catch (err) {
    throw new Error(`Cannot create folder: ${err.message}`);
  }
});

// --- IPC: Shell Utilities ---
ipcMain.handle('shell:openExternal', async (event, url) => {
  return shell.openExternal(url);
});

ipcMain.handle('shell:showInFolder', async (event, filePath) => {
  return shell.showItemInFolder(filePath);
});

// --- IPC: Environment & Setup Wizard ---
ipcMain.handle('env:getToolchainStatus', async () => {
  const compilers = [
    { name: 'enlng', desc: 'Pure C99 Logic Engine (.enlng)' },
    { name: 'enlngdb', desc: 'Pure C Microsecond Database Engine (.enlngdb)' },
    { name: 'enlangg', desc: 'Sovereign Master CLI Runner' },
    { name: 'enlngf', desc: 'Sovereign Frontend UI Markup Engine (.enlngf)' },
    { name: 'enlngd', desc: 'Sovereign Design Token CSS Compiler (.enlngd)' },
    { name: 'enlngs', desc: 'Sovereign Browser Reactivity Engine (.enlngs)' },
    { name: 'enlngm', desc: 'Sovereign Mobile & Math Engine (.enlngm)' }
  ];

  const binaries = [];
  for (const c of compilers) {
    const binPath = findBinary(c.name);
    const exists = fs.existsSync(binPath);
    let details = 'Not Found';
    if (exists) {
      try {
        const stats = fs.statSync(binPath);
        details = `${(stats.size / 1024).toFixed(0)} KB · Ready`;
      } catch (_) {
        details = 'Ready';
      }
    }
    binaries.push({
      name: c.name,
      desc: c.desc,
      path: binPath,
      exists,
      details
    });
  }

  const bundledBinDir = process.resourcesPath ? path.join(process.resourcesPath, 'bin') : path.join(__dirname, 'bin');
  const userHomeBin = path.join(process.env.USERPROFILE || process.env.HOME || '', '.enlangg', 'bin');
  const userPath = process.env.PATH || '';
  const inPath = userPath.toLowerCase().includes(bundledBinDir.toLowerCase()) || userPath.toLowerCase().includes(userHomeBin.toLowerCase());

  return {
    isPackaged: app.isPackaged,
    installPath: process.resourcesPath ? path.dirname(process.resourcesPath) : path.join(__dirname, '..'),
    bundledBinDir,
    userHomeBin,
    inPath,
    binaries,
    platform: process.platform,
    arch: process.arch,
    electronVersion: process.versions.electron,
    nodeVersion: process.versions.node
  };
});

ipcMain.handle('env:addToPath', async () => {
  if (process.platform !== 'win32') return { success: false, message: 'Only supported on Windows' };
  try {
    const bundledBinDir = process.resourcesPath ? path.join(process.resourcesPath, 'bin') : path.join(__dirname, 'bin');
    const { execSync } = require('child_process');
    const currentPath = execSync('powershell -NoProfile -Command "[Environment]::GetEnvironmentVariable(\'Path\', \'User\')"').toString().trim();
    if (!currentPath.toLowerCase().includes(bundledBinDir.toLowerCase())) {
      const newPath = currentPath ? `${currentPath};${bundledBinDir}` : bundledBinDir;
      execSync(`powershell -NoProfile -Command "[Environment]::SetEnvironmentVariable('Path', '${newPath.replace(/'/g, "''")}', 'User')"`);
      return { success: true, message: `Successfully registered ${bundledBinDir} in Windows User PATH.` };
    } else {
      return { success: true, message: 'Compilers folder is already configured in Windows User PATH.' };
    }
  } catch (err) {
    return { success: false, message: `Failed to register PATH: ${err.message}` };
  }
});

ipcMain.handle('env:openFolder', async (event, target) => {
  let targetPath = '';
  if (target === 'install') {
    targetPath = process.resourcesPath ? path.dirname(process.resourcesPath) : path.join(__dirname, '..');
  } else if (target === 'bin') {
    targetPath = process.resourcesPath ? path.join(process.resourcesPath, 'bin') : path.join(__dirname, 'bin');
  } else if (target === 'userData') {
    targetPath = app.getPath('userData');
  }
  if (targetPath && fs.existsSync(targetPath)) {
    shell.openPath(targetPath);
    return true;
  }
  return false;
});

// App Lifecycle with Single Instance Lock
const gotTheLock = app.requestSingleInstanceLock();

if (!gotTheLock) {
  app.quit();
} else {
  app.on('second-instance', () => {
    if (mainWindow) {
      if (mainWindow.isMinimized()) mainWindow.restore();
      mainWindow.show();
      mainWindow.focus();
    }
  });

  app.whenReady().then(createWindow);

  app.on('window-all-closed', () => {
    if (process.platform !== 'darwin') {
      app.quit();
    }
  });

  app.on('activate', () => {
    if (mainWindow === null) {
      createWindow();
    }
  });
}
