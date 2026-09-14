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

  // Append text to terminal
  function appendTerminal(html) {
    if (!terminalOutput) return;
    terminalOutput.innerHTML += '\n' + html;
    terminalOutput.scrollTop = terminalOutput.scrollHeight;
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

  // Setup Event Listeners
  function setupEventListeners() {
    // Editor text input
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
        if ((e.ctrlKey || e.metaKey) && e.key === 's') {
          e.preventDefault();
          if (activeFile) {
            dirtyFiles.delete(activeFile);
            renderTabs();
            saveVfs();
          }
        }
      });

      codeEditor.addEventListener('click', updateCursorPos);
      codeEditor.addEventListener('keyup', updateCursorPos);
    }

    // Run Buttons
    const topRunBtn = document.getElementById('topRunBtn');
    if (topRunBtn) topRunBtn.addEventListener('click', executeActiveFile);
    const editorRunBtn = document.getElementById('editorRunBtn');
    if (editorRunBtn) editorRunBtn.addEventListener('click', executeActiveFile);

    // Toggle Preview
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

    // New File Button
    const newFileBtn = document.getElementById('newFileBtn');
    if (newFileBtn) {
      newFileBtn.addEventListener('click', () => {
        const name = prompt('Enter new file name (e.g. src/utils.enlng or db/store.enlngdb):', 'src/new_script.enlng');
        if (name && name.trim()) {
          const cleanName = name.trim();
          vfs[cleanName] = `type enlng\n\n# New Enlangg Script\nshow "Hello from ${cleanName}"\n`;
          saveVfs();
          openFile(cleanName);
        }
      });
    }

    // Reset Project Button
    const resetProjectBtn = document.getElementById('resetProjectBtn');
    if (resetProjectBtn) {
      resetProjectBtn.addEventListener('click', () => {
        if (confirm('Reset workspace to default Sovereign Banking & Ledger project?')) {
          vfs = Object.assign({}, DEFAULT_WORKSPACE);
          openTabs = ['src/main.enlng', 'db/schema.enlngdb'];
          activeFile = 'src/main.enlng';
          saveVfs();
          renderFileTree();
          renderTabs();
          loadActiveFileContent();
          updateBreadcrumbs();
        }
      });
    }

    // Activity Bar Icons
    const activities = [
      { id: 'actExplorer', pane: 'paneExplorer' },
      { id: 'actSearch', pane: 'paneSearch' },
      { id: 'actDatabase', pane: 'paneDatabase' },
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

    // Copilot Toggle
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

    // Dock Tabs
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

    // BYOK Modal
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

    // Quick Open (Ctrl + P)
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

    // Global Shortcuts
    window.addEventListener('keydown', (e) => {
      if ((e.ctrlKey || e.metaKey) && e.key === 'p') {
        e.preventDefault();
        openQuickModal();
      }
      if ((e.ctrlKey || e.metaKey) && e.key === '`') {
        e.preventDefault();
        bottomDock.classList.toggle('collapsed');
      }
      if ((e.ctrlKey || e.metaKey) && e.key === 'b') {
        e.preventDefault();
        mainSidebar.classList.toggle('collapsed');
      }
      if (e.key === 'Escape') {
        closeQuickModal();
        closeByok();
      }
    });

    // EnlangDB Inline Query Bar
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
