const { app, BrowserWindow, ipcMain, dialog, shell, Menu } = require('electron');
const path = require('path');
const fs = require('fs');
const os = require('os');
const { spawn } = require('child_process');

let mainWindow = null;

function findBinary(name) {
  const isWin = process.platform === 'win32';
  const targetName = isWin ? `${name}.exe` : name;

  // 1. Check user installed ~/.enlangg/bin
  const homeDir = process.env.USERPROFILE || process.env.HOME || '';
  const userBin = path.join(homeDir, '.enlangg', 'bin', targetName);
  if (fs.existsSync(userBin)) return userBin;

  // 2. Check repo root
  const repoBin = path.join(__dirname, '..', targetName);
  if (fs.existsSync(repoBin)) return repoBin;

  // 3. Fall back to name for PATH lookup
  return name;
}

function createWindow() {
  Menu.setApplicationMenu(null); // Remove default retro menu bar

  mainWindow = new BrowserWindow({
    width: 1440,
    height: 900,
    minWidth: 1024,
    minHeight: 640,
    title: 'Enlangg Studio',
    backgroundColor: '#0d1117',
    show: false, // Wait until ready-to-show for zero flicker
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

  const studioHtmlPath = path.join(__dirname, '..', 'website', 'studio.html');
  mainWindow.loadFile(studioHtmlPath);

  mainWindow.once('ready-to-show', () => {
    mainWindow.show();
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

// App Lifecycle
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
