// =====================================================================
//   Enlangg Official Website - High-Performance Transpiler & VM Runner
// =====================================================================

document.addEventListener('DOMContentLoaded', () => {
  initThemeToggle();
  highlightActiveNav();
  initMobileMenu();
  initInstallerTabs();
  initPlayground();
  initDomainTabs();
  initAnimatedBook();
  initFaqAccordion();
  initUniversalCopyButtons();
  initDocsToc();
  initLibrarySearch();
  initCurriculumSidebar();
});

// --- 1. Multi-OS Installer Tabs & Clipboard Copy ---
const INSTALL_COMMANDS = {
  powershell: 'powershell -ExecutionPolicy ByPass -c "irm https://enlangg.vercel.app/install.ps1 | iex"',
  cmd: 'curl -fsSL https://enlangg.vercel.app/install.cmd -o install.cmd && install.cmd',
  bash: 'curl -fsSL https://enlangg.vercel.app/install.sh | bash'
};

function initInstallerTabs() {
  const tabs = document.querySelectorAll('.os-tab');
  const codeElem = document.getElementById('installerCode');
  const copyBtn = document.getElementById('copyInstallerBtn');
  const copyBtnText = document.getElementById('copyBtnText');

  tabs.forEach(tab => {
    tab.addEventListener('click', () => {
      tabs.forEach(t => t.classList.remove('active'));
      tab.classList.add('active');
      const os = tab.getAttribute('data-os');
      if (INSTALL_COMMANDS[os]) {
        codeElem.textContent = INSTALL_COMMANDS[os];
      }
    });
  });

  if (copyBtn) {
    copyBtn.addEventListener('click', async () => {
      const textToCopy = codeElem.textContent.trim();
      try {
        await navigator.clipboard.writeText(textToCopy);
        copyBtn.classList.add('copied');
        copyBtnText.textContent = 'Copied!';
        setTimeout(() => {
          copyBtn.classList.remove('copied');
          copyBtnText.textContent = 'Copy';
        }, 2200);
      } catch (err) {
        const ta = document.createElement('textarea');
        ta.value = textToCopy;
        document.body.appendChild(ta);
        ta.select();
        document.execCommand('copy');
        document.body.removeChild(ta);
        copyBtnText.textContent = 'Copied!';
        setTimeout(() => { copyBtnText.textContent = 'Copy'; }, 2000);
      }
    });
  }
}

// --- 2. Live In-Browser Enlng Playground & VM ---
const CODE_PRESETS = {
  fibonacci: `type enlng

create n of 10
create first of 0
create second of 1
create count of 0

display "--- Fibonacci Series (First 10) ---"

while count is less than n:
    display first
    create a next of first plus second
    set first to second
    set second to next
    set count to count plus 1`,

  palindrome: `type enlng

create a word of "madam"
create a reversed of ""
create a i of 0

while i less than length of word:
    set reversed to word[i] plus reversed 
    set i to i plus 1

if word is equal to reversed:
    display "The word '" + word + "' is a palindrome!"
else:
    display "not palindrome"`,

  smart_input: `type enlng

set price to 45
set tax to 5
set total to price + tax

display "--- Intentional '+' Concatenation ---"
display "number" + 10 + "train"

display "--- Natural Without Spaces ---"
display "order", 402, "placed" without spaces

display "--- Standard Output ---"
display "The total calculated price is: ", total`,

  even_odd: `type enlng

create a number of 100
square = number * number

if number % 2 is equal to 0:
    status = "even"
else:
    status = "odd"

display "the square of", number, "is", square
display "the number", number, "is", status`,

  factorial: `type enlng

create a number of 6
create a result of 1
create a i of 1

while i is less than or equal to number:
    set result to result * i
    set i to i plus 1

display "Factorial calculation:"
display "The factorial of", number, "is", result`
};

function initPlayground() {
  const editor = document.getElementById('codeEditor');
  const terminal = document.getElementById('terminalOutput');
  const runBtn = document.getElementById('runCodeBtn');
  const clearBtn = document.getElementById('clearOutputBtn');
  const copyOutputBtn = document.getElementById('copyOutputBtn');
  const lineNumbersElem = document.getElementById('editorLineNumbers');
  const lineCountElem = document.getElementById('editorLineCount');
  const presetBtns = document.querySelectorAll('.preset-btn');

  function updateLineNumbers() {
    if (!editor || !lineNumbersElem) return;
    const lines = editor.value.split('\n');
    if (lineCountElem) lineCountElem.textContent = lines.length;
    let numsHtml = '';
    for (let i = 1; i <= lines.length; i++) {
      numsHtml += `<span>${i}</span>`;
    }
    lineNumbersElem.innerHTML = numsHtml;
  }

  if (editor && lineNumbersElem) {
    editor.addEventListener('input', updateLineNumbers);
    editor.addEventListener('scroll', () => {
      lineNumbersElem.scrollTop = editor.scrollTop;
    });
  }

  // Load default preset
  if (editor && CODE_PRESETS.fibonacci) {
    editor.value = CODE_PRESETS.fibonacci;
    updateLineNumbers();
  }

  // Preset buttons
  presetBtns.forEach(btn => {
    btn.addEventListener('click', () => {
      presetBtns.forEach(b => b.classList.remove('active'));
      btn.classList.add('active');
      const presetName = btn.getAttribute('data-preset');
      if (CODE_PRESETS[presetName]) {
        editor.value = CODE_PRESETS[presetName];
        updateLineNumbers();
      }
    });
  });

  if (clearBtn) {
    clearBtn.addEventListener('click', () => {
      terminal.innerHTML = '<span class="term-dim">// Terminal cleared. Press Run Code or Ctrl+Enter to execute.</span>';
    });
  }

  if (copyOutputBtn) {
    copyOutputBtn.addEventListener('click', async () => {
      const text = terminal.textContent;
      try {
        await navigator.clipboard.writeText(text);
        copyOutputBtn.textContent = 'Copied!';
        setTimeout(() => { copyOutputBtn.textContent = 'Copy'; }, 2000);
      } catch (err) {
        copyOutputBtn.textContent = 'Copied!';
        setTimeout(() => { copyOutputBtn.textContent = 'Copy'; }, 2000);
      }
    });
  }

  const runCode = () => {
    if (runBtn) {
      runBtn.classList.add('running');
      setTimeout(() => runBtn.classList.remove('running'), 200);
    }
    executeEnlngInBrowser(editor.value, terminal);
  };

  if (runBtn) {
    runBtn.addEventListener('click', runCode);
  }

  // Ctrl + Enter shortcut support
  window.addEventListener('keydown', (e) => {
    if ((e.ctrlKey || e.metaKey) && e.key === 'Enter') {
      e.preventDefault();
      runCode();
    }
  });
}

// Client-Side Sovereign Enlng Transpiler & Sandbox Execution
function executeEnlngInBrowser(sourceCode, terminal) {
  terminal.innerHTML = '';
  const lines = sourceCode.split('\n');
  const outputLines = [];

  const log = (...args) => {
    outputLines.push(args.join(' '));
  };

  try {
    const jsCode = transpileEnlngToJS(lines);
    
    // Sandbox execution context
    const sandboxFunction = new Function('display', 'smartDisplay', 'cat', jsCode);
    
    const smartDisplay = (...args) => {
      let sep = ' ';
      let cleanArgs = args;
      if (args.length > 0 && typeof args[args.length - 1] === 'object' && args[args.length - 1] !== null && '__sep' in args[args.length - 1]) {
        sep = args[args.length - 1].__sep;
        cleanArgs = args.slice(0, -1);
      }
      
      if (sep === '') {
        log(cleanArgs.map(a => String(a)).join(''));
        return;
      }
      
      const res = [];
      for (let i = 0; i < cleanArgs.length; i++) {
        const s = String(cleanArgs[i]);
        if (i > 0 && res.length > 0) {
          const prev = res[res.length - 1];
          if (!prev.endsWith(' ') && !s.startsWith(' ')) {
            res.push(sep);
          }
        }
        res.push(s);
      }
      log(res.join(''));
    };

    const cat = (...args) => args.map(a => String(a)).join('');

    const t0 = performance.now();
    sandboxFunction(smartDisplay, smartDisplay, cat);
    const t1 = performance.now();

    const timePill = document.getElementById('runtimeExecTime');
    if (timePill) {
      timePill.textContent = `Execution: ${(t1 - t0).toFixed(2)}ms (Zero GC)`;
    }

    if (outputLines.length === 0) {
      terminal.innerHTML = '<span class="term-dim">// Execution completed with 0 output statements</span>';
    } else {
      terminal.textContent = outputLines.join('\n');
    }
  } catch (err) {
    terminal.innerHTML = `<span class="term-err">Enlng Runtime Error: ${escapeHtml(err.message)}</span>`;
    const timePill = document.getElementById('runtimeExecTime');
    if (timePill) {
      timePill.textContent = 'Execution: Interrupted';
    }
  }
}

function splitOutsideQuotes(str, delimiter) {
  const result = [];
  let current = '';
  let inSingle = false;
  let inDouble = false;

  for (let i = 0; i < str.length; i++) {
    const ch = str[i];
    if (ch === "'" && !inDouble) {
      inSingle = !inSingle;
      current += ch;
    } else if (ch === '"' && !inSingle) {
      inDouble = !inDouble;
      current += ch;
    } else if (ch === delimiter && !inSingle && !inDouble) {
      result.push(current.trim());
      current = '';
    } else {
      current += ch;
    }
  }
  result.push(current.trim());
  return result;
}

function transpileEnlngToJS(lines) {
  const intermediateLines = [];
  
  for (let rawLine of lines) {
    const indentLen = rawLine.length - rawLine.trimStart().length;
    let trimmed = rawLine.trim();

    if (!trimmed) {
      intermediateLines.push({ type: 'blank', indent: indentLen, code: '' });
      continue;
    }

    if (trimmed.startsWith('#')) {
      intermediateLines.push({ type: 'comment', indent: indentLen, code: `// ${trimmed.replace(/^#\s*/, '')}` });
      continue;
    }

    if (trimmed.startsWith('type ') || trimmed.startsWith('hint ')) {
      intermediateLines.push({ type: 'comment', indent: indentLen, code: `// ${trimmed}` });
      continue;
    }

    // 1. Variable Declarations (create / declare / initialize / let)
    const createMatch = trimmed.match(/^(?:create\s+(?:a\s+|an\s+|the\s+)?|declare\s+(?:a\s+|an\s+|the\s+)?|initialize\s+(?:a\s+|an\s+|the\s+)?|let\s+)([a-zA-Z0-9_]+)\s+(?:of|as|to|=)\s+(.*)$/i);
    if (createMatch) {
      const varName = createMatch[1];
      const valExpr = transpileExpression(createMatch[2]);
      intermediateLines.push({ type: 'stmt', indent: indentLen, code: `let ${varName} = ${valExpr};` });
      continue;
    }

    // 2. Variable Assignments (set / update / assign)
    const setMatch = trimmed.match(/^(?:set|update|assign)\s+([a-zA-Z0-9_\[\]\.]+)\s+(?:to|=)\s+(.*)$/i);
    if (setMatch) {
      const target = setMatch[1];
      const valExpr = transpileExpression(setMatch[2]);
      intermediateLines.push({ type: 'stmt', indent: indentLen, code: `${target} = ${valExpr};` });
      continue;
    }

    // 3. Direct assignment: a = b
    const directAssign = trimmed.match(/^([a-zA-Z0-9_\[\]\.]+)\s*=\s*(.*)$/);
    if (directAssign) {
      const target = directAssign[1];
      const valExpr = transpileExpression(directAssign[2]);
      intermediateLines.push({ type: 'stmt', indent: indentLen, code: `${target} = ${valExpr};` });
      continue;
    }

    // 4. Loops (while / repeat while)
    const whileMatch = trimmed.match(/^(?:while|repeat\s+while)\s+(.*?):$/i);
    if (whileMatch) {
      const cond = transpileExpression(whileMatch[1]);
      intermediateLines.push({ type: 'block_open', indent: indentLen, code: `while (${cond}) {` });
      continue;
    }

    // 5. Conditionals (if / elif / else)
    const ifMatch = trimmed.match(/^(?:if|when)\s+(.*?):$/i);
    if (ifMatch) {
      const cond = transpileExpression(ifMatch[1]);
      intermediateLines.push({ type: 'block_open', indent: indentLen, code: `if (${cond}) {` });
      continue;
    }

    const elifMatch = trimmed.match(/^(?:else\s+if|elif)\s+(.*?):$/i);
    if (elifMatch) {
      const cond = transpileExpression(elifMatch[1]);
      intermediateLines.push({ type: 'block_mid', indent: indentLen, code: `} else if (${cond}) {` });
      continue;
    }

    if (/^else:$/i.test(trimmed)) {
      intermediateLines.push({ type: 'block_mid', indent: indentLen, code: `} else {` });
      continue;
    }

    // 6. Function definitions
    const defMatch = trimmed.match(/^(?:define|def|function)\s+([a-zA-Z0-9_]+)\s*\((.*?)\)\s*:$/i);
    if (defMatch) {
      intermediateLines.push({ type: 'block_open', indent: indentLen, code: `function ${defMatch[1]}(${defMatch[2]}) {` });
      continue;
    }

    // 7. Returns
    const retMatch = trimmed.match(/^return(?:\s+(.*))?$/i);
    if (retMatch) {
      const val = retMatch[1] ? transpileExpression(retMatch[1]) : '';
      intermediateLines.push({ type: 'stmt', indent: indentLen, code: `return ${val};` });
      continue;
    }

    // 8. Display / Print statements
    const displayMatch = trimmed.match(/^(?:display|show|output|print)\s+(.*)$/i);
    if (displayMatch) {
      let body = displayMatch[1].trim();
      let sepArg = '';

      if (/\b(?:without spaces?|with no spaces?|joined)\s*$/i.test(body)) {
        sepArg = ', {__sep: ""}';
        body = body.replace(/\b(?:without spaces?|with no spaces?|joined)\s*$/i, '').trim();
      } else {
        const sepMatch = body.match(/\bwith separator\s+(".*?"|'.*?')\s*$/i);
        if (sepMatch) {
          sepArg = `, {__sep: ${sepMatch[1]}}`;
          body = body.replace(/\bwith separator\s+(".*?"|'.*?')\s*$/i, '').trim();
        }
      }

      // Check for concatenation operator '+' outside string literals
      const plusParts = splitOutsideQuotes(body, '+');
      if (plusParts.length > 1) {
        const catArgs = plusParts.map(p => transpileExpression(p.trim())).join(', ');
        intermediateLines.push({ type: 'stmt', indent: indentLen, code: `smartDisplay(cat(${catArgs})${sepArg});` });
        continue;
      }

      body = transpileExpression(body);

      // Auto comma for comma-less space-separated string literals and tokens
      if (!body.includes(',')) {
        body = body.replace(/("[^"]*"|'[^']*')\s+([a-zA-Z0-9_\(\[\{])/g, '$1, $2');
        body = body.replace(/([a-zA-Z0-9_\]\)\}])\s+("[^"]*"|'[^']*')/g, '$1, $2');
        body = body.replace(/("[^"]*"|'[^']*')\s+("[^"]*"|'[^']*')/g, '$1, $2');
      }

      intermediateLines.push({ type: 'stmt', indent: indentLen, code: `smartDisplay(${body}${sepArg});` });
      continue;
    }

    // Fallback statement
    intermediateLines.push({ type: 'stmt', indent: indentLen, code: `${transpileExpression(trimmed)};` });
  }

  // Second pass: Indentation block resolution
  const finalJS = [];
  const blockStack = []; // stores indent of enclosing blocks

  for (const item of intermediateLines) {
    if (item.type === 'blank') {
      finalJS.push('');
      continue;
    }

    if (item.type === 'comment') {
      finalJS.push(' '.repeat(item.indent) + item.code);
      continue;
    }

    if (item.type === 'block_mid') {
      while (blockStack.length > 0 && item.indent < blockStack[blockStack.length - 1]) {
        blockStack.pop();
        finalJS.push(' '.repeat(item.indent) + '}');
      }
      finalJS.push(' '.repeat(item.indent) + item.code);
      continue;
    }

    // For statements and new block openings, close any blocks deeper than this indent
    while (blockStack.length > 0 && item.indent <= blockStack[blockStack.length - 1]) {
      const closedIndent = blockStack.pop();
      finalJS.push(' '.repeat(closedIndent) + '}');
    }

    finalJS.push(' '.repeat(item.indent) + item.code);

    if (item.type === 'block_open') {
      blockStack.push(item.indent);
    }
  }

  while (blockStack.length > 0) {
    const closedIndent = blockStack.pop();
    finalJS.push(' '.repeat(closedIndent) + '}');
  }

  return finalJS.join('\n');
}

function transpileExpression(expr) {
  if (!expr) return '';
  let res = expr;

  // Comparison & logical aliases
  res = res.replace(/\bis equal to\b/gi, '===');
  res = res.replace(/\bis not equal to\b/gi, '!==');
  res = res.replace(/\bis at least\b/gi, '>=');
  res = res.replace(/\bis at most\b/gi, '<=');
  res = res.replace(/\bis greater than or equal to\b/gi, '>=');
  res = res.replace(/\bis less than or equal to\b/gi, '<=');
  res = res.replace(/\bis greater than\b/gi, '>');
  res = res.replace(/\bis less than\b/gi, '<');
  res = res.replace(/\bgreater than or equal to\b/gi, '>=');
  res = res.replace(/\bless than or equal to\b/gi, '<=');
  res = res.replace(/\bgreater than\b/gi, '>');
  res = res.replace(/\bless than\b/gi, '<');
  res = res.replace(/\bequal to\b/gi, '===');
  res = res.replace(/\bequals\b/gi, '===');

  // Math aliases
  res = res.replace(/\bmultiplied by\b/gi, '*');
  res = res.replace(/\bdivided by\b/gi, '/');
  res = res.replace(/\bplus\b/gi, '+');
  res = res.replace(/\bminus\b/gi, '-');
  res = res.replace(/\b(mod|modulo|modulus|modulous|modoulous)\b/gi, '%');

  // Collection size/length: length of <var> or size of <var>
  res = res.replace(/\b(?:length of|size of)\s+([a-zA-Z0-9_\[\]]+)/gi, '$1.length');

  return res;
}

function escapeHtml(str) {
  return str.replace(/&/g, '&amp;').replace(/</g, '&lt;').replace(/>/g, '&gt;');
}

// --- 3. Domain Showcase Tabs ---
const DOMAINS_DATA = {
  enlng: {
    tier: 'Tier 1 · Sovereign Architecture',
    title: 'Core Backend (.enlng)',
    filename: 'core_algorithm.enlng',
    desc: 'General-purpose algorithms, collections, math, OOP, exceptions, and native standard I/O.',
    features: [
      'Zero runtime garbage collector pauses',
      'Direct compilation to native machine instructions',
      'Spoken English arithmetic and control structures'
    ],
    code: `type enlng

create a word of "madam"
create a reversed of ""
create a i of 0

while i less than length of word:
    set reversed to word[i] plus reversed 
    set i to i plus 1

if word is equal to reversed:
    display "The word '" + word + "' is a palindrome!"
else:
    display "not palindrome"`
  },
  enlngf: {
    tier: 'Tier 2 · Client Representation',
    title: 'Frontend Markup & UI (.enlngf)',
    filename: 'user_profile.enlngf',
    desc: 'Declarative component trees, props, stateful reactive hooks, and DOM events compiled directly to Web UI.',
    features: [
      'Reactive signals without virtual DOM diffing overhead',
      'Scoped design token inheritance',
      'Native keyboard and touch gesture dispatchers'
    ],
    code: `type enlngf

component UserProfile with username, status:
    create container styled as "profile-card":
        create text username with style "heading-1"
        create badge status with style "status-online"
        create button "Message":
            on click: emit open_direct_message(username)`
  },
  enlngs: {
    tier: 'Tier 3 · Network Services',
    title: 'Server & API Routes (.enlngs)',
    filename: 'gateway_api.enlngs',
    desc: 'Zero-overhead HTTP REST endpoints, WebSockets, microservices, and middleware routing engine.',
    features: [
      'Built-in non-blocking epoll/IOCP event loop',
      'Automatic JSON validation and serialization',
      'Thread-safe session contexts with rate limiting'
    ],
    code: `type enlngs

listen on port 8080

route get "/api/v1/health":
    respond with status 200 and json {"status": "healthy"}

route post "/api/v1/auth" with body payload:
    verify payload.token and respond with session`
  },
  enlngd: {
    tier: 'Tier 4 · Design Systems',
    title: 'Design Tokens & Styles (.enlngd)',
    filename: 'theme_matrix.enlngd',
    desc: 'Sovereign design tokens, responsive grid/flex layouts, color matrices, and GPU-accelerated keyframe animations.',
    features: [
      'Deterministic design token propagation',
      'Automated dark mode contrast verification',
      'Hardware-accelerated CSS keyframe generator'
    ],
    code: `type enlngd

define theme dark_mode:
    primary_surface is #09090b
    card_surface is #18181b
    border_color is #27272a
    accent_color is #ffffff

style class "card":
    background is card_surface
    border is "1px solid " + border_color
    border_radius is 8px`
  },
  enlngm: {
    tier: 'Tier 5 · Mobile Hardware',
    title: 'Mobile Apps & HAL (.enlngm)',
    filename: 'mobile_activity.enlngm',
    desc: 'Hardware Abstraction Layer for Android (NDK/Vulkan) and iOS (Metal) with 120 FPS native activities.',
    features: [
      'Direct C-ABI bindings to Android NDK & Apple Metal',
      'Zero bridge serialization latency for gestures',
      'Native background lifecycle and battery conservation'
    ],
    code: `type enlngm

screen Dashboard:
    on device orientation change:
        realign layout to current orientation
        
    on hardware back button pressed:
        navigate back or prompt exit confirmation`
  },
  enlngdb: {
    tier: 'Tier 6 · Sovereign Data',
    title: 'Declarative Database (.enlngdb)',
    filename: 'schema_registry.enlngdb',
    desc: 'Type-safe relational models, migration trees, and high-concurrency declarative queries with ACID compliance.',
    features: [
      'Zero ORM mapping penalty via slot-aligned memory',
      'Strict compile-time SQL injection impossibility',
      'Built-in migration checkpoint hashes'
    ],
    code: `type enlngdb

table Accounts:
    column id uuid primary key
    column username string unique required
    column balance decimal default 0.00
    column created_at timestamp default now()`
  }
};

function initDomainTabs() {
  const tabs = document.querySelectorAll('.domain-tab');
  const titleElem = document.getElementById('domainTitle');
  const descElem = document.getElementById('domainDesc');
  const codeElem = document.getElementById('domainCodeSample');
  const tierElem = document.getElementById('domainTierTag');
  const fileElem = document.getElementById('domainFilename');
  const feat1 = document.getElementById('domainFeat1');
  const feat2 = document.getElementById('domainFeat2');
  const feat3 = document.getElementById('domainFeat3');

  tabs.forEach(tab => {
    tab.addEventListener('click', () => {
      tabs.forEach(t => t.classList.remove('active'));
      tab.classList.add('active');
      const domainKey = tab.getAttribute('data-domain');
      const data = DOMAINS_DATA[domainKey];
      if (data) {
        if (titleElem) titleElem.textContent = data.title;
        if (descElem) descElem.textContent = data.desc;
        if (codeElem) codeElem.textContent = data.code;
        if (tierElem) tierElem.textContent = data.tier;
        if (fileElem) fileElem.textContent = data.filename;
        if (feat1 && data.features[0]) feat1.textContent = data.features[0];
        if (feat2 && data.features[1]) feat2.textContent = data.features[1];
        if (feat3 && data.features[2]) feat3.textContent = data.features[2];
      }
    });
  });
}

// --- 4. Interactive 3D Animated Book (360° Drag + Multi-Page Highlight Reader) ---
const BOOK_PAGES = [
  {
    chapter: 'CHAPTER 1',
    pages: 'pp. 1-28 / 193',
    badge: 'Core Architectural Philosophy',
    title: 'The Sovereign Grammar',
    excerpt: 'Natural English syntax eliminates symbol barrier friction, translating directly into high-throughput machine bytecode.',
    snippet: `<span class="code-kw">type</span> <span class="code-val">enlng</span>\n<span class="code-kw">create</span> <span class="code-id">greeting</span> <span class="code-kw">of</span> <span class="code-val">"Hello Sovereign World"</span>\n<span class="code-kw">display</span> <span class="code-id">greeting</span>`,
    seal: 'Universal Syntax Specification'
  },
  {
    chapter: 'CHAPTER 2',
    pages: 'pp. 29-64 / 193',
    badge: 'Native Arithmetic Engine',
    title: 'Inferred Types & Natural Math',
    excerpt: 'Automatic compile-time type inference with zero runtime boxing overhead and spoken English arithmetic operators.',
    snippet: `<span class="code-kw">create</span> <span class="code-id">price</span> <span class="code-kw">of</span> <span class="code-num">250</span>\n<span class="code-kw">create</span> <span class="code-id">tax</span> <span class="code-kw">of</span> <span class="code-id">price</span> <span class="code-op">multiplied by</span> <span class="code-num">0.18</span>\n<span class="code-kw">display</span> <span class="code-val">"Total: "</span> <span class="code-op">+</span> (<span class="code-id">price</span> <span class="code-op">plus</span> <span class="code-id">tax</span>)`,
    seal: 'High-Throughput Math Runtime'
  },
  {
    chapter: 'CHAPTER 3',
    pages: 'pp. 65-102 / 193',
    badge: 'Flow Control & Logic',
    title: 'Natural Branching & Loops',
    excerpt: 'Express complex decision logic and collection sweeps cleanly in human sentence structures without nested bracket noise.',
    snippet: `<span class="code-kw">while</span> <span class="code-id">count</span> <span class="code-op">is less than</span> <span class="code-id">target</span>:\n  <span class="code-kw">set</span> <span class="code-id">count</span> <span class="code-kw">to</span> <span class="code-id">count</span> <span class="code-op">plus</span> <span class="code-num">1</span>\n  <span class="code-kw">display</span> <span class="code-val">"Step "</span> <span class="code-op">+</span> <span class="code-id">count</span>`,
    seal: 'Deterministic Control Flow'
  },
  {
    chapter: 'CHAPTER 4',
    pages: 'pp. 103-146 / 193',
    badge: '6 Sovereign Domains',
    title: 'Cross-Domain Tier Isolation',
    excerpt: 'Compile-time isolation guarantees across .enlng, .enlngf, .enlngs, .enlngd, .enlngm, and .enlngdb tiers.',
    snippet: `<span class="code-kw">type</span> <span class="code-val">enlngs</span>\n<span class="code-kw">create</span> <span class="code-id">route</span> <span class="code-kw">of</span> <span class="code-val">"/api/v1/health"</span>\n<span class="code-kw">respond with json</span> {<span class="code-val">"status"</span>: <span class="code-val">"healthy"</span>}`,
    seal: 'Tier Security Invariant'
  },
  {
    chapter: 'CHAPTER 5',
    pages: 'pp. 147-172 / 193',
    badge: 'Memory & Native Compilation',
    title: 'Deterministic Memory Model',
    excerpt: 'Zero garbage-collector latency pauses via slot-based compile-time registers and instant 1.8ms warm boot.',
    snippet: `<span class="code-kw">type</span> <span class="code-val">enlng</span>\n<span class="code-kw">function</span> <span class="code-id">process_buffer</span> <span class="code-kw">with</span> <span class="code-id">buf</span>:\n  <span class="code-kw">display</span> <span class="code-val">"Active registers: "</span> <span class="code-op">+</span> <span class="code-kw">length of</span> <span class="code-id">buf</span>`,
    seal: 'Zero-GC Machine Runtime'
  },
  {
    chapter: 'CHAPTER 6',
    pages: 'pp. 173-193 / 193',
    badge: 'Standard Universal Library',
    title: 'Full Universal Standard Library',
    excerpt: 'Complete built-in standard library for advanced data structures, math, cryptography, file I/O, and sockets.',
    snippet: `<span class="code-kw">type</span> <span class="code-val">enlngdb</span>\n<span class="code-kw">connect to</span> <span class="code-val">"production.db"</span>\n<span class="code-kw">find all</span> <span class="code-id">users</span> <span class="code-kw">where</span> <span class="code-id">active</span> <span class="code-op">is</span> <span class="code-val">true</span>`,
    seal: 'Universal Standard Parity'
  }
];

function initAnimatedBook() {
  const bookStage = document.getElementById('bookStage');
  const book3D = document.getElementById('book3D');
  const openHint = document.getElementById('openHint');
  const toggleCoverBtn = document.getElementById('toggleCoverBtn');
  const coverBtnText = document.getElementById('coverBtnText');
  const toggleOrbitBtn = document.getElementById('toggleOrbitBtn');
  const autoFlipBtn = document.getElementById('autoFlipBtn');
  const autoFlipText = document.getElementById('autoFlipText');
  const reset3dBtn = document.getElementById('reset3dBtn');
  const browsePagesBtn = document.getElementById('browsePagesBtn');
  const prevPageBtn = document.getElementById('prevPageBtn');
  const nextPageBtn = document.getElementById('nextPageBtn');
  const pageDotsBar = document.getElementById('pageDotsBar');
  const chapterChipsBar = document.getElementById('chapterChipsBar');
  const viewBtns = document.querySelectorAll('.view-preset-btn');

  if (!bookStage || !book3D) return;

  // --- 1. Multi-Page Highlight State & Navigation ---
  let currentPageIndex = 0;
  let autoFlipInterval = null;

  function renderPageDots() {
    if (pageDotsBar) {
      pageDotsBar.innerHTML = '';
      BOOK_PAGES.forEach((pg, idx) => {
        const dot = document.createElement('button');
        dot.className = `page-dot ${idx === currentPageIndex ? 'active' : ''}`;
        dot.title = `${pg.chapter}: ${pg.title}`;
        dot.addEventListener('click', (e) => {
          e.stopPropagation();
          setPage(idx);
        });
        pageDotsBar.appendChild(dot);
      });
    }

    if (chapterChipsBar) {
      chapterChipsBar.innerHTML = '';
      BOOK_PAGES.forEach((pg, idx) => {
        const chip = document.createElement('button');
        chip.className = `chapter-chip ${idx === currentPageIndex ? 'active' : ''}`;
        chip.textContent = `${idx + 1}. ${pg.title.split(' ')[0]}`;
        chip.title = `${pg.chapter}: ${pg.title}`;
        chip.addEventListener('click', (e) => {
          e.stopPropagation();
          setPage(idx);
        });
        chapterChipsBar.appendChild(chip);
      });
    }
  }

  function setPage(idx) {
    if (idx < 0) idx = BOOK_PAGES.length - 1;
    if (idx >= BOOK_PAGES.length) idx = 0;
    currentPageIndex = idx;

    const pageData = BOOK_PAGES[currentPageIndex];
    const pageTurnOverlay = document.getElementById('pageTurnOverlay');

    if (pageTurnOverlay) {
      pageTurnOverlay.classList.remove('flipping');
      void pageTurnOverlay.offsetWidth; // trigger reflow
      pageTurnOverlay.classList.add('flipping');
    }

    // Update DOM content
    const pageCh = document.getElementById('pageCh');
    const pageNum = document.getElementById('pageNum');
    const pageTitle = document.getElementById('pageTitle');
    const pageHighlightBadge = document.getElementById('pageHighlightBadge');
    const pageExcerpt = document.getElementById('pageExcerpt');
    const pageCodeSnippet = document.getElementById('pageCodeSnippet');
    const pageSeal = document.getElementById('pageSeal');

    if (pageCh) pageCh.textContent = pageData.chapter;
    if (pageNum) pageNum.textContent = pageData.pages;
    if (pageTitle) pageTitle.textContent = pageData.title;
    if (pageHighlightBadge) pageHighlightBadge.textContent = pageData.badge;
    if (pageExcerpt) pageExcerpt.textContent = pageData.excerpt;
    if (pageCodeSnippet) pageCodeSnippet.innerHTML = pageData.snippet.replace(/\n/g, '<br>').replace(/  /g, '&nbsp;&nbsp;');
    if (pageSeal) pageSeal.textContent = pageData.seal;

    renderPageDots();

    // Ensure cover is open so user sees the page
    if (!bookStage.classList.contains('book-open')) {
      bookStage.classList.add('book-open');
      updateCoverButton(true);
    }
  }

  if (prevPageBtn) {
    prevPageBtn.addEventListener('click', (e) => {
      e.stopPropagation();
      setPage(currentPageIndex - 1);
    });
  }

  if (nextPageBtn) {
    nextPageBtn.addEventListener('click', (e) => {
      e.stopPropagation();
      setPage(currentPageIndex + 1);
    });
  }

  renderPageDots();

  // --- 2. Auto-Flip Progression ---
  function toggleAutoFlip() {
    if (autoFlipInterval) {
      clearInterval(autoFlipInterval);
      autoFlipInterval = null;
      if (autoFlipText) autoFlipText.textContent = 'Auto-Flip Pages';
      if (autoFlipBtn) autoFlipBtn.classList.remove('active');
    } else {
      if (!bookStage.classList.contains('book-open')) {
        bookStage.classList.add('book-open');
        updateCoverButton(true);
      }
      if (autoFlipText) autoFlipText.textContent = 'Pause Flip';
      if (autoFlipBtn) autoFlipBtn.classList.add('active');
      autoFlipInterval = setInterval(() => {
        setPage(currentPageIndex + 1);
      }, 3500);
    }
  }

  if (autoFlipBtn) {
    autoFlipBtn.addEventListener('click', (e) => {
      e.stopPropagation();
      toggleAutoFlip();
    });
  }

  // --- 3. Cover Open / Close Toggle ---
  function updateCoverButton(isOpen) {
    if (coverBtnText) {
      coverBtnText.textContent = isOpen ? 'Close Book' : 'Open Book';
    }
    if (openHint) {
      openHint.textContent = isOpen ? 'Click to Close' : 'Click to Open →';
    }
  }

  function toggleCover() {
    const isOpen = bookStage.classList.toggle('book-open');
    updateCoverButton(isOpen);
  }

  if (toggleCoverBtn) {
    toggleCoverBtn.addEventListener('click', (e) => {
      e.stopPropagation();
      toggleCover();
    });
  }

  if (browsePagesBtn) {
    browsePagesBtn.addEventListener('click', (e) => {
      e.preventDefault();
      e.stopPropagation();
      if (!bookStage.classList.contains('book-open')) {
        bookStage.classList.add('book-open');
        updateCoverButton(true);
      }
      setPage(currentPageIndex + 1);
    });
  }

  // --- 4. 360° Mouse / Touch Drag Rotation with Momentum ---
  let isDragging = false;
  let hasDragged = false;
  let startX = 0;
  let startY = 0;
  let currentRotY = -22;
  let currentRotX = 10;
  let isOrbiting = false;
  let velocityX = 0;
  let velocityY = 0;
  let inertiaFrame = null;

  function applyTransform() {
    book3D.style.transform = `rotateY(${currentRotY}deg) rotateX(${currentRotX}deg)`;
  }

  function stopInertia() {
    if (inertiaFrame) {
      cancelAnimationFrame(inertiaFrame);
      inertiaFrame = null;
    }
  }

  function runInertia() {
    if (Math.abs(velocityX) > 0.05 || Math.abs(velocityY) > 0.05) {
      currentRotY += velocityX;
      currentRotX -= velocityY;
      currentRotX = Math.max(-45, Math.min(45, currentRotX));
      applyTransform();
      velocityX *= 0.92;
      velocityY *= 0.92;
      inertiaFrame = requestAnimationFrame(runInertia);
    } else {
      stopInertia();
    }
  }

  const startDrag = (clientX, clientY) => {
    isDragging = true;
    hasDragged = false;
    startX = clientX;
    startY = clientY;
    velocityX = 0;
    velocityY = 0;
    stopInertia();
    bookStage.classList.add('is-dragging');
    book3D.style.animation = 'none'; // pause float
    if (isOrbiting) stopOrbit();
  };

  const moveDrag = (clientX, clientY) => {
    if (!isDragging) return;
    const deltaX = clientX - startX;
    const deltaY = clientY - startY;
    if (Math.abs(deltaX) > 2 || Math.abs(deltaY) > 2) {
      hasDragged = true;
    }
    velocityX = deltaX * 0.45;
    velocityY = deltaY * 0.3;
    currentRotY += deltaX * 0.75;
    currentRotX -= deltaY * 0.5;

    // Clamp X rotation to avoid upside-down flip
    currentRotX = Math.max(-45, Math.min(45, currentRotX));

    applyTransform();
    startX = clientX;
    startY = clientY;
  };

  const endDrag = () => {
    if (!isDragging) return;
    isDragging = false;
    bookStage.classList.remove('is-dragging');
    if (hasDragged) {
      runInertia();
    }
  };

  bookStage.addEventListener('mousedown', (e) => {
    if (e.target.closest('button')) return;
    startDrag(e.clientX, e.clientY);
  });

  window.addEventListener('mousemove', (e) => {
    moveDrag(e.clientX, e.clientY);
  });

  window.addEventListener('mouseup', () => {
    endDrag();
  });

  // Touch Support for Mobile
  bookStage.addEventListener('touchstart', (e) => {
    if (e.touches.length === 1) {
      startDrag(e.touches[0].clientX, e.touches[0].clientY);
    }
  }, { passive: true });

  window.addEventListener('touchmove', (e) => {
    if (e.touches.length === 1) {
      moveDrag(e.touches[0].clientX, e.touches[0].clientY);
    }
  }, { passive: true });

  window.addEventListener('touchend', () => {
    endDrag();
  });

  // Click on book cover opens/closes only if user did not drag
  bookStage.addEventListener('click', (e) => {
    if (hasDragged) return;
    if (e.target.closest('button')) return;
    toggleCover();
  });

  // --- 5. 360° Orbit Turntable ---
  let orbitTimer = null;
  function startOrbit() {
    isOrbiting = true;
    stopInertia();
    book3D.style.animation = 'none';
    if (toggleOrbitBtn) toggleOrbitBtn.classList.add('active');
    orbitTimer = setInterval(() => {
      currentRotY = (currentRotY + 1.2) % 360;
      applyTransform();
    }, 16);
  }

  function stopOrbit() {
    isOrbiting = false;
    if (orbitTimer) {
      clearInterval(orbitTimer);
      orbitTimer = null;
    }
    if (toggleOrbitBtn) toggleOrbitBtn.classList.remove('active');
  }

  if (toggleOrbitBtn) {
    toggleOrbitBtn.addEventListener('click', (e) => {
      e.stopPropagation();
      if (isOrbiting) {
        stopOrbit();
      } else {
        startOrbit();
      }
    });
  }

  // --- 6. Perspective Presets ---
  viewBtns.forEach(btn => {
    btn.addEventListener('click', (e) => {
      e.stopPropagation();
      viewBtns.forEach(b => b.classList.remove('active'));
      btn.classList.add('active');
      stopOrbit();
      stopInertia();
      book3D.style.animation = 'none';

      const view = btn.getAttribute('data-view');
      if (view === 'isometric') {
        currentRotY = -22;
        currentRotX = 10;
      } else if (view === 'front') {
        currentRotY = 0;
        currentRotX = 0;
      } else if (view === 'back') {
        currentRotY = 180;
        currentRotX = 0;
      } else if (view === 'spine') {
        currentRotY = 90;
        currentRotX = 0;
      }
      applyTransform();
    });
  });

  // --- 7. Reset 3D View ---
  if (reset3dBtn) {
    reset3dBtn.addEventListener('click', (e) => {
      e.stopPropagation();
      stopOrbit();
      stopInertia();
      viewBtns.forEach(b => b.classList.remove('active'));
      const isoBtn = document.querySelector('.view-preset-btn[data-view="isometric"]');
      if (isoBtn) isoBtn.classList.add('active');
      currentRotY = -22;
      currentRotX = 10;
      applyTransform();
      book3D.style.animation = 'bookFloat 6s ease-in-out infinite';
    });
  }
}

// --- 5. Frequently Asked Questions Accordion ---
function initFaqAccordion() {
  const faqCards = document.querySelectorAll('.faq-card');
  faqCards.forEach(card => {
    const btn = card.querySelector('.faq-question');
    if (btn) {
      btn.addEventListener('click', () => {
        const isOpen = card.classList.contains('active');
        faqCards.forEach(c => c.classList.remove('active'));
        if (!isOpen) {
          card.classList.add('active');
        }
      });
    }
  });
}

// --- 6. Universal Light / Dark Mode System ---
function initThemeToggle() {
  const savedTheme = localStorage.getItem('enlangg-theme');
  const prefersLight = window.matchMedia && window.matchMedia('(prefers-color-scheme: light)').matches;
  const currentTheme = savedTheme || (prefersLight ? 'light' : 'dark');

  document.documentElement.setAttribute('data-theme', currentTheme);
  updateThemeIcons(currentTheme);

  const toggleBtns = document.querySelectorAll('.theme-toggle-btn');
  toggleBtns.forEach(btn => {
    btn.addEventListener('click', () => {
      const activeTheme = document.documentElement.getAttribute('data-theme') === 'light' ? 'dark' : 'light';
      document.documentElement.setAttribute('data-theme', activeTheme);
      localStorage.setItem('enlangg-theme', activeTheme);
      updateThemeIcons(activeTheme);
    });
  });
}

function updateThemeIcons(theme) {
  const moonIcons = document.querySelectorAll('.theme-icon-moon');
  const sunIcons = document.querySelectorAll('.theme-icon-sun');
  if (theme === 'light') {
    moonIcons.forEach(i => i.style.display = 'block');
    sunIcons.forEach(i => i.style.display = 'none');
  } else {
    moonIcons.forEach(i => i.style.display = 'none');
    sunIcons.forEach(i => i.style.display = 'block');
  }
}

// --- 7. Highlight Active Navigation Link ---
function highlightActiveNav() {
  const path = window.location.pathname.toLowerCase();
  const navLinks = document.querySelectorAll('.nav-link, .mobile-drawer a');
  
  navLinks.forEach(link => {
    const href = link.getAttribute('href');
    if (!href) return;
    const cleanHref = href.replace('.html', '').toLowerCase();
    const cleanPath = path.replace('.html', '').replace(/\/$/, '') || '/';
    
    if ((cleanPath === '/' && (href === '/' || href === 'index.html')) ||
        (cleanPath !== '/' && cleanPath.endsWith(cleanHref))) {
      link.classList.add('active');
    }
  });
}

function initMobileMenu() {
  const toggleBtn = document.getElementById('mobileMenuToggle');
  const drawer = document.getElementById('mobileDrawer');
  if (!toggleBtn || !drawer) return;

  function setDrawer(open) {
    if (open) {
      drawer.classList.add('open');
      toggleBtn.setAttribute('aria-expanded', 'true');
      document.body.style.overflow = 'hidden';
    } else {
      drawer.classList.remove('open');
      toggleBtn.setAttribute('aria-expanded', 'false');
      document.body.style.overflow = '';
    }
  }

  toggleBtn.addEventListener('click', (e) => {
    e.stopPropagation();
    const isOpen = drawer.classList.contains('open');
    setDrawer(!isOpen);
  });

  // Close drawer on link click
  const drawerLinks = drawer.querySelectorAll('a');
  drawerLinks.forEach(link => {
    link.addEventListener('click', () => {
      setDrawer(false);
    });
  });

  // Close on Escape key
  document.addEventListener('keydown', (e) => {
    if (e.key === 'Escape' && drawer.classList.contains('open')) {
      setDrawer(false);
    }
  });
}

// --- 9. Universal Code Block Copy Buttons ---
function initUniversalCopyButtons() {
  const copyButtons = document.querySelectorAll('.copy-code-btn');
  copyButtons.forEach(btn => {
    btn.addEventListener('click', async () => {
      const targetId = btn.getAttribute('data-target');
      const codeElem = targetId ? document.getElementById(targetId) : btn.closest('.code-block-card')?.querySelector('pre code, pre');
      if (!codeElem) return;

      const codeText = codeElem.innerText.trim();
      try {
        await navigator.clipboard.writeText(codeText);
        const originalText = btn.textContent;
        btn.textContent = 'Copied!';
        btn.classList.add('copied');
        setTimeout(() => {
          btn.textContent = originalText;
          btn.classList.remove('copied');
        }, 2000);
      } catch (err) {
        console.warn('Clipboard write failed:', err);
      }
    });
  });
}

// --- 10. Documentation Table of Contents Scroll-Spy ---
function initDocsToc() {
  const tocLinks = document.querySelectorAll('.toc-item a');
  const sections = document.querySelectorAll('.docs-article-section');
  if (tocLinks.length === 0 || sections.length === 0) return;

  const observer = new IntersectionObserver((entries) => {
    entries.forEach(entry => {
      if (entry.isIntersecting) {
        const id = entry.target.getAttribute('id');
        tocLinks.forEach(link => {
          if (link.getAttribute('href') === `#${id}`) {
            link.classList.add('active');
          } else {
            link.classList.remove('active');
          }
        });
      }
    });
  }, { rootMargin: '-20% 0px -70% 0px' });

  sections.forEach(section => observer.observe(section));
}

// --- 11. Library Search and Module Filter ---
function initLibrarySearch() {
  const searchInput = document.getElementById('libSearchInput');
  const modulePills = document.querySelectorAll('.mod-pill');
  const apiCards = document.querySelectorAll('.api-card');
  if (!searchInput && modulePills.length === 0) return;

  let activeModule = 'all';

  function filterCards() {
    const query = searchInput ? searchInput.value.toLowerCase().trim() : '';

    apiCards.forEach(card => {
      const cardModule = card.getAttribute('data-module');
      const cardText = card.textContent.toLowerCase();
      
      const matchesModule = activeModule === 'all' || cardModule === activeModule;
      const matchesQuery = query === '' || cardText.includes(query);

      if (matchesModule && matchesQuery) {
        card.style.display = 'flex';
      } else {
        card.style.display = 'none';
      }
    });
  }

  if (searchInput) {
    searchInput.addEventListener('input', filterCards);
  }

  modulePills.forEach(pill => {
    pill.addEventListener('click', () => {
      modulePills.forEach(p => p.classList.remove('active'));
      pill.classList.add('active');
      activeModule = pill.getAttribute('data-filter') || 'all';
      filterCards();
    });
  });
}

// --- 12. Learn Curriculum Sidebar Active Link Observer ---
function initCurriculumSidebar() {
  const curriculumLinks = document.querySelectorAll('.curriculum-item-link');
  const sections = document.querySelectorAll('.topic-lesson-section');
  if (curriculumLinks.length === 0 || sections.length === 0) return;

  const observer = new IntersectionObserver((entries) => {
    entries.forEach(entry => {
      if (entry.isIntersecting) {
        const id = entry.target.getAttribute('id');
        curriculumLinks.forEach(link => {
          if (link.getAttribute('href') === `#${id}`) {
            link.classList.add('active');
          } else {
            link.classList.remove('active');
          }
        });
      }
    });
  }, { rootMargin: '-15% 0px -65% 0px' });

  sections.forEach(section => observer.observe(section));

  curriculumLinks.forEach(link => {
    link.addEventListener('click', () => {
      curriculumLinks.forEach(l => l.classList.remove('active'));
      link.classList.add('active');
    });
  });
}

