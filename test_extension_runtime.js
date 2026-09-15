/**
 * Test suite for Enlangg Studio ExtensionHostRuntime
 * Verifies:
 * 1. Manifest Parsing (contributes.viewsContainers.activitybar, contributes.views)
 * 2. Activity Bar Icon Injection & Container Panes
 * 3. Webview View Provider (vscode.window.registerWebviewViewProvider)
 * 4. Bidirectional Message Passing (postMessage / onDidReceiveMessage RPC)
 * 5. Lifecycle Management & Cleanup (deactivateManifest)
 */

const assert = require('assert');
const fs = require('fs');

// Create a minimal mock DOM environment to test the runtime
class MockElement {
  constructor(tagName = 'div') {
    this.tagName = tagName.toUpperCase();
    this.id = '';
    this.className = '';
    this.classList = {
      _classes: new Set(),
      add: (c) => this.classList._classes.add(c),
      remove: (c) => this.classList._classes.delete(c),
      contains: (c) => this.classList._classes.has(c),
      toggle: (c, force) => {
        if (force === undefined) {
          if (this.classList._classes.has(c)) this.classList._classes.delete(c);
          else this.classList._classes.add(c);
        } else if (force) {
          this.classList._classes.add(c);
        } else {
          this.classList._classes.delete(c);
        }
      }
    };
    this.dataset = {};
    this.children = [];
    this.parentNode = null;
    this.innerHTML = '';
    this.style = {};
    this.attributes = {};
    this._listeners = {};
    this.srcdoc = '';
    this.contentWindow = {
      postMessage: (msg, targetOrigin) => {
        if (this._onIframeMessage) {
          this._onIframeMessage(msg, targetOrigin);
        }
        if (this.contentWindow && this.contentWindow._onIframeMessage) {
          this.contentWindow._onIframeMessage(msg, targetOrigin);
        }
      }
    };
  }

  setAttribute(name, val) {
    this.attributes[name] = val;
    if (name === 'id') this.id = val;
    if (name === 'class') this.className = val;
  }
  getAttribute(name) {
    return this.attributes[name] || null;
  }
  appendChild(child) {
    child.parentNode = this;
    this.children.push(child);
    mockElements.set(child.id, child);
    return child;
  }
  removeChild(child) {
    const idx = this.children.indexOf(child);
    if (idx !== -1) {
      this.children.splice(idx, 1);
      child.parentNode = null;
      if (child.id) mockElements.delete(child.id);
    }
    return child;
  }
  remove() {
    if (this.parentNode) {
      this.parentNode.removeChild(this);
    }
    if (this.id) mockElements.delete(this.id);
  }
  querySelector(sel) {
    return this.querySelectorAll(sel)[0] || null;
  }
  querySelectorAll(sel) {
    const results = [];
    const check = (node) => {
      if (sel.startsWith('#') && node.id === sel.slice(1)) results.push(node);
      else if (sel.startsWith('.') && (node.className.includes(sel.slice(1)) || node.classList.contains(sel.slice(1)))) results.push(node);
      else if (node.tagName.toLowerCase() === sel.toLowerCase()) results.push(node);
      for (const ch of node.children) check(ch);
    };
    for (const ch of this.children) check(ch);
    return results;
  }
  addEventListener(evt, fn) {
    if (!this._listeners[evt]) this._listeners[evt] = [];
    this._listeners[evt].push(fn);
  }
  removeEventListener(evt, fn) {
    if (this._listeners[evt]) {
      this._listeners[evt] = this._listeners[evt].filter(l => l !== fn);
    }
  }
  dispatchEvent(evt) {
    const list = this._listeners[evt.type] || [];
    list.forEach(l => l(evt));
  }
  click() {
    this.dispatchEvent({ type: 'click' });
  }
}

// Setup global mock DOM
const mockElements = new Map();
global.document = {
  createElement: (tag) => {
    const el = new MockElement(tag);
    return el;
  },
  getElementById: (id) => mockElements.get(id) || null,
  querySelector: (sel) => {
    if (sel.startsWith('#')) return mockElements.get(sel.slice(1)) || null;
    return null;
  },
  querySelectorAll: (sel) => []
};

// Top activity bar and sidebar containers
const activityBarExtensionsGroup = new MockElement('div');
activityBarExtensionsGroup.id = 'activityBarExtensionsGroup';
mockElements.set('activityBarExtensionsGroup', activityBarExtensionsGroup);

const sidebar = new MockElement('div');
sidebar.id = 'mainSidebar';
mockElements.set('mainSidebar', sidebar);

const windowListeners = {};
global.window = {
  addEventListener: (evt, fn) => {
    if (!windowListeners[evt]) windowListeners[evt] = [];
    windowListeners[evt].push(fn);
  },
  removeEventListener: (evt, fn) => {
    if (windowListeners[evt]) {
      windowListeners[evt] = windowListeners[evt].filter(l => l !== fn);
    }
  },
  triggerMessage: (data, source) => {
    (windowListeners['message'] || []).forEach(fn => fn({ data, source }));
  },
  showSidebarPane: (paneId) => {
    mockElements.set('_activeSidebar', paneId);
  }
};

// Mock localStorage
const localStorageMap = new Map();
global.localStorage = {
  getItem: (key) => localStorageMap.get(key) || null,
  setItem: (key, val) => localStorageMap.set(key, String(val)),
  removeItem: (key) => localStorageMap.delete(key),
  clear: () => localStorageMap.clear()
};

// Extract ExtensionHostRuntime from studio.js
const studioCode = fs.readFileSync('website/studio.js', 'utf8');
const lines = studioCode.split(/\r?\n/);
const startIdx = lines.findIndex(l => l.includes('class ExtensionHostRuntime {'));
let endIdx = -1;
for (let i = startIdx + 1; i < lines.length; i++) {
  if (lines[i] === '  }' && lines.slice(i + 1, i + 5).some(l => l.includes('Singleton instance'))) {
    endIdx = i + 1;
    break;
  }
}

if (startIdx === -1 || endIdx === -1) {
  throw new Error(`Failed to find ExtensionHostRuntime boundaries: startIdx=${startIdx}, endIdx=${endIdx}`);
}

const classCode = lines.slice(startIdx, endIdx).join('\n');

// Evaluate class in mock environment with stub helpers
const evalScript = `
function appendTerminal() {}
function showStudioToast() {}
function toggleSidebarPane() {}
function getInstalledExtensions() { return []; }
function detectExtensionEmoji() { return '🧩'; }
function detectExtensionCategory() { return 'AI'; }

${classCode}
return ExtensionHostRuntime;
`;
const ExtensionHostRuntime = new Function(evalScript)();

console.log('✅ ExtensionHostRuntime loaded successfully.');

async function runTests() {
  console.log('\n--- Running ExtensionHostRuntime Test Suite ---');
  const runtime = new ExtensionHostRuntime();
  global.window.extensionHostRuntime = runtime;
  global.window.vscode = {
    window: {
      registerWebviewViewProvider: (viewId, provider, options) => {
        return runtime.registerWebviewViewProvider(viewId, provider, options);
      }
    }
  };

  // Test 1: Register sample manifest with activitybar and views
  console.log('\n[Test 1] Manifest Parsing & Activity Bar Injection');
  const sampleManifest = {
    name: 'test-ai-assistant',
    displayName: 'Test AI Assistant',
    publisher: 'enlangg',
    version: '1.0.0',
    contributes: {
      viewsContainers: {
        activitybar: [
          {
            id: 'test-ai-container',
            title: 'AI Assistant',
            icon: 'media/robot.svg'
          }
        ]
      },
      views: {
        'test-ai-container': [
          {
            id: 'test-ai-chat-view',
            name: 'AI Chatbot',
            type: 'webview'
          }
        ]
      }
    }
  };

  const ext = {
    id: 'enlangg.test-ai-assistant',
    namespace: 'enlangg',
    name: 'test-ai-assistant',
    displayName: 'Test AI Assistant',
    version: '1.0.0',
    files: {
      icon: 'https://open-vsx.org/api/enlangg/test-ai-assistant/icon.png'
    }
  };
  const extId = 'enlangg.test-ai-assistant';

  // 1a: Parse contributions
  runtime.parseContributions(extId, sampleManifest);
  assert.strictEqual(runtime._viewContainers.size, 1, 'Expected 1 container parsed');
  assert.strictEqual(runtime._views.size, 1, 'Expected 1 view parsed');

  const containerData = runtime._viewContainers.get('test-ai-container');
  assert(containerData, 'Container data should exist');
  assert.strictEqual(containerData.title, 'AI Assistant');

  // 1b: Inject activity bar icon
  runtime.injectActivityBarIcon(containerData, ext);
  const actIcon = mockElements.get('act_manifest_test-ai-container');
  assert(actIcon, 'Expected activity bar icon with ID act_manifest_test-ai-container');
  assert(actIcon.title.includes('AI Assistant'), 'Expected title to include container title');
  console.log('✅ Manifest parsed & activity bar icon injected successfully.');

  // 1c: Register view in container
  const viewData = runtime._views.get('test-ai-chat-view');
  assert(viewData, 'View data should exist');
  runtime.registerView('test-ai-container', viewData, ext);

  const paneEl = mockElements.get('pane_manifest_test-ai-container');
  assert(paneEl, 'Expected container pane to be created in sidebar');

  const webviewFrame = runtime._webviewFrames.get('test-ai-chat-view');
  assert(webviewFrame, 'Expected webview iframe to be mounted');
  assert(webviewFrame.srcdoc.includes('acquireVsCodeApi'), 'Expected iframe srcdoc to contain VS Code API shim');
  assert(webviewFrame.srcdoc.includes('Content-Security-Policy'), 'Expected iframe srcdoc to contain CSP');
  console.log('✅ Container pane created and sandboxed webview mounted.');

  // Test 2: Register Webview View Provider (vscode.window.registerWebviewViewProvider)
  console.log('\n[Test 2] Webview View Provider (vscode.window.registerWebviewViewProvider)');
  let providerResolved = false;
  let receivedWebview = null;

  const mockProvider = {
    resolveWebviewView(webviewView, context, token) {
      providerResolved = true;
      receivedWebview = webviewView.webview;
      webviewView.webview.html = `<!DOCTYPE html><html><body><div id="ai-chat">Hello Enlangg!</div></body></html>`;
    }
  };

  const disposable = window.vscode.window.registerWebviewViewProvider('test-ai-chat-view', mockProvider);
  assert(providerResolved, 'Expected provider.resolveWebviewView to be called immediately upon registration');
  assert(receivedWebview, 'Expected webview instance to be provided');
  assert(receivedWebview.html.includes('Hello Enlangg!'), 'Expected webview HTML setter to update srcdoc');
  console.log('✅ Webview View Provider registered and resolved correctly.');

  // Test 3: Bidirectional Message Passing (postMessage / onDidReceiveMessage RPC)
  console.log('\n[Test 3] Bidirectional Message Passing (postMessage / onDidReceiveMessage)');
  let messageToWebview = null;
  let messageToHost = null;

  // Intercept messages posted to iframe.contentWindow
  webviewFrame.contentWindow._onIframeMessage = (msg, targetOrigin) => {
    messageToWebview = msg;
  };

  // Host extension listens for messages from webview
  receivedWebview.onDidReceiveMessage((data) => {
    messageToHost = data;
  });

  // 3a: Host extension calls postMessage -> iframe receives it
  const hostPayload = { command: 'configure', model: 'sovereign-ai-v2' };
  receivedWebview.postMessage(hostPayload);
  assert.deepStrictEqual(messageToWebview, {
    type: 'webview-rpc',
    direction: 'host-to-webview',
    viewId: 'test-ai-chat-view',
    payload: hostPayload
  }, 'Expected iframe to receive host postMessage payload');
  console.log('✅ Host -> Webview postMessage verified.');

  // 3b: Webview posts message -> Host receives it
  const webviewPayload = { command: 'prompt', text: 'How do Enlang pair loops work?' };
  global.window.triggerMessage({
    type: 'webview-rpc',
    direction: 'webview-to-host',
    viewId: 'test-ai-chat-view',
    payload: webviewPayload
  }, webviewFrame.contentWindow);

  assert.deepStrictEqual(messageToHost, webviewPayload, 'Expected host to receive webview message');
  console.log('✅ Webview -> Host onDidReceiveMessage RPC verified.');

  // Test 4: Deactivation and Lifecycle Cleanup
  console.log('\n[Test 4] Extension Deactivation & Cleanup (deactivateManifest)');
  runtime.deactivateManifest(extId);

  const actIconAfter = mockElements.get('act_manifest_test-ai-container');
  assert(!actIconAfter, 'Expected activity bar icon to be removed on deactivation');

  const paneAfter = mockElements.get('pane_manifest_test-ai-container');
  assert(!paneAfter, 'Expected sidebar pane to be removed on deactivation');

  assert(!runtime._webviewFrames.has('test-ai-chat-view'), 'Expected webview frame map to be cleared');
  assert(!runtime._views.has('test-ai-chat-view'), 'Expected view map to be cleared');
  console.log('✅ Deactivation cleanly cleared all DOM elements, views, and frames.');

  console.log('\n🎉 ALL 4 EXTENSION RUNTIME ARCHITECTURE SUITES PASSED WITH 100% SUCCESS!\n');
}

runTests().catch(err => {
  console.error('❌ Test failed:', err);
  process.exit(1);
});
