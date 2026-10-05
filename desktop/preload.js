const { contextBridge, ipcRenderer } = require('electron');

contextBridge.exposeInMainWorld('EnlangElectron', {
  isElectron: true,
  platform: process.platform,

  // 1. Native Compiler Execution
  runCompiler: (data) => ipcRenderer.invoke('compiler:run', data),

  // 2. Native File System APIs
  openDirectoryDialog: () => ipcRenderer.invoke('fs:openDirectoryDialog'),
  openFileDialog: (options) => ipcRenderer.invoke('fs:openFileDialog', options),
  saveFileDialog: (options) => ipcRenderer.invoke('fs:saveFileDialog', options),
  listDirectory: (dirPath) => ipcRenderer.invoke('fs:listDirectory', dirPath),
  readFile: (filePath) => ipcRenderer.invoke('fs:readFile', filePath),
  writeFile: (filePath, content) => ipcRenderer.invoke('fs:writeFile', filePath, content),
  createFile: (filePath) => ipcRenderer.invoke('fs:createFile', filePath),
  createFolder: (dirPath) => ipcRenderer.invoke('fs:createFolder', dirPath),

  // 3. System Shell APIs
  openExternal: (url) => ipcRenderer.invoke('shell:openExternal', url),
  showInFolder: (filePath) => ipcRenderer.invoke('shell:showInFolder', filePath),

  // 4. Setup & Environment APIs
  getToolchainStatus: () => ipcRenderer.invoke('env:getToolchainStatus'),
  addToPath: () => ipcRenderer.invoke('env:addToPath'),
  openSystemFolder: (target) => ipcRenderer.invoke('env:openFolder', target),
  launchPureVSCode: (workspacePath) => ipcRenderer.invoke('env:launchPureVSCode', workspacePath),

  // 5. Auto-Updater APIs ("Update in a Go")
  checkForUpdates: () => ipcRenderer.invoke('updater:checkForUpdates'),
  downloadUpdate: (options) => ipcRenderer.invoke('updater:downloadUpdate', options),
  installUpdate: (options) => ipcRenderer.invoke('updater:installUpdate', options),
  onUpdateAvailable: (callback) => {
    const handler = (event, updateInfo) => callback(updateInfo);
    ipcRenderer.on('updater:updateAvailable', handler);
    return () => ipcRenderer.removeListener('updater:updateAvailable', handler);
  },
  onDownloadProgress: (callback) => {
    const handler = (event, progress) => callback(progress);
    ipcRenderer.on('updater:downloadProgress', handler);
    return () => ipcRenderer.removeListener('updater:downloadProgress', handler);
  }
});

