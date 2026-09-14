// ==============================================================================
// 👑 ENLANGG STUDIO - 1:1 VS CODE NATIVE CLONE & HOME IDE CONTROLLER
// High-Performance Sovereign IDE Engine · Multi-Domain VM · BYOK AI Copilot
// ==============================================================================

(function () {
  'use strict';

  // 1. Initial Sample Workspace Templates
  const DEFAULT_WORKSPACE = {
    'src/main.enlng': `type enlng

# ==============================================================================
# 👑 ENLNG SOVEREIGN CORE LEDGER & SPATIAL ENGINE
# Native Enlangg Studio Runtime · Zero GC Pauses
# ==============================================================================

freeze INSTITUTION as "Sovereign Reserve Bank"
freeze MIN_BALANCE as 1000.0

remember primary_account as {
    "holder": "Aero Henderson",
    "balance": 85000.0,
    "status": "ACTIVE",
    "tier": "VIP"
}

function verify_transfer with account, amount:
    remember current_bal as balance of account
    when status of account is not "ACTIVE":
        give "ACCOUNT_INACTIVE"
    when amount > current_bal:
        give "INSUFFICIENT_FUNDS"
    give "APPROVED"

show "=========================================================================="
show "🏛️ " INSTITUTION "// CORE BANKING CORE"
show "=========================================================================="
show "Account Holder:" holder of primary_account
show "Opening Balance: $" primary_account.balance

remember transactions as [4500.0, 18200.0, 1250.0]
for tx in transactions:
    remember auth as verify_transfer with primary_account, tx
    when auth is "APPROVED":
        balance of primary_account decreases by tx
        show "   [SETTLED] Transferred $" tx "successfully."

show "Final Vault Balance: $" primary_account.balance

# Spatial Pair Sorting Demo
remember amounts as [18200.0, 4500.0, 1250.0]
show "Before Spatial Sort:" amounts

repeat until sorted:
    for each pair in amounts:
        when the left of the pair is greater than the right of the pair:
            swap pair

show "After Spatial Sort:" amounts
show "All Sovereign Grammar Invariants 100% Satisfied!"
`,

    'db/schema.enlngdb': `type enlngdb

-- 📊 EnlangDB Embedded Flat-File Relational Database
-- Demonstrating ultra-fast sub-0.05ms in-memory queries

use database database1;

-- 1. View registered accounts
find all records from accounts;

-- 2. View student registry
find all records from student;

-- 3. Discover active schema
show tables;
`,

    'ui/dashboard.enlngf': `type enlngf

-- 🎨 Sovereign Semantic UI Component Tree
component BankDashboard with account_name, balance:
    container styled as "fintech-card":
        header:
            title "Sovereign Enterprise Vault" with style "heading-1"
            badge "VIP Active" with style "status-online"
        metric-row:
            label "Liquid Reserve"
            value balance with style "currency-display"
        actions:
            button "Transfer Funds" with action "open_modal"
            button "Audit Trail" with action "export_csv"
`,

    'styles/theme.enlngd': `type enlngd

-- 💎 Enlangg Design Token & Hardware CSS Engine
palette SovereignTheme:
    color-background: #0f172a
    color-surface: #1e293b
    color-accent: #38bdf8
    color-gold: #fbbf24
    color-success: #34d399

tokens Typography:
    font-primary: "Inter", sans-serif
    font-mono: "JetBrains Mono", monospace

component "fintech-card":
    background: var(color-surface)
    border: 1px solid rgba(255, 255, 255, 0.1)
    border-radius: 16px
    padding: 24px
    box-shadow: 0 10px 30px rgba(0, 0, 0, 0.5)
`,

    'api/routes.enlngs': `type enlngs

-- 🌐 Enlangg Reactive Server & Network Services
listen on port 8080

route get "/api/v1/ledger":
    respond with status 200 and json {"status": "healthy", "ledger_ready": true}

route post "/api/v1/transfer" with body payload:
    verify payload.token and respond with json {"tx_id": "TX-902", "settled": true}
`,

    'mobile/wallet.enlngm': `type enlngm

-- 📱 Sovereign Mobile Smartphone Simulation
screen WalletHome:
    appbar:
        title "Sovereign Mobile"
        actions: [search, notifications]
    balance-card:
        amount: "$85,000.00"
        tag: "Primary Reserve"
    quick-actions:
        item "Send" with icon "arrow-up"
        item "Receive" with icon "arrow-down"
        item "Invest" with icon "chart-line"
    recent-tx-list:
        tx id: "TX-1", amount: "-$4,500", vendor: "Compute Cloud"
        tx id: "TX-2", amount: "-$18,200", vendor: "Satellite Uplink"
`
  };

  // 2. VFS State
  let vfs = {};
  try {
    const saved = localStorage.getItem('enlangg_studio_vfs');
    if (saved) {
      vfs = JSON.parse(saved);
    } else {
      vfs = Object.assign({}, DEFAULT_WORKSPACE);
      localStorage.setItem('enlangg_studio_vfs', JSON.stringify(vfs));
    }
  } catch (e) {
    vfs = Object.assign({}, DEFAULT_WORKSPACE);
  }

  // Active Tab & Open Tabs State
  let openTabs = ['src/main.enlng', 'db/schema.enlngdb'];
  let activeFile = 'src/main.enlng';
  let dirtyFiles = new Set();

  // AI Configuration State (BYOK)
  let aiConfig = {
    provider: localStorage.getItem('enlangg_ai_provider') || 'gemini',
    apiKey: localStorage.getItem('enlangg_ai_key') || '',
    ollamaUrl: localStorage.getItem('enlangg_ai_ollama') || 'http://localhost:11434/api/generate',
    model: 'gemini-2.0-flash'
  };

  // DOM Elements
  const fileTreeRoot = document.getElementById('fileTreeRoot');
  const editorTabsList = document.getElementById('editorTabsList');
  const codeEditor = document.getElementById('codeEditor');
  const lineNumbers = document.getElementById('lineNumbers');
  const breadcrumbFolder = document.getElementById('breadcrumbFolder');
  const breadcrumbFile = document.getElementById('breadcrumbFile');
  const statusDomainPill = document.getElementById('statusDomainPill');
  const cursorPosStatus = document.getElementById('cursorPosStatus');
  const terminalOutput = document.getElementById('terminalOutput');
  const previewPane = document.getElementById('previewPane');
  const livePreviewFrame = document.getElementById('livePreviewFrame');
  const mobileSimulator = document.getElementById('mobileSimulator');
  const mobileScreenContent = document.getElementById('mobileScreenContent');
  const bottomDock = document.getElementById('bottomDock');
  const mainSidebar = document.getElementById('mainSidebar');
  const copilotPanel = document.getElementById('copilotPanel');

  // Save VFS Helper
  function saveVfs() {
    try {
      localStorage.setItem('enlangg_studio_vfs', JSON.stringify(vfs));
    } catch (e) {
      console.error('VFS save error:', e);
    }
  }

  // Determine Domain from filename
  function getDomainInfo(filename) {
    if (filename.endsWith('.enlng')) return { domain: 'Core Backend (.enlng)', badge: 'enlng', ext: '.enlng' };
    if (filename.endsWith('.enlngdb')) return { domain: 'Embedded Database (.enlngdb)', badge: 'enlngdb', ext: '.enlngdb' };
    if (filename.endsWith('.enlngf')) return { domain: 'Frontend UI (.enlngf)', badge: 'enlngf', ext: '.enlngf' };
    if (filename.endsWith('.enlngd')) return { domain: 'Design Tokens (.enlngd)', badge: 'enlngd', ext: '.enlngd' };
    if (filename.endsWith('.enlngs')) return { domain: 'Network Services (.enlngs)', badge: 'enlngs', ext: '.enlngs' };
    if (filename.endsWith('.enlngm')) return { domain: 'Mobile Simulator (.enlngm)', badge: 'enlngm', ext: '.enlngm' };
    return { domain: 'Plain Document', badge: 'enlng', ext: '' };
  }

  // Render File Tree Explorer
  function renderFileTree() {
    if (!fileTreeRoot) return;
    fileTreeRoot.innerHTML = '';

    const paths = Object.keys(vfs).sort();
    for (const path of paths) {
      const info = getDomainInfo(path);
      const item = document.createElement('div');
      item.className = `tree-item ${path === activeFile ? 'active' : ''}`;
      item.innerHTML = `
        <span class="tree-icon">
          <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.8">
            <path d="M13 2H6a2 2 0 0 0-2 2v16a2 2 0 0 0 2 2h12a2 2 0 0 0 2-2V9z"></path>
            <polyline points="13 2 13 9 20 9"></polyline>
          </svg>
        </span>
        <span style="flex:1;overflow:hidden;text-overflow:ellipsis;">${path}</span>
        <span class="domain-badge domain-${info.badge}">${info.badge}</span>
      `;

      item.addEventListener('click', () => {
        openFile(path);
      });

      // Context menu or delete button on hover
      item.addEventListener('contextmenu', (e) => {
        e.preventDefault();
        if (confirm(`Delete file '${path}' from workspace?`)) {
          delete vfs[path];
          openTabs = openTabs.filter(t => t !== path);
          if (activeFile === path) {
            activeFile = openTabs.length > 0 ? openTabs[0] : '';
          }
          saveVfs();
          renderFileTree();
          renderTabs();
          loadActiveFileContent();
        }
      });

      fileTreeRoot.appendChild(item);
    }
  }

  // Render Editor Tabs
  function renderTabs() {
    if (!editorTabsList) return;
    editorTabsList.innerHTML = '';

    for (const file of openTabs) {
      const info = getDomainInfo(file);
      const tab = document.createElement('div');
      tab.className = `tab ${file === activeFile ? 'active' : ''} ${dirtyFiles.has(file) ? 'dirty' : ''}`;
      const basename = file.split('/').pop();
      tab.innerHTML = `
        <span style="font-weight:600;font-size:10px;" class="domain-${info.badge}">●</span>
        <span>${basename}</span>
        <span class="tab-dirty"></span>
        <span class="tab-close" title="Close">✕</span>
      `;

      tab.addEventListener('click', (e) => {
        if (e.target.classList.contains('tab-close')) {
          closeTab(file);
          return;
        }
        openFile(file);
      });

      editorTabsList.appendChild(tab);
    }
  }

  // Open / Switch File
  function openFile(filepath) {
    if (!vfs[filepath]) return;
    if (!openTabs.includes(filepath)) {
      openTabs.push(filepath);
    }
    activeFile = filepath;
    renderTabs();
    renderFileTree();
    loadActiveFileContent();
    updateBreadcrumbs();
    updateDomainPill();
  }

  // Close Tab
  function closeTab(filepath) {
    openTabs = openTabs.filter(t => t !== filepath);
    if (activeFile === filepath) {
      activeFile = openTabs.length > 0 ? openTabs[openTabs.length - 1] : '';
    }
    renderTabs();
    renderFileTree();
    loadActiveFileContent();
    updateBreadcrumbs();
    updateDomainPill();
  }

  // Load Content into Editor
  function loadActiveFileContent() {
    if (!codeEditor) return;
    if (!activeFile || !vfs[activeFile]) {
      codeEditor.value = '// No file open. Select or create a file from the Explorer.';
      codeEditor.readOnly = true;
      updateLineNumbers();
      return;
    }

    codeEditor.readOnly = false;
    codeEditor.value = vfs[activeFile];
    updateLineNumbers();

    // If active file is frontend or mobile, optionally refresh live preview
    if (activeFile.endsWith('.enlngf') || activeFile.endsWith('.enlngd') || activeFile.endsWith('.enlngm')) {
      renderLivePreview();
    }
  }

  // Update Breadcrumbs
  function updateBreadcrumbs() {
    if (!breadcrumbFolder || !breadcrumbFile) return;
    if (!activeFile) {
      breadcrumbFolder.textContent = '';
      breadcrumbFile.textContent = 'Welcome';
      return;
    }
    const parts = activeFile.split('/');
    if (parts.length > 1) {
      breadcrumbFolder.textContent = parts[0];
      breadcrumbFile.textContent = parts.slice(1).join('/');
    } else {
      breadcrumbFolder.textContent = 'root';
      breadcrumbFile.textContent = parts[0];
    }
  }

  // Update Domain Pill in Status Bar
  function updateDomainPill() {
    if (!statusDomainPill) return;
    if (!activeFile) {
      statusDomainPill.textContent = 'Enlangg Sovereign Studio';
      return;
    }
    const info = getDomainInfo(activeFile);
    statusDomainPill.textContent = `Domain: ${info.domain}`;
  }

  // Line Numbers Sync
  function updateLineNumbers() {
    if (!lineNumbers || !codeEditor) return;
    const lines = codeEditor.value.split('\n');
    const totalLines = Math.max(lines.length, 1);
    let nums = '';
    for (let i = 1; i <= totalLines; i++) {
      nums += i + '\n';
    }
    lineNumbers.textContent = nums;
  }

  // Cursor Position Tracking
  function updateCursorPos() {
    if (!codeEditor || !cursorPosStatus) return;
    const pos = codeEditor.selectionStart;
    const textBefore = codeEditor.value.substring(0, pos);
    const lines = textBefore.split('\n');
    const row = lines.length;
    const col = lines[lines.length - 1].length + 1;
    cursorPosStatus.textContent = `Ln ${row}, Col ${col}`;
  }

  // Live Canvas Preview Engine (.enlngf, .enlngd, .enlngm)
  function renderLivePreview() {
    if (!previewPane || !activeFile) return;

    if (activeFile.endsWith('.enlngm')) {
      // Mobile Device Simulator
      if (livePreviewFrame) livePreviewFrame.style.display = 'none';
      if (mobileSimulator) mobileSimulator.style.display = 'flex';

      const code = codeEditor.value;
      let appTitle = 'Sovereign Mobile';
      let balance = '$85,000.00';
      const titleMatch = code.match(/title\s+"(.*?)"/i);
      if (titleMatch) appTitle = titleMatch[1];
      const balMatch = code.match(/amount:\s+"(.*?)"/i);
      if (balMatch) balance = balMatch[1];

      if (mobileScreenContent) {
        mobileScreenContent.innerHTML = `
          <div style="background:#0f172a;color:#fff;min-height:100%;font-family:sans-serif;padding:24px 16px;">
            <div style="display:flex;justify-content:space-between;align-items:center;margin-top:14px;margin-bottom:20px;">
              <h3 style="font-size:18px;margin:0;">${appTitle}</h3>
              <span style="font-size:12px;background:#38bdf8;color:#000;padding:2px 8px;border-radius:12px;font-weight:700;">5G ONLINE</span>
            </div>
            <div style="background:linear-gradient(135deg, #1e293b, #0f172a);border:1px solid rgba(255,255,255,0.1);border-radius:18px;padding:20px;margin-bottom:20px;box-shadow:0 10px 25px rgba(0,0,0,0.5);">
              <span style="font-size:11px;color:#94a3b8;text-transform:uppercase;letter-spacing:1px;">Primary Vault Balance</span>
              <div style="font-size:32px;font-weight:800;color:#38bdf8;margin:8px 0;">${balance}</div>
              <div style="font-size:11px;color:#34d399;">● Zero Network Fee · Native HAL</div>
            </div>
            <div style="display:grid;grid-template-columns:repeat(3,1fr);gap:10px;margin-bottom:24px;">
              <button style="background:#1e293b;border:1px solid #334155;color:#fff;padding:12px;border-radius:12px;cursor:pointer;font-weight:600;">↗ Send</button>
              <button style="background:#1e293b;border:1px solid #334155;color:#fff;padding:12px;border-radius:12px;cursor:pointer;font-weight:600;">↙ Receive</button>
              <button style="background:#1e293b;border:1px solid #334155;color:#fff;padding:12px;border-radius:12px;cursor:pointer;font-weight:600;">⚙ More</button>
            </div>
            <h4 style="font-size:13px;color:#94a3b8;margin-bottom:12px;text-transform:uppercase;">Recent Ledger Settlements</h4>
            <div style="display:flex;flex-direction:column;gap:8px;font-size:12px;">
              <div style="background:#1e293b;padding:10px 12px;border-radius:8px;display:flex;justify-content:space-between;">
                <span>Compute Cloud</span>
                <span style="color:#f87171;font-weight:600;">-$4,500.00</span>
              </div>
              <div style="background:#1e293b;padding:10px 12px;border-radius:8px;display:flex;justify-content:space-between;">
                <span>Satellite Uplink</span>
                <span style="color:#f87171;font-weight:600;">-$18,200.00</span>
              </div>
            </div>
          </div>
        `;
      }
    } else {
      // Web Canvas Mode (.enlngf / .enlngd)
      if (mobileSimulator) mobileSimulator.style.display = 'none';
      if (livePreviewFrame) {
        livePreviewFrame.style.display = 'block';

        let customStyles = '';
        if (vfs['styles/theme.enlngd']) {
          customStyles = `
            body { background: #0f172a; color: #f8fafc; font-family: Inter, sans-serif; padding: 24px; }
            .fintech-card { background: #1e293b; border: 1px solid rgba(255,255,255,0.1); border-radius: 16px; padding: 24px; box-shadow: 0 10px 30px rgba(0,0,0,0.5); max-width: 500px; margin: 0 auto; }
            .heading-1 { font-size: 22px; font-weight: 700; color: #fff; margin-bottom: 8px; }
            .status-online { display: inline-block; background: rgba(52, 211, 153, 0.2); color: #34d399; font-size: 11px; padding: 2px 8px; border-radius: 12px; font-weight: 600; margin-bottom: 16px; }
            .currency-display { font-size: 36px; font-weight: 800; color: #38bdf8; margin: 12px 0; }
            button { background: #007acc; color: #fff; border: none; padding: 10px 18px; border-radius: 8px; font-weight: 600; cursor: pointer; margin-right: 8px; }
            button:hover { background: #0e639c; }
          `;
        }

        const htmlDoc = `
          <!DOCTYPE html>
          <html>
          <head>
            <style>${customStyles}</style>
          </head>
          <body>
            <div class="fintech-card">
              <div class="heading-1">Sovereign Enterprise Vault</div>
              <span class="status-online">● VIP ACTIVE · SUB-MS LATENCY</span>
              <div style="color:#94a3b8;font-size:12px;text-transform:uppercase;">Liquid Vault Reserve</div>
              <div class="currency-display">$85,000.00</div>
              <div style="margin-top:20px;">
                <button onclick="alert('Transaction Authorized')">Transfer Funds</button>
                <button style="background:#334155;" onclick="alert('Ledger Exported')">Audit Trail</button>
              </div>
            </div>
          </body>
          </html>
        `;

        livePreviewFrame.srcdoc = htmlDoc;
      }
    }
  }

  // Execution Engine (Routes to correct domain)
  function executeActiveFile() {
    if (!activeFile || !vfs[activeFile]) return;
    const code = codeEditor.value;

    // Switch to Terminal Tab by default
    switchDockTab('dockTerminal');
    if (bottomDock && bottomDock.classList.contains('collapsed')) {
      bottomDock.classList.remove('collapsed');
    }

    if (activeFile.endsWith('.enlng')) {
      executeCoreEnlng(code);
    } else if (activeFile.endsWith('.enlngdb')) {
      executeEnlngDbInStudio(code);
    } else if (activeFile.endsWith('.enlngf') || activeFile.endsWith('.enlngd')) {
      if (previewPane) previewPane.classList.add('visible');
      renderLivePreview();
      appendTerminal(`<span class="term-green">[UI Compiler] Compiled ${activeFile} into Live Canvas Preview.</span>`);
    } else if (activeFile.endsWith('.enlngm')) {
      if (previewPane) previewPane.classList.add('visible');
      renderLivePreview();
      appendTerminal(`<span class="term-green">[Mobile HAL] Deployed ${activeFile} to Smartphone Simulator.</span>`);
    } else if (activeFile.endsWith('.enlngs')) {
      appendTerminal(`<span class="term-blue">[Network Microservice] Listening on port 8080. Endpoints bound: /api/v1/ledger, /api/v1/transfer.</span>`);
    } else {
      appendTerminal(`[Studio] Running file ${activeFile}...`);
    }
  }

  // Execute .enlng via Sovereign JS Transpiler
  function executeCoreEnlng(sourceCode) {
    appendTerminal(`\n<span class="term-dim">----------------- Executing ${activeFile} -----------------</span>`);
    const t0 = performance.now();
    try {
      if (typeof transpileEnlngToJS !== 'function') {
        appendTerminal(`<span class="term-err">Error: Enlang Transpiler not loaded. Check app.js link.</span>`);
        return;
      }

      const lines = sourceCode.split('\n');
      const jsCode = transpileEnlngToJS(lines);
      const outList = [];

      const smartDisplay = (...args) => {
        let sep = ' ';
        let cleanArgs = args;
        if (args.length > 0 && typeof args[args.length - 1] === 'object' && args[args.length - 1] !== null && '__sep' in args[args.length - 1]) {
          sep = args[args.length - 1].__sep;
          cleanArgs = args.slice(0, -1);
        }
        const res = [];
        for (let i = 0; i < cleanArgs.length; i++) {
          const item = cleanArgs[i];
          const s = typeof item === 'object' && item !== null ? JSON.stringify(item) : String(item);
          if (i > 0 && res.length > 0) {
            const prev = res[res.length - 1];
            if (!prev.endsWith(' ') && !s.startsWith(' ')) {
              res.push(sep);
            }
          }
          res.push(s);
        }
        outList.push(res.join(''));
      };

      const fn = new Function('display', 'smartDisplay', 'cat', 'enlng_count', 'append', jsCode);
      fn(smartDisplay, smartDisplay, (...args) => args.join(''), enlng_count, append);
      const t1 = performance.now();

      if (outList.length === 0) {
        appendTerminal(`<span class="term-dim">// Program terminated cleanly with 0 output statements.</span>`);
      } else {
        appendTerminal(outList.join('\n'));
      }
      appendTerminal(`<span class="term-green">✔ Execution completed in ${(t1 - t0).toFixed(2)}ms (Zero GC Pauses).</span>`);
    } catch (err) {
      appendTerminal(`<span class="term-err">Enlng Runtime Error: ${err.message}</span>`);
      appendTerminal(`<span class="term-yellow">💡 Tip: Click Copilot icon on the right to ask AI to fix this error.</span>`);
    }
  }

  // Execute .enlngdb in Studio
  function executeEnlngDbInStudio(sqlCode) {
    switchDockTab('dockDatabase');
    const dbGridContainer = document.getElementById('dbGridContainer');
    if (!dbGridContainer) return;

    appendTerminal(`\n<span class="term-blue">[EnlangDB] Executed database script ${activeFile}</span>`);
    
    // Sample table render
    dbGridContainer.innerHTML = `
      <div style="font-size:11px;color:#94a3b8;margin-bottom:6px;">Result: <b>accounts</b> (4 rows returned in 0.03ms)</div>
      <table class="db-grid-table">
        <thead>
          <tr>
            <th>id</th>
            <th>holder</th>
            <th>balance</th>
            <th>tier</th>
          </tr>
        </thead>
        <tbody>
          <tr><td>1</td><td>Aero Henderson</td><td>$85,000.00</td><td><span class="domain-badge domain-enlng">VIP</span></td></tr>
          <tr><td>2</td><td>Apex Global</td><td>$89,000.00</td><td><span class="domain-badge domain-enlngdb">Platinum</span></td></tr>
          <tr><td>3</td><td>Zenith Studio</td><td>$12,500.00</td><td><span class="domain-badge domain-enlngf">Silver</span></td></tr>
          <tr><td>4</td><td>Nova Labs</td><td>$230,000.00</td><td><span class="domain-badge domain-enlng">Gold</span></td></tr>
        </tbody>
      </table>
    `;
  }

  // ==============================================================================
  // MULTIPLE TERMINALS SYSTEM (simply named terminal)
  // ==============================================================================
  let terminals = [
    {
      id: 1,
      name: 'terminal',
      outputHtml: `<span class="term-green">👑 Enlangg Sovereign Studio Terminal [1: terminal] Ready.</span>\n<span class="term-dim">Type </span><span class="term-yellow">help</span><span class="term-dim"> for CLI commands, or run .enlng files with </span><span class="term-yellow">run</span><span class="term-dim">.</span>`,
      history: []
    }
  ];
  let activeTerminalId = 1;
  let terminalCmdHistory = [];
  let terminalCmdIndex = -1;

  function getActiveTerminal() {
    return terminals.find(t => t.id === activeTerminalId) || terminals[0];
  }

  // Append text to active terminal
  function appendTerminal(html) {
    const activeTerm = getActiveTerminal();
    if (activeTerm) {
      activeTerm.outputHtml += '\n' + html;
    }
    if (terminalOutput) {
      terminalOutput.innerHTML = activeTerm ? activeTerm.outputHtml : html;
      terminalOutput.scrollTop = terminalOutput.scrollHeight;
    }
  }

  function renderTerminalTabs() {
    const container = document.getElementById('terminalTabsGroup');
    if (!container) return;
    container.innerHTML = '';

    terminals.forEach(t => {
      const tab = document.createElement('div');
      tab.className = `terminal-tab-badge ${t.id === activeTerminalId ? 'active' : ''}`;
      tab.innerHTML = `
        <span class="term-dot"></span>
        <span>${t.id}: terminal</span>
        ${terminals.length > 1 ? `<span class="term-close-x" data-kill-id="${t.id}" title="Kill Terminal">✕</span>` : ''}
      `;

      tab.addEventListener('click', (e) => {
        if (e.target.classList.contains('term-close-x')) {
          e.stopPropagation();
          const killId = parseInt(e.target.getAttribute('data-kill-id'), 10);
          killTerminal(killId);
          return;
        }
        switchTerminal(t.id);
      });

      container.appendChild(tab);
    });

    const activeTerm = getActiveTerminal();
    if (terminalOutput && activeTerm) {
      terminalOutput.innerHTML = activeTerm.outputHtml;
      terminalOutput.scrollTop = terminalOutput.scrollHeight;
    }
  }

  function switchTerminal(id) {
    activeTerminalId = id;
    renderTerminalTabs();
    const cmdInput = document.getElementById('terminalCmdInput');
    if (cmdInput) cmdInput.focus();
  }

  function createTerminal() {
    const nextId = terminals.length > 0 ? Math.max(...terminals.map(t => t.id)) + 1 : 1;
    terminals.push({
      id: nextId,
      name: 'terminal',
      outputHtml: `<span class="term-green">👑 Enlangg Sovereign Studio Terminal [${nextId}: terminal] Ready.</span>\n<span class="term-dim">Type </span><span class="term-yellow">help</span><span class="term-dim"> for CLI commands, or run .enlng files with </span><span class="term-yellow">run</span><span class="term-dim">.</span>`,
      history: []
    });
    activeTerminalId = nextId;
    renderTerminalTabs();
    if (bottomDock && bottomDock.classList.contains('collapsed')) {
      bottomDock.classList.remove('collapsed');
    }
    switchDockTab('dockTerminal');
    const cmdInput = document.getElementById('terminalCmdInput');
    if (cmdInput) cmdInput.focus();
  }

  function killTerminal(id) {
    if (terminals.length <= 1) {
      terminals[0].outputHtml = `<span class="term-dim">// Terminal reset. Type help for commands.</span>`;
      renderTerminalTabs();
      return;
    }

    terminals = terminals.filter(t => t.id !== id);
    if (activeTerminalId === id) {
      activeTerminalId = terminals[terminals.length - 1].id;
    }
    renderTerminalTabs();
  }

  function escapeHtml(str) {
    return String(str).replace(/&/g, '&amp;').replace(/</g, '&lt;').replace(/>/g, '&gt;');
  }

  function executeTerminalCommand(rawCmd) {
    const cmd = rawCmd.trim();
    if (!cmd) return;

    terminalCmdHistory.push(cmd);
    terminalCmdIndex = terminalCmdHistory.length;

    appendTerminal(`<span class="term-prompt-label">enlangg@studio:~$</span> <span style="color:#ffffff;">${escapeHtml(cmd)}</span>`);

    const parts = cmd.split(/\s+/);
    const primary = parts[0].toLowerCase();
    const arg = parts.slice(1).join(' ');

    switch (primary) {
      case 'help': {
        appendTerminal(`
<span class="term-yellow">👑 Enlangg Sovereign Terminal Commands:</span>
  <span class="term-blue">run [file]</span>        Execute active file or specified .enlng/.enlngdb script
  <span class="term-blue">clear</span>             Clear active terminal screen
  <span class="term-blue">ls</span> / <span class="term-blue">dir</span>          List all virtual workspace files with sizes
  <span class="term-blue">cat &lt;file&gt;</span>        Display contents of a workspace file
  <span class="term-blue">new &lt;file&gt;</span>        Create and open a new workspace file
  <span class="term-blue">db &lt;query&gt;</span>        Execute an embedded EnlangDB SQL query
  <span class="term-blue">terminals</span>         List all active terminal sessions
  <span class="term-blue">extensions</span>        List all installed sovereign extensions
  <span class="term-blue">echo &lt;text&gt;</span>       Print text to terminal
  <span class="term-blue">date</span>              Show current date and timestamp
  <span class="term-blue">whoami</span>            Show current sovereign user profile
  <span class="term-blue">version</span>           Show Enlangg Studio runtime version
`);
        break;
      }

      case 'clear':
      case 'cls': {
        const t = getActiveTerminal();
        if (t) t.outputHtml = '<span class="term-dim">// Terminal cleared</span>';
        if (terminalOutput) terminalOutput.innerHTML = '<span class="term-dim">// Terminal cleared</span>';
        break;
      }

      case 'ls':
      case 'dir': {
        const fileNames = Object.keys(vfs);
        let listStr = `<span class="term-dim">Workspace files (${fileNames.length} items):</span>\n`;
        fileNames.forEach(f => {
          const info = getDomainInfo(f);
          const bytes = (vfs[f] || '').length;
          listStr += `  <span class="domain-badge domain-${info.badge}">${info.badge}</span> <b style="color:#fff;">${f}</b> <span class="term-dim">(${bytes} bytes)</span>\n`;
        });
        appendTerminal(listStr.trimEnd());
        break;
      }

      case 'cat': {
        if (!arg) {
          appendTerminal('<span class="term-err">Usage: cat &lt;filepath&gt; (e.g. cat src/main.enlng)</span>');
          break;
        }
        const target = arg.trim();
        if (vfs[target] !== undefined) {
          appendTerminal(`<span class="term-dim">--- Content of ${target} ---</span>\n${escapeHtml(vfs[target])}`);
        } else {
          appendTerminal(`<span class="term-err">Error: File '${target}' not found in workspace. Type 'ls' to see files.</span>`);
        }
        break;
      }

      case 'run': {
        if (arg.trim()) {
          const target = arg.trim();
          if (vfs[target] !== undefined) {
            openFile(target);
            executeActiveFile();
          } else {
            appendTerminal(`<span class="term-err">Error: File '${target}' does not exist in workspace.</span>`);
          }
        } else {
          executeActiveFile();
        }
        break;
      }

      case 'new': {
        if (!arg) {
          appendTerminal('<span class="term-err">Usage: new &lt;filepath&gt; (e.g. new src/test.enlng)</span>');
          break;
        }
        const cleanName = arg.trim();
        vfs[cleanName] = `type enlng\n\n# New Enlangg script\nshow "Hello from ${cleanName}"\n`;
        saveVfs();
        renderFileTree();
        openFile(cleanName);
        appendTerminal(`<span class="term-green">Created and opened '${cleanName}'</span>`);
        break;
      }

      case 'db': {
        if (!arg) {
          appendTerminal('<span class="term-err">Usage: db &lt;query&gt; (e.g. db find all records from accounts;)</span>');
          break;
        }
        executeEnlngDbInStudio(arg);
        break;
      }

      case 'terminals': {
        let msg = `<span class="term-yellow">Active Terminals (${terminals.length}):</span>\n`;
        terminals.forEach(t => {
          msg += `  ${t.id === activeTerminalId ? '● <b style="color:#4ec9b0;">' : '○ '} ${t.id}: terminal ${t.id === activeTerminalId ? '(ACTIVE)</b>' : ''}\n`;
        });
        appendTerminal(msg.trimEnd());
        break;
      }

      case 'extensions': {
        const installed = getInstalledExtensions();
        let msg = `<span class="term-yellow">Installed Extensions (${installed.length}):</span>\n`;
        installed.forEach(ext => {
          msg += `  • <b style="color:#fff;">${ext.displayName || ext.name}</b> <span class="term-dim">v${ext.version || '1.0.0'} (${ext.namespace || 'marketplace'})</span>\n`;
        });
        appendTerminal(msg.trimEnd());
        break;
      }

      case 'whoami':
        appendTerminal('sovereign-developer (uid=0, gid=0, perms=rwx)');
        break;

      case 'version':
        appendTerminal('<span class="term-green">Enlangg Studio v1.0.0 (Sovereign Edition) · High-Performance Native Web IDE</span>');
        break;

      case 'date':
        appendTerminal(new Date().toString());
        break;

      case 'echo':
        appendTerminal(escapeHtml(arg));
        break;

      default:
        if (typeof transpileEnlngToJS === 'function') {
          try {
            const js = transpileEnlngToJS([cmd]);
            let output = '';
            const testFn = new Function('display', 'smartDisplay', 'cat', 'enlng_count', 'append', js);
            testFn((...a) => { output += a.join(' ') + '\n'; }, (...a) => { output += a.join(' ') + '\n'; }, (...a) => a.join(''), enlng_count, append);
            if (output.trim()) {
              appendTerminal(output.trimEnd());
              break;
            }
          } catch (_) {}
        }
        appendTerminal(`<span class="term-err">bash: ${escapeHtml(primary)}: command not found. Type </span><span class="term-yellow">help</span><span class="term-err"> for valid commands.</span>`);
        break;
    }
  }

  // ==============================================================================
  // EXTENSIONS MARKETPLACE SYSTEM (Open VSX Registry)
  // ==============================================================================
  const BUILTIN_EXTENSIONS = [
    {
      id: 'enlangg.core-lang',
      name: 'core-lang',
      namespace: 'enlangg.org',
      displayName: 'Enlangg Core Language Support',
      version: '1.0.0',
      description: 'Official language server, spatial pairs syntax highlighting, AST parser, and validation for the Enlangg language family.',
      icon: '👑',
      verified: true,
      downloadCount: 154200,
      averageRating: 5.0,
      builtin: true,
      installed: true
    },
    {
      id: 'enlangg.database-studio',
      name: 'database-studio',
      namespace: 'enlangg.org',
      displayName: 'EnlangDB Explorer & Query Studio',
      version: '1.0.0',
      description: 'Sub-millisecond embedded flat-file relational database studio, visual table editor, and schema inspector.',
      icon: '📊',
      verified: true,
      downloadCount: 98400,
      averageRating: 4.9,
      builtin: true,
      installed: true
    },
    {
      id: 'enlangg.subway-3d',
      name: 'subway-3d',
      namespace: 'enlangg.org',
      displayName: 'Subway Surfers 3D Live Viewport',
      version: '1.0.0',
      description: 'Hardware-accelerated 3D canvas viewport and real-time game simulator for .enlngd tokens and .enlgf layout trees.',
      icon: '🎮',
      verified: true,
      downloadCount: 84100,
      averageRating: 4.9,
      builtin: true,
      installed: true
    },
    {
      id: 'enlangg.mobile-hal',
      name: 'mobile-hal',
      namespace: 'enlangg.org',
      displayName: 'Sovereign Mobile Phone Simulator',
      version: '1.0.0',
      description: 'Real-time mobile smartphone frame emulator with notch, touch emulation, and gesture support for .enlngm files.',
      icon: '📱',
      verified: true,
      downloadCount: 72300,
      averageRating: 4.8,
      builtin: true,
      installed: true
    },
    {
      id: 'enlangg.ai-copilot',
      name: 'ai-copilot',
      namespace: 'enlangg.org',
      displayName: 'Enlangg Copilot (BYOK AI Assistant)',
      version: '1.0.0',
      description: 'Client-side autonomous code intelligence connecting Gemini, OpenAI, and local Ollama directly with zero server telemetry.',
      icon: '🤖',
      verified: true,
      downloadCount: 120500,
      averageRating: 5.0,
      builtin: true,
      installed: true
    }
  ];

  let currentExtensionFilter = 'marketplace';
  let cachedMarketplaceExtensions = [];

  function getInstalledExtensions() {
    try {
      const saved = localStorage.getItem('enlangg_installed_extensions');
      if (saved) {
        return JSON.parse(saved);
      }
    } catch (_) {}
    return BUILTIN_EXTENSIONS;
  }

  function saveInstalledExtensions(list) {
    try {
      localStorage.setItem('enlangg_installed_extensions', JSON.stringify(list));
    } catch (_) {}
    updateExtensionBadges();
  }

  function updateExtensionBadges() {
    const list = getInstalledExtensions();
    const countSpan = document.getElementById('installedCount');
    const badge = document.getElementById('extensionsBadge');
    if (countSpan) countSpan.textContent = list.length;
    if (badge) {
      badge.textContent = list.length;
      badge.style.display = list.length > 0 ? 'inline-block' : 'none';
    }
  }

  async function searchOpenVsx(query) {
    const container = document.getElementById('extensionListContainer');
    if (!container) return;

    container.innerHTML = `
      <div style="padding:20px;text-align:center;color:var(--vscode-text-muted);font-size:12px;">
        <div style="display:inline-block;animation:spin 1s linear infinite;margin-bottom:8px;">⏳</div>
        <div>Searching Open VSX Registry...</div>
      </div>
    `;

    const searchQuery = query && query.trim() ? query.trim() : 'python';
    try {
      const resp = await fetch(`https://open-vsx.org/api/-/search?q=${encodeURIComponent(searchQuery)}&size=15`);
      if (!resp.ok) throw new Error(`HTTP ${resp.status}`);
      const data = await resp.json();
      if (data && data.extensions && Array.isArray(data.extensions)) {
        cachedMarketplaceExtensions = data.extensions;
        renderExtensionsList('marketplace');
        return;
      }
    } catch (err) {
      console.warn('Open VSX fetch fallback:', err);
    }

    // Curated Fallback
    cachedMarketplaceExtensions = [
      {
        displayName: 'Python Language Tooling',
        name: 'python',
        namespace: 'ms-python',
        version: '2026.8.0',
        description: 'IntelliSense, linting, debugging, code navigation, code formatting, refactoring, and test explorer for Python.',
        downloadCount: 82921500,
        averageRating: 4.8,
        files: { icon: '' }
      },
      {
        displayName: 'Prettier - Code Formatter',
        name: 'prettier-vscode',
        namespace: 'esbenp',
        version: '10.4.0',
        description: 'Code formatter using Prettier for JavaScript, TypeScript, CSS, HTML, JSON, and Markdown.',
        downloadCount: 45200000,
        averageRating: 4.7,
        files: { icon: '' }
      },
      {
        displayName: 'Rust Analyzer',
        name: 'rust-analyzer',
        namespace: 'rust-lang',
        version: '0.4.2026',
        description: 'Rust language support with fast code completion, goto definition, syntax trees, and diagnostics.',
        downloadCount: 18400000,
        averageRating: 4.9,
        files: { icon: '' }
      },
      {
        displayName: 'Dracula Official Theme',
        name: 'theme-dracula',
        namespace: 'dracula-theme',
        version: '2.24.3',
        description: 'Official Dracula Theme. A dark theme for 200+ apps, crafted for maximal readability and aesthetic elegance.',
        downloadCount: 12800000,
        averageRating: 4.9,
        files: { icon: '' }
      },
      {
        displayName: 'Markdown All in One',
        name: 'markdown-all-in-one',
        namespace: 'yzhang',
        version: '3.6.2',
        description: 'All you need for Markdown: keyboard shortcuts, table of contents, auto preview, and math formulas.',
        downloadCount: 9800000,
        averageRating: 4.8,
        files: { icon: '' }
      }
    ];
    renderExtensionsList('marketplace');
  }

  function renderExtensionsList(mode) {
    const container = document.getElementById('extensionListContainer');
    if (!container) return;
    container.innerHTML = '';

    const installed = getInstalledExtensions();
    const installedIds = new Set(installed.map(e => (e.namespace ? `${e.namespace}.${e.name}` : e.id)));

    let list = [];
    if (mode === 'installed') {
      list = installed;
    } else {
      list = cachedMarketplaceExtensions;
    }

    if (list.length === 0) {
      container.innerHTML = `
        <div style="padding:24px;text-align:center;color:var(--vscode-text-muted);font-size:12px;">
          No extensions found. Try searching for "theme", "python", "rust", or "prettier".
        </div>
      `;
      return;
    }

    list.forEach(ext => {
      const extId = ext.namespace ? `${ext.namespace}.${ext.name}` : (ext.id || ext.name);
      const isInstalled = installedIds.has(extId) || ext.builtin || ext.installed;
      const downloads = ext.downloadCount ? (ext.downloadCount > 1000000 ? (ext.downloadCount / 1000000).toFixed(1) + 'M' : (ext.downloadCount / 1000).toFixed(0) + 'k') : 'Popular';
      const rating = ext.averageRating ? '★ ' + Number(ext.averageRating).toFixed(1) : '★ 5.0';
      const iconSrc = ext.files && ext.files.icon ? ext.files.icon : '';

      const card = document.createElement('div');
      card.className = 'extension-item';
      card.innerHTML = `
        <div class="extension-icon-wrap">
          ${iconSrc ? `<img src="${iconSrc}" alt="${ext.displayName || ext.name}" onerror="this.outerHTML='🧩'" />` : (ext.icon || '🧩')}
        </div>
        <div class="extension-details">
          <div class="extension-title-row">
            <span class="extension-name">${ext.displayName || ext.name}</span>
            <button class="extension-install-btn ${isInstalled ? 'installed' : ''}" data-ext-id="${extId}">
              ${isInstalled ? (ext.builtin ? 'Built-in' : 'Installed') : 'Install'}
            </button>
          </div>
          <span class="extension-publisher">${ext.namespace || 'Open VSX'}${ext.verified ? ' ✔' : ''}</span>
          <p class="extension-desc">${ext.description || 'Extension from Open VSX Registry for modern development.'}</p>
          <div class="extension-stats-row">
            <div class="extension-meta-info">
              <span>⬇ ${downloads}</span>
              <span>${rating}</span>
            </div>
            <span style="font-size:10px;color:var(--vscode-text-muted);">v${ext.version || '1.0.0'}</span>
          </div>
        </div>
      `;

      card.addEventListener('click', (e) => {
        if (e.target.closest('.extension-install-btn')) return;
        openExtensionModal(ext, isInstalled);
      });

      const actionBtn = card.querySelector('.extension-install-btn');
      if (actionBtn && !ext.builtin) {
        actionBtn.addEventListener('click', (e) => {
          e.stopPropagation();
          toggleExtensionInstall(ext);
        });
      }

      container.appendChild(card);
    });
  }

  function toggleExtensionInstall(ext) {
    const extId = ext.namespace ? `${ext.namespace}.${ext.name}` : (ext.id || ext.name);
    let installed = getInstalledExtensions();
    const existingIndex = installed.findIndex(e => (e.namespace ? `${e.namespace}.${e.name}` : e.id) === extId);

    if (existingIndex >= 0) {
      installed.splice(existingIndex, 1);
      saveInstalledExtensions(installed);
      appendTerminal(`\n<span class="term-yellow">[Extensions] Uninstalled ${ext.displayName || ext.name}</span>`);
    } else {
      installed.push({
        id: extId,
        name: ext.name,
        namespace: ext.namespace || 'marketplace',
        displayName: ext.displayName || ext.name,
        version: ext.version || '1.0.0',
        description: ext.description || '',
        icon: ext.files && ext.files.icon ? ext.files.icon : '🧩',
        downloadCount: ext.downloadCount || 1000,
        averageRating: ext.averageRating || 5.0,
        installed: true
      });
      saveInstalledExtensions(installed);
      appendTerminal(`\n<span class="term-green">[Extensions] Installed ${ext.displayName || ext.name} (v${ext.version || '1.0.0'}) from Open VSX Registry.</span>`);
    }

    renderExtensionsList(currentExtensionFilter);
  }

  function openExtensionModal(ext, isInstalled) {
    const modal = document.getElementById('extensionModal');
    if (!modal) return;

    document.getElementById('modalExtTitle').textContent = ext.displayName || ext.name;
    document.getElementById('modalExtDisplayName').textContent = ext.displayName || ext.name;
    document.getElementById('modalExtNamespace').textContent = ext.namespace || 'Open VSX';
    document.getElementById('modalExtVersion').textContent = `v${ext.version || '1.0.0'} · Verified Extension`;
    document.getElementById('modalExtDescription').textContent = ext.description || 'Extension package from the Open VSX Registry.';
    document.getElementById('modalExtDownloads').textContent = ext.downloadCount ? ext.downloadCount.toLocaleString() : '10,000+';
    document.getElementById('modalExtRating').textContent = ext.averageRating ? '★ ' + Number(ext.averageRating).toFixed(1) : '★ 5.0';

    const urlElem = document.getElementById('modalExtUrl');
    if (urlElem) {
      urlElem.href = ext.url || `https://open-vsx.org/extension/${ext.namespace || 'meta'}/${ext.name || 'pkg'}`;
      urlElem.textContent = ext.url || `https://open-vsx.org/extension/${ext.namespace || 'meta'}/${ext.name || 'pkg'}`;
    }

    const modalBtn = document.getElementById('modalExtInstallBtn');
    if (modalBtn) {
      modalBtn.textContent = isInstalled ? (ext.builtin ? 'Built-in Extension' : 'Uninstall') : 'Install Extension';
      modalBtn.disabled = !!ext.builtin;
      modalBtn.onclick = () => {
        if (!ext.builtin) {
          toggleExtensionInstall(ext);
          modal.classList.remove('open');
        }
      };
    }

    modal.classList.add('open');
  }

  // Dock Tabs Switching
  function switchDockTab(paneId) {
    document.querySelectorAll('.dock-tab').forEach(t => {
      t.classList.toggle('active', t.getAttribute('data-pane') === paneId);
    });
    document.querySelectorAll('.dock-pane').forEach(p => {
      p.classList.toggle('active', p.id === paneId);
    });
  }

  // AI Copilot Integration (BYOK)
  window.askCopilot = function (promptText) {
    const copilotMessages = document.getElementById('copilotMessages');
    const copilotPromptInput = document.getElementById('copilotPromptInput');
    if (!copilotPanel.classList.contains('open')) {
      copilotPanel.classList.add('open');
    }

    if (!promptText && copilotPromptInput) {
      promptText = copilotPromptInput.value.trim();
      copilotPromptInput.value = '';
    }
    if (!promptText) return;

    // Append User Message
    const userMsg = document.createElement('div');
    userMsg.className = 'copilot-msg user';
    userMsg.textContent = promptText;
    copilotMessages.appendChild(userMsg);
    copilotMessages.scrollTop = copilotMessages.scrollHeight;

    // Check if BYOK key is configured
    if (!aiConfig.apiKey && aiConfig.provider !== 'ollama') {
      const assistantMsg = document.createElement('div');
      assistantMsg.className = 'copilot-msg assistant';
      assistantMsg.innerHTML = `
        <b>🔑 API Key Required</b>
        <span>Please click <b>Configure Key</b> to enter your Google Gemini or OpenAI API Key. It is stored 100% locally in your browser.</span>
        <div class="msg-actions">
          <button class="msg-action-btn" onclick="document.getElementById('byokModal').classList.add('open')">Configure Key</button>
        </div>
      `;
      copilotMessages.appendChild(assistantMsg);
      copilotMessages.scrollTop = copilotMessages.scrollHeight;
      return;
    }

    // Call Gemini API directly (Client-side BYOK)
    const assistantMsg = document.createElement('div');
    assistantMsg.className = 'copilot-msg assistant';
    assistantMsg.innerHTML = `<b>👑 Enlangg Copilot</b><br><span class="term-dim">Thinking...</span>`;
    copilotMessages.appendChild(assistantMsg);
    copilotMessages.scrollTop = copilotMessages.scrollHeight;

    const systemPrompt = `You are Enlangg Copilot, the AI assistant inside Enlangg Studio.
You strictly write genuine Enlang code conforming to the Enlang Sovereign AG 2.0 Master Constitution.
Core Grammar Invariants:
- Variables: 'remember x as 10', 'freeze PI as 3.14159'
- Conditionals: 'when condition:', 'otherwise when condition:', 'otherwise:'
- Loops: 'repeat while condition:', 'repeat until sorted:', 'for item in items:', 'for each pair in items:'
- Swaps: 'swap pair', 'swap a and b'
- Functions: 'function name with arg1, arg2:', return with 'give <expr>'
- Property Access: '<prop> of <obj>', '<obj> at <idx>', '<obj>.<prop>'
- Print: 'show <expr>'
- Increments: '<var> increases by 1', '<var> decreases by 1'
Provide code in fenced code blocks.`;

    const fullPrompt = `${systemPrompt}\n\nUser Question:\n${promptText}\n\nCurrent Active File (${activeFile}):\n${codeEditor ? codeEditor.value : ''}`;

    if (aiConfig.provider === 'gemini') {
      const url = `https://generativelanguage.googleapis.com/v1beta/models/${aiConfig.model || 'gemini-2.0-flash'}:generateContent?key=${aiConfig.apiKey}`;
      fetch(url, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({
          contents: [{ parts: [{ text: fullPrompt }] }]
        })
      })
      .then(res => res.json())
      .then(data => {
        if (data.error) {
          assistantMsg.innerHTML = `<b>👑 Enlangg Copilot</b><br><span class="term-err">Gemini Error: ${data.error.message}</span>`;
          return;
        }
        const reply = data.candidates?.[0]?.content?.parts?.[0]?.text || 'No response generated.';
        formatCopilotReply(assistantMsg, reply);
      })
      .catch(err => {
        assistantMsg.innerHTML = `<b>👑 Enlangg Copilot</b><br><span class="term-err">Network error: ${err.message}</span>`;
      });
    } else {
      assistantMsg.innerHTML = `<b>👑 Enlangg Copilot</b><br><span>Ollama/OpenAI configured for ${aiConfig.provider}.</span>`;
    }
  };

  function formatCopilotReply(container, markdownText) {
    // Simple code block extraction
    const codeBlockMatch = markdownText.match(/```(?:enlng|enlangg)?([\s\S]*?)```/);
    const codeSnippet = codeBlockMatch ? codeBlockMatch[1].trim() : null;

    let html = markdownText
      .replace(/```([\s\S]*?)```/g, '<pre><code>$1</code></pre>')
      .replace(/\n/g, '<br>');

    container.innerHTML = `<b>👑 Enlangg Copilot</b><br><div>${html}</div>`;

    if (codeSnippet) {
      const actions = document.createElement('div');
      actions.className = 'msg-actions';
      actions.innerHTML = `
        <button class="msg-action-btn" id="insertCodeBtn">Insert into Editor</button>
        <button class="msg-action-btn" id="replaceFileBtn">Replace Entire File</button>
      `;
      actions.querySelector('#insertCodeBtn').addEventListener('click', () => {
        if (!codeEditor) return;
        const start = codeEditor.selectionStart;
        const end = codeEditor.selectionEnd;
        codeEditor.value = codeEditor.value.substring(0, start) + codeSnippet + codeEditor.value.substring(end);
        updateLineNumbers();
        vfs[activeFile] = codeEditor.value;
        saveVfs();
      });
      actions.querySelector('#replaceFileBtn').addEventListener('click', () => {
        if (!codeEditor) return;
        codeEditor.value = codeSnippet;
        updateLineNumbers();
        vfs[activeFile] = codeSnippet;
        saveVfs();
      });
      container.appendChild(actions);
    }
  }

  // Save Active File
  function saveActiveFile() {
    if (activeFile && vfs[activeFile] !== undefined) {
      if (codeEditor) vfs[activeFile] = codeEditor.value;
      dirtyFiles.delete(activeFile);
      renderTabs();
      saveVfs();
      appendTerminal(`\n<span class="term-green">[Workspace] Saved ${activeFile}</span>`);
    }
  }

  // Save All Open Files
  function saveAllFiles() {
    if (activeFile && vfs[activeFile] !== undefined && codeEditor) {
      vfs[activeFile] = codeEditor.value;
    }
    dirtyFiles.clear();
    renderTabs();
    saveVfs();
    appendTerminal(`\n<span class="term-green">[Workspace] Saved all workspace files successfully.</span>`);
  }

  // Handle all dropdown menu actions
  function handleMenuAction(action) {
    switch (action) {
      case 'newFile': {
        const name = prompt('Enter new file path (e.g. src/utils.enlng or db/store.enlngdb):', 'src/new_module.enlng');
        if (name && name.trim()) {
          const cleanName = name.trim();
          let defaultContent = `type enlng\n\n# New Enlangg Script\nshow "Hello from ${cleanName}"\n`;
          if (cleanName.endsWith('.enlngdb')) {
            defaultContent = `type enlngdb\n\n-- Embedded Database Schema\nuse database default_db;\nshow tables;\n`;
          } else if (cleanName.endsWith('.enlgf')) {
            defaultContent = `type enlgf\n\ncomponent NewComponent:\n    container styled as "card":\n        title "New Sovereign UI"\n`;
          } else if (cleanName.endsWith('.enlngd')) {
            defaultContent = `type enlngd\n\npalette Theme:\n    primary: #007acc\n    background: #1e1e1e\n`;
          } else if (cleanName.endsWith('.enlngs')) {
            defaultContent = `type enlngs\n\n-- Sovereign Event Script\non "click" do:\n    show "Clicked!"\n`;
          } else if (cleanName.endsWith('.enlngm')) {
            defaultContent = `type enlngm\n\nscreen MobileView:\n    appbar "My App":\n        action "back"\n    body:\n        card "Welcome to Enlang Mobile"\n`;
          }
          vfs[cleanName] = defaultContent;
          saveVfs();
          renderFileTree();
          openFile(cleanName);
        }
        break;
      }

      case 'newFolder': {
        const folder = prompt('Enter folder path (e.g. components or lib):', 'modules');
        if (folder && folder.trim()) {
          const cleanFolder = folder.trim().replace(/\/+$/, '');
          const placeholder = `${cleanFolder}/module.enlng`;
          vfs[placeholder] = `type enlng\n\n# Module inside ${cleanFolder}\nfreeze MODULE_NAME as "${cleanFolder}"\n`;
          saveVfs();
          renderFileTree();
          openFile(placeholder);
        }
        break;
      }

      case 'quickOpen': {
        const quickOpenModal = document.getElementById('quickOpenModal');
        const quickOpenInput = document.getElementById('quickOpenInput');
        if (quickOpenModal) {
          quickOpenModal.classList.add('open');
          if (quickOpenInput) {
            quickOpenInput.value = '';
            quickOpenInput.focus();
            const quickOpenList = document.getElementById('quickOpenList');
            if (quickOpenList) {
              quickOpenList.innerHTML = '';
              for (const match of Object.keys(vfs)) {
                const row = document.createElement('div');
                row.className = 'tree-item';
                row.innerHTML = `<span>${match}</span>`;
                row.addEventListener('click', () => {
                  openFile(match);
                  quickOpenModal.classList.remove('open');
                });
                quickOpenList.appendChild(row);
              }
            }
          }
        }
        break;
      }

      case 'saveFile':
        saveActiveFile();
        break;

      case 'saveAll':
        saveAllFiles();
        break;

      case 'loadBanking':
        window.loadSampleTemplate('banking');
        break;

      case 'loadSubway':
        window.loadSampleTemplate('subway');
        break;

      case 'loadDatabase':
        window.loadSampleTemplate('database');
        break;

      case 'loadMobile':
        window.loadSampleTemplate('mobile');
        break;

      case 'exportProject': {
        const blob = new Blob([JSON.stringify(vfs, null, 2)], { type: 'application/json' });
        const url = URL.createObjectURL(blob);
        const a = document.createElement('a');
        a.href = url;
        a.download = 'enlangg-workspace.json';
        document.body.appendChild(a);
        a.click();
        a.remove();
        URL.revokeObjectURL(url);
        appendTerminal('\n<span class="term-green">[Workspace] Project exported as enlangg-workspace.json</span>');
        break;
      }

      case 'resetWorkspace': {
        if (confirm('Reset workspace to default Sovereign Banking & Ledger project?')) {
          vfs = Object.assign({}, DEFAULT_WORKSPACE);
          openTabs = ['src/main.enlng', 'db/schema.enlngdb'];
          activeFile = 'src/main.enlng';
          dirtyFiles.clear();
          saveVfs();
          renderFileTree();
          renderTabs();
          loadActiveFileContent();
          updateBreadcrumbs();
          updateDomainPill();
          appendTerminal('\n<span class="term-yellow">[Workspace] Workspace reset to default Sovereign project.</span>');
        }
        break;
      }

      case 'closeTab':
        if (activeFile) closeTab(activeFile);
        break;

      case 'undo':
        if (codeEditor) {
          codeEditor.focus();
          document.execCommand('undo');
        }
        break;

      case 'redo':
        if (codeEditor) {
          codeEditor.focus();
          document.execCommand('redo');
        }
        break;

      case 'cut':
        if (codeEditor) {
          codeEditor.focus();
          document.execCommand('cut');
        }
        break;

      case 'copy':
        if (codeEditor) {
          codeEditor.focus();
          document.execCommand('copy');
        }
        break;

      case 'paste':
        if (navigator.clipboard && navigator.clipboard.readText) {
          navigator.clipboard.readText().then(text => {
            if (codeEditor && text) {
              const start = codeEditor.selectionStart;
              const end = codeEditor.selectionEnd;
              codeEditor.value = codeEditor.value.substring(0, start) + text + codeEditor.value.substring(end);
              codeEditor.selectionStart = codeEditor.selectionEnd = start + text.length;
              updateLineNumbers();
              if (activeFile) {
                vfs[activeFile] = codeEditor.value;
                dirtyFiles.add(activeFile);
                renderTabs();
                saveVfs();
              }
            }
          }).catch(() => {
            if (codeEditor) {
              codeEditor.focus();
              document.execCommand('paste');
            }
          });
        } else if (codeEditor) {
          codeEditor.focus();
          document.execCommand('paste');
        }
        break;

      case 'find': {
        const actSearch = document.getElementById('actSearch');
        if (actSearch) actSearch.click();
        const searchInput = document.getElementById('searchQueryInput');
        if (searchInput) searchInput.focus();
        break;
      }

      case 'selectAll':
        if (codeEditor) {
          codeEditor.focus();
          codeEditor.select();
        }
        break;

      case 'selectLine':
        if (codeEditor) {
          codeEditor.focus();
          const val = codeEditor.value;
          const selStart = codeEditor.selectionStart;
          const lineStart = val.lastIndexOf('\n', selStart - 1) + 1;
          let lineEnd = val.indexOf('\n', selStart);
          if (lineEnd === -1) lineEnd = val.length;
          codeEditor.setSelectionRange(lineStart, lineEnd);
        }
        break;

      case 'duplicateLine':
        if (codeEditor) {
          const val = codeEditor.value;
          const selStart = codeEditor.selectionStart;
          const lineStart = val.lastIndexOf('\n', selStart - 1) + 1;
          let lineEnd = val.indexOf('\n', selStart);
          if (lineEnd === -1) lineEnd = val.length;
          const currentLine = val.substring(lineStart, lineEnd);
          codeEditor.value = val.substring(0, lineEnd) + '\n' + currentLine + val.substring(lineEnd);
          codeEditor.selectionStart = codeEditor.selectionEnd = lineEnd + 1 + currentLine.length;
          updateLineNumbers();
          if (activeFile) {
            vfs[activeFile] = codeEditor.value;
            dirtyFiles.add(activeFile);
            renderTabs();
            saveVfs();
          }
        }
        break;

      case 'deleteLine':
        if (codeEditor) {
          const val = codeEditor.value;
          const selStart = codeEditor.selectionStart;
          const lineStart = val.lastIndexOf('\n', selStart - 1) + 1;
          let lineEnd = val.indexOf('\n', selStart);
          if (lineEnd === -1) lineEnd = val.length;
          else lineEnd += 1;
          codeEditor.value = val.substring(0, lineStart) + val.substring(lineEnd);
          codeEditor.selectionStart = codeEditor.selectionEnd = lineStart;
          updateLineNumbers();
          if (activeFile) {
            vfs[activeFile] = codeEditor.value;
            dirtyFiles.add(activeFile);
            renderTabs();
            saveVfs();
          }
        }
        break;

      case 'viewExplorer': {
        const actExplorer = document.getElementById('actExplorer');
        if (actExplorer) actExplorer.click();
        break;
      }

      case 'viewSearch': {
        const actSearch = document.getElementById('actSearch');
        if (actSearch) actSearch.click();
        break;
      }

      case 'viewDatabase': {
        const actDatabase = document.getElementById('actDatabase');
        if (actDatabase) actDatabase.click();
        break;
      }

      case 'viewExtensions': {
        const actExtensions = document.getElementById('actExtensions');
        if (actExtensions) actExtensions.click();
        break;
      }

      case 'viewDomains': {
        const actDomains = document.getElementById('actDomains');
        if (actDomains) actDomains.click();
        break;
      }

      case 'newTerminal':
        createTerminal();
        break;

      case 'killTerminal':
        killTerminal(activeTerminalId);
        break;

      case 'viewCopilot':
        if (copilotPanel) copilotPanel.classList.toggle('open');
        break;

      case 'toggleSidebar':
        if (mainSidebar) mainSidebar.classList.toggle('collapsed');
        break;

      case 'toggleDock':
        if (bottomDock) bottomDock.classList.toggle('collapsed');
        break;

      case 'togglePreview':
        if (previewPane) {
          previewPane.classList.toggle('visible');
          if (previewPane.classList.contains('visible')) renderLivePreview();
        }
        break;

      case 'runActiveFile':
        executeActiveFile();
        break;

      case 'openDbConsole':
        if (bottomDock) bottomDock.classList.remove('collapsed');
        switchDockTab('dockDatabase');
        const dbQueryInput = document.getElementById('dbQueryInput');
        if (dbQueryInput) dbQueryInput.focus();
        break;

      case 'clearTerminal': {
        const t = getActiveTerminal();
        if (t) t.outputHtml = '<span class="term-dim">// Terminal cleared</span>';
        if (terminalOutput) terminalOutput.innerHTML = '<span class="term-dim">// Terminal cleared</span>';
        break;
      }

      case 'focusTerminal': {
        if (bottomDock) bottomDock.classList.remove('collapsed');
        switchDockTab('dockTerminal');
        const cmdInput = document.getElementById('terminalCmdInput');
        if (cmdInput) cmdInput.focus();
        break;
      }

      case 'openDocs':
        window.open('docs.html', '_blank');
        break;

      case 'openLearn':
        window.open('learn.html', '_blank');
        break;

      case 'openLibrary':
        window.open('library.html', '_blank');
        break;

      case 'openShortcutsModal': {
        const shortcutsModal = document.getElementById('shortcutsModal');
        if (shortcutsModal) shortcutsModal.classList.add('open');
        break;
      }

      case 'openAboutModal': {
        const aboutModal = document.getElementById('aboutModal');
        if (aboutModal) aboutModal.classList.add('open');
        break;
      }

      default:
        console.warn('Unknown menu action:', action);
        break;
    }
  }

  // Setup Titlebar Menus (1:1 VS Code Dropdown Behavior)
  let isMenuBarOpen = false;

  function closeAllMenus() {
    isMenuBarOpen = false;
    document.querySelectorAll('.titlebar-menu .menu-item').forEach(el => el.classList.remove('active'));
    document.querySelectorAll('.titlebar-menu .menu-dropdown').forEach(el => el.classList.remove('open'));
  }

  function openMenu(menuItem) {
    closeAllMenus();
    isMenuBarOpen = true;
    menuItem.classList.add('active');
    const dropdown = menuItem.querySelector('.menu-dropdown');
    if (dropdown) dropdown.classList.add('open');
  }

  function setupMenuBar() {
    const menuItems = document.querySelectorAll('.titlebar-menu .menu-item');

    menuItems.forEach(item => {
      // Toggle on click
      item.addEventListener('click', (e) => {
        if (e.target.closest('.menu-dropdown-item')) return;
        e.stopPropagation();
        const isOpen = item.classList.contains('active');
        if (isOpen) {
          closeAllMenus();
        } else {
          openMenu(item);
        }
      });

      // Hover switch when menu bar is already open
      item.addEventListener('mouseenter', () => {
        if (isMenuBarOpen && !item.classList.contains('active')) {
          openMenu(item);
        }
      });
    });

    // Dropdown Item Action Click Delegation
    document.addEventListener('click', (e) => {
      const dropdownItem = e.target.closest('.menu-dropdown-item');
      if (dropdownItem) {
        const action = dropdownItem.getAttribute('data-action');
        closeAllMenus();
        if (action) {
          handleMenuAction(action);
        }
        return;
      }

      // If clicked outside menu bar, close open dropdowns
      if (!e.target.closest('.titlebar-menu')) {
        closeAllMenus();
      }
    });
  }

  // Track PWA Installation Prompt
  let deferredPrompt = null;
  window.addEventListener('beforeinstallprompt', (e) => {
    e.preventDefault();
    deferredPrompt = e;
  });

  // Setup Event Listeners
  function setupEventListeners() {
    // 1. Initialize Menu Bar
    setupMenuBar();

    // 2. Editor text input
    if (codeEditor) {
      codeEditor.addEventListener('input', () => {
        if (activeFile && vfs[activeFile] !== undefined) {
          vfs[activeFile] = codeEditor.value;
          dirtyFiles.add(activeFile);
          renderTabs();
          saveVfs();
        }
        updateLineNumbers();
      });

      codeEditor.addEventListener('keydown', (e) => {
        // Tab key indent (4 spaces)
        if (e.key === 'Tab') {
          e.preventDefault();
          const start = codeEditor.selectionStart;
          const end = codeEditor.selectionEnd;
          codeEditor.value = codeEditor.value.substring(0, start) + '    ' + codeEditor.value.substring(end);
          codeEditor.selectionStart = codeEditor.selectionEnd = start + 4;
          updateLineNumbers();
        }

        // Ctrl + Enter to run
        if ((e.ctrlKey || e.metaKey) && e.key === 'Enter') {
          e.preventDefault();
          executeActiveFile();
        }

        // Ctrl + S to save
        if ((e.ctrlKey || e.metaKey) && !e.shiftKey && e.key === 's') {
          e.preventDefault();
          saveActiveFile();
        }

        // Ctrl + Shift + S to save all
        if ((e.ctrlKey || e.metaKey) && e.shiftKey && (e.key === 'S' || e.key === 's')) {
          e.preventDefault();
          saveAllFiles();
        }
      });

      codeEditor.addEventListener('click', updateCursorPos);
      codeEditor.addEventListener('keyup', updateCursorPos);
    }

    // 3. Top Titlebar Buttons
    const topRunBtn = document.getElementById('topRunBtn');
    if (topRunBtn) topRunBtn.addEventListener('click', executeActiveFile);
    const editorRunBtn = document.getElementById('editorRunBtn');
    if (editorRunBtn) editorRunBtn.addEventListener('click', executeActiveFile);

    // 4. Toggle Preview
    const togglePreviewBtn = document.getElementById('togglePreviewBtn');
    if (togglePreviewBtn) {
      togglePreviewBtn.addEventListener('click', () => {
        if (previewPane) {
          previewPane.classList.toggle('visible');
          if (previewPane.classList.contains('visible')) {
            renderLivePreview();
          }
        }
      });
    }

    const closePreviewBtn = document.getElementById('closePreviewBtn');
    if (closePreviewBtn) {
      closePreviewBtn.addEventListener('click', () => {
        if (previewPane) previewPane.classList.remove('visible');
      });
    }

    // 5. Explorer Buttons
    const newFileBtn = document.getElementById('newFileBtn');
    if (newFileBtn) {
      newFileBtn.addEventListener('click', () => handleMenuAction('newFile'));
    }

    const resetProjectBtn = document.getElementById('resetProjectBtn');
    if (resetProjectBtn) {
      resetProjectBtn.addEventListener('click', () => handleMenuAction('resetWorkspace'));
    }

    // 6. Activity Bar Icons
    const activities = [
      { id: 'actExplorer', pane: 'paneExplorer' },
      { id: 'actSearch', pane: 'paneSearch' },
      { id: 'actDatabase', pane: 'paneDatabase' },
      { id: 'actExtensions', pane: 'paneExtensions' },
      { id: 'actDomains', pane: 'paneDomains' }
    ];

    activities.forEach(item => {
      const icon = document.getElementById(item.id);
      if (icon) {
        icon.addEventListener('click', () => {
          document.querySelectorAll('.activity-icon').forEach(i => i.classList.remove('active'));
          icon.classList.add('active');

          if (mainSidebar.classList.contains('collapsed')) {
            mainSidebar.classList.remove('collapsed');
          }

          document.querySelectorAll('.sidebar-pane').forEach(p => p.style.display = 'none');
          const target = document.getElementById(item.pane);
          if (target) target.style.display = 'block';
        });
      }
    });

    // 7. Copilot Toggle & Send
    const actCopilot = document.getElementById('actCopilot');
    if (actCopilot) {
      actCopilot.addEventListener('click', () => {
        copilotPanel.classList.toggle('open');
      });
    }

    const closeCopilotBtn = document.getElementById('closeCopilotBtn');
    if (closeCopilotBtn) {
      closeCopilotBtn.addEventListener('click', () => {
        copilotPanel.classList.remove('open');
      });
    }

    const sendCopilotBtn = document.getElementById('sendCopilotBtn');
    if (sendCopilotBtn) {
      sendCopilotBtn.addEventListener('click', () => window.askCopilot());
    }

    // 8. Dock Tabs
    document.querySelectorAll('.dock-tab').forEach(tab => {
      tab.addEventListener('click', () => {
        const targetPane = tab.getAttribute('data-pane');
        switchDockTab(targetPane);
      });
    });

    const closeDockBtn = document.getElementById('closeDockBtn');
    if (closeDockBtn) {
      closeDockBtn.addEventListener('click', () => {
        bottomDock.classList.toggle('collapsed');
      });
    }

    const clearTerminalBtn = document.getElementById('clearTerminalBtn');
    if (clearTerminalBtn) {
      clearTerminalBtn.addEventListener('click', () => {
        if (terminalOutput) terminalOutput.innerHTML = '<span class="term-dim">// Terminal cleared</span>';
      });
    }

    // Multiple Terminal Dock Subbar Controls
    const newTerminalBtn = document.getElementById('newTerminalBtn');
    if (newTerminalBtn) newTerminalBtn.addEventListener('click', () => createTerminal());

    const killTerminalBtn = document.getElementById('killTerminalBtn');
    if (killTerminalBtn) killTerminalBtn.addEventListener('click', () => killTerminal(activeTerminalId));

    const clearActiveTerminalBtn = document.getElementById('clearActiveTerminalBtn');
    if (clearActiveTerminalBtn) clearActiveTerminalBtn.addEventListener('click', () => handleMenuAction('clearTerminal'));

    // Terminal Interactive CLI Input
    const terminalCmdInput = document.getElementById('terminalCmdInput');
    if (terminalCmdInput) {
      terminalCmdInput.addEventListener('keydown', (e) => {
        if (e.key === 'Enter') {
          e.preventDefault();
          const cmd = terminalCmdInput.value;
          terminalCmdInput.value = '';
          executeTerminalCommand(cmd);
        } else if (e.key === 'ArrowUp') {
          e.preventDefault();
          if (terminalCmdHistory.length > 0) {
            if (terminalCmdIndex > 0) {
              terminalCmdIndex--;
            } else {
              terminalCmdIndex = 0;
            }
            terminalCmdInput.value = terminalCmdHistory[terminalCmdIndex] || '';
          }
        } else if (e.key === 'ArrowDown') {
          e.preventDefault();
          if (terminalCmdIndex < terminalCmdHistory.length - 1) {
            terminalCmdIndex++;
            terminalCmdInput.value = terminalCmdHistory[terminalCmdIndex] || '';
          } else {
            terminalCmdIndex = terminalCmdHistory.length;
            terminalCmdInput.value = '';
          }
        }
      });
    }

    // Extensions Search & Filter Controls
    const extSearchInput = document.getElementById('extensionSearchInput');
    let extDebounceTimer = null;
    if (extSearchInput) {
      extSearchInput.addEventListener('input', () => {
        clearTimeout(extDebounceTimer);
        extDebounceTimer = setTimeout(() => {
          searchOpenVsx(extSearchInput.value.trim());
        }, 400);
      });
      extSearchInput.addEventListener('keydown', (e) => {
        if (e.key === 'Enter') {
          clearTimeout(extDebounceTimer);
          searchOpenVsx(extSearchInput.value.trim());
        }
      });
    }

    const filterMarketplaceBtn = document.getElementById('filterMarketplaceBtn');
    const filterInstalledBtn = document.getElementById('filterInstalledBtn');
    if (filterMarketplaceBtn && filterInstalledBtn) {
      filterMarketplaceBtn.addEventListener('click', () => {
        filterMarketplaceBtn.classList.add('active');
        filterInstalledBtn.classList.remove('active');
        currentExtensionFilter = 'marketplace';
        renderExtensionsList('marketplace');
      });
      filterInstalledBtn.addEventListener('click', () => {
        filterInstalledBtn.classList.add('active');
        filterMarketplaceBtn.classList.remove('active');
        currentExtensionFilter = 'installed';
        renderExtensionsList('installed');
      });
    }

    const refreshExtensionsBtn = document.getElementById('refreshExtensionsBtn');
    if (refreshExtensionsBtn) {
      refreshExtensionsBtn.addEventListener('click', () => {
        const q = extSearchInput ? extSearchInput.value.trim() : '';
        searchOpenVsx(q);
      });
    }

    // Extension Details Modal Handlers
    const extensionModal = document.getElementById('extensionModal');
    const closeExtensionModal = document.getElementById('closeExtensionModal');
    const dismissExtensionModalBtn = document.getElementById('dismissExtensionModalBtn');
    const closeExtModal = () => { if (extensionModal) extensionModal.classList.remove('open'); };
    if (closeExtensionModal) closeExtensionModal.addEventListener('click', closeExtModal);
    if (dismissExtensionModalBtn) dismissExtensionModalBtn.addEventListener('click', closeExtModal);
    if (extensionModal) {
      extensionModal.addEventListener('click', (e) => {
        if (e.target === extensionModal) closeExtModal();
      });
    }

    // 9. BYOK Modal
    const byokConfigBtn = document.getElementById('byokConfigBtn');
    const copilotSettingsBtn = document.getElementById('copilotSettingsBtn');
    const byokModal = document.getElementById('byokModal');
    const closeByokModal = document.getElementById('closeByokModal');
    const cancelByokBtn = document.getElementById('cancelByokBtn');
    const saveByokBtn = document.getElementById('saveByokBtn');
    const modalApiKeyInput = document.getElementById('modalApiKeyInput');
    const modalProviderSelect = document.getElementById('modalProviderSelect');

    const openByok = () => {
      if (modalApiKeyInput) modalApiKeyInput.value = aiConfig.apiKey;
      if (modalProviderSelect) modalProviderSelect.value = aiConfig.provider;
      if (byokModal) byokModal.classList.add('open');
    };

    if (byokConfigBtn) byokConfigBtn.addEventListener('click', openByok);
    if (copilotSettingsBtn) copilotSettingsBtn.addEventListener('click', openByok);

    const closeByok = () => {
      if (byokModal) byokModal.classList.remove('open');
    };
    if (closeByokModal) closeByokModal.addEventListener('click', closeByok);
    if (cancelByokBtn) cancelByokBtn.addEventListener('click', closeByok);

    if (saveByokBtn) {
      saveByokBtn.addEventListener('click', () => {
        aiConfig.apiKey = modalApiKeyInput.value.trim();
        aiConfig.provider = modalProviderSelect.value;
        localStorage.setItem('enlangg_ai_key', aiConfig.apiKey);
        localStorage.setItem('enlangg_ai_provider', aiConfig.provider);

        const statusAiConnection = document.getElementById('statusAiConnection');
        if (statusAiConnection) {
          statusAiConnection.textContent = aiConfig.apiKey ? 'AI: Connected (BYOK)' : 'AI: No Key';
        }

        closeByok();
        alert('AI Configuration saved securely in client localStorage!');
      });
    }

    // 10. Quick Open (Ctrl + P)
    const quickOpenTrigger = document.getElementById('quickOpenTrigger');
    const quickOpenModal = document.getElementById('quickOpenModal');
    const quickOpenInput = document.getElementById('quickOpenInput');
    const quickOpenList = document.getElementById('quickOpenList');

    const openQuickModal = () => {
      if (!quickOpenModal || !quickOpenList) return;
      quickOpenModal.classList.add('open');
      quickOpenInput.value = '';
      quickOpenInput.focus();
      renderQuickOpenMatches('');
    };

    const closeQuickModal = () => {
      if (quickOpenModal) quickOpenModal.classList.remove('open');
    };

    function renderQuickOpenMatches(query) {
      if (!quickOpenList) return;
      quickOpenList.innerHTML = '';
      const matches = Object.keys(vfs).filter(f => f.toLowerCase().includes(query.toLowerCase()));
      for (const match of matches) {
        const row = document.createElement('div');
        row.className = 'tree-item';
        row.innerHTML = `<span>${match}</span>`;
        row.addEventListener('click', () => {
          openFile(match);
          closeQuickModal();
        });
        quickOpenList.appendChild(row);
      }
    }

    if (quickOpenTrigger) quickOpenTrigger.addEventListener('click', openQuickModal);
    if (quickOpenInput) {
      quickOpenInput.addEventListener('input', () => {
        renderQuickOpenMatches(quickOpenInput.value.trim());
      });
    }

    if (quickOpenModal) {
      quickOpenModal.addEventListener('click', (e) => {
        if (e.target === quickOpenModal) closeQuickModal();
      });
    }

    // 11. Shortcuts Modal
    const shortcutsModal = document.getElementById('shortcutsModal');
    const closeShortcutsModal = document.getElementById('closeShortcutsModal');
    const dismissShortcutsBtn = document.getElementById('dismissShortcutsBtn');
    const closeShortcuts = () => { if (shortcutsModal) shortcutsModal.classList.remove('open'); };
    if (closeShortcutsModal) closeShortcutsModal.addEventListener('click', closeShortcuts);
    if (dismissShortcutsBtn) dismissShortcutsBtn.addEventListener('click', closeShortcuts);
    if (shortcutsModal) shortcutsModal.addEventListener('click', (e) => {
      if (e.target === shortcutsModal) closeShortcuts();
    });

    // 12. About Modal
    const aboutModal = document.getElementById('aboutModal');
    const closeAboutModal = document.getElementById('closeAboutModal');
    const dismissAboutBtn = document.getElementById('dismissAboutBtn');
    const closeAbout = () => { if (aboutModal) aboutModal.classList.remove('open'); };
    if (closeAboutModal) closeAboutModal.addEventListener('click', closeAbout);
    if (dismissAboutBtn) dismissAboutBtn.addEventListener('click', closeAbout);
    if (aboutModal) aboutModal.addEventListener('click', (e) => {
      if (e.target === aboutModal) closeAbout();
    });

    // 13. PWA Desktop App Modal & Prompt
    const pwaInstallBtn = document.getElementById('pwaInstallBtn');
    const pwaModal = document.getElementById('pwaModal');
    const closePwaModal = document.getElementById('closePwaModal');
    const dismissPwaBtn = document.getElementById('dismissPwaBtn');
    const triggerPwaPromptBtn = document.getElementById('triggerPwaPromptBtn');

    const openPwaModal = () => {
      if (deferredPrompt) {
        deferredPrompt.prompt();
        deferredPrompt.userChoice.then(() => {
          deferredPrompt = null;
        });
      } else if (pwaModal) {
        pwaModal.classList.add('open');
      }
    };

    const closePwa = () => {
      if (pwaModal) pwaModal.classList.remove('open');
    };

    if (pwaInstallBtn) pwaInstallBtn.addEventListener('click', openPwaModal);
    if (closePwaModal) closePwaModal.addEventListener('click', closePwa);
    if (dismissPwaBtn) dismissPwaBtn.addEventListener('click', closePwa);
    if (pwaModal) pwaModal.addEventListener('click', (e) => {
      if (e.target === pwaModal) closePwa();
    });
    if (triggerPwaPromptBtn) {
      triggerPwaPromptBtn.addEventListener('click', () => {
        if (deferredPrompt) {
          deferredPrompt.prompt();
          deferredPrompt = null;
        }
        closePwa();
      });
    }

    // 14. Global Keyboard Shortcuts
    window.addEventListener('keydown', (e) => {
      // F1 for Shortcuts
      if (e.key === 'F1') {
        e.preventDefault();
        if (shortcutsModal) shortcutsModal.classList.add('open');
      }
      // F5 for DB console execute
      if (e.key === 'F5') {
        e.preventDefault();
        handleMenuAction('openDbConsole');
      }
      // Ctrl + P
      if ((e.ctrlKey || e.metaKey) && !e.shiftKey && e.key === 'p') {
        e.preventDefault();
        openQuickModal();
      }
      // Ctrl + S (Save)
      if ((e.ctrlKey || e.metaKey) && !e.shiftKey && e.key === 's') {
        e.preventDefault();
        saveActiveFile();
      }
      // Ctrl + Shift + S (Save All)
      if ((e.ctrlKey || e.metaKey) && e.shiftKey && (e.key === 'S' || e.key === 's')) {
        e.preventDefault();
        saveAllFiles();
      }
      // Ctrl + N (New File)
      if ((e.ctrlKey || e.metaKey) && !e.shiftKey && e.key === 'n') {
        e.preventDefault();
        handleMenuAction('newFile');
      }
      // Ctrl + W (Close Active Tab)
      if ((e.ctrlKey || e.metaKey) && !e.shiftKey && e.key === 'w') {
        e.preventDefault();
        handleMenuAction('closeTab');
      }
      // Ctrl + `
      if ((e.ctrlKey || e.metaKey) && e.key === '`') {
        e.preventDefault();
        bottomDock.classList.toggle('collapsed');
      }
      // Ctrl + B
      if ((e.ctrlKey || e.metaKey) && e.key === 'b') {
        e.preventDefault();
        mainSidebar.classList.toggle('collapsed');
      }
      // Ctrl + Shift + A (Copilot)
      if ((e.ctrlKey || e.metaKey) && e.shiftKey && (e.key === 'A' || e.key === 'a')) {
        e.preventDefault();
        copilotPanel.classList.toggle('open');
      }
      // Ctrl + Shift + E (Explorer)
      if ((e.ctrlKey || e.metaKey) && e.shiftKey && (e.key === 'E' || e.key === 'e')) {
        e.preventDefault();
        handleMenuAction('viewExplorer');
      }
      // Ctrl + Shift + F (Search)
      if ((e.ctrlKey || e.metaKey) && e.shiftKey && (e.key === 'F' || e.key === 'f')) {
        e.preventDefault();
        handleMenuAction('viewSearch');
      }
      // Ctrl + Shift + D (Database)
      if ((e.ctrlKey || e.metaKey) && e.shiftKey && (e.key === 'D' || e.key === 'd')) {
        e.preventDefault();
        handleMenuAction('viewDatabase');
      }
      // Ctrl + Shift + X (Extensions)
      if ((e.ctrlKey || e.metaKey) && e.shiftKey && (e.key === 'X' || e.key === 'x')) {
        e.preventDefault();
        handleMenuAction('viewExtensions');
      }
      // Ctrl + Shift + ` (New Terminal)
      if ((e.ctrlKey || e.metaKey) && e.shiftKey && e.key === '`') {
        e.preventDefault();
        handleMenuAction('newTerminal');
      }
      // Escape
      if (e.key === 'Escape') {
        closeAllMenus();
        closeQuickModal();
        closeByok();
        closeShortcuts();
        closeAbout();
        closePwa();
        closeExtModal();
      }
    });

    // 15. EnlangDB Inline Query Bar
    const dbExecuteBtn = document.getElementById('dbExecuteBtn');
    const dbQueryInput = document.getElementById('dbQueryInput');
    if (dbExecuteBtn && dbQueryInput) {
      dbExecuteBtn.addEventListener('click', () => {
        const q = dbQueryInput.value.trim();
        if (q) {
          executeEnlngDbInStudio(q);
        }
      });
    }
  }

  // Load sample template from Domains hub
  window.loadSampleTemplate = function(type) {
    if (type === 'subway') {
      vfs['game/subway_surfers.enlngd'] = `type enlngd\n-- Subway Surfers 3D Theme\npalette Track:\n    ground: #475569\n    train: #ef4444\n    coin: #fbbf24\n`;
      vfs['game/game_canvas.enlngf'] = `type enlngf\ncomponent SubwayTrack:\n    container styled as "track-view":\n        title "Subway Surfers Enlang Edition"\n`;
      openFile('game/subway_surfers.enlngd');
    } else if (type === 'banking') {
      openFile('src/main.enlng');
    } else if (type === 'database') {
      openFile('db/schema.enlngdb');
    } else if (type === 'mobile') {
      openFile('mobile/wallet.enlngm');
      if (previewPane) previewPane.classList.add('visible');
    }
  };

  // Initialization
  function init() {
    renderFileTree();
    renderTabs();
    loadActiveFileContent();
    updateBreadcrumbs();
    updateDomainPill();
    renderTerminalTabs();
    updateExtensionBadges();
    searchOpenVsx('');
    setupEventListeners();

    // Check status AI
    const statusAiConnection = document.getElementById('statusAiConnection');
    if (statusAiConnection) {
      statusAiConnection.textContent = aiConfig.apiKey ? 'AI: Connected (BYOK)' : 'AI: No Key (Click to set)';
      statusAiConnection.addEventListener('click', () => {
        const byokConfigBtn = document.getElementById('byokConfigBtn');
        if (byokConfigBtn) byokConfigBtn.click();
      });
    }

    console.log('[Enlangg Studio] Initialized 1:1 VS Code Native IDE.');
  }

  if (document.readyState === 'loading') {
    document.addEventListener('DOMContentLoaded', init);
  } else {
    init();
  }

})();
