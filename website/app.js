// =====================================================================
//   Enlangg Official Website - High-Performance Transpiler & VM Runner
// =====================================================================

document.addEventListener('DOMContentLoaded', () => {
  initInstallerTabs();
  initPlayground();
  initDomainTabs();
  initAnimatedBook();
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
        copyBtnText.textContent = 'Copied! ✓';
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
display "the number", number, "is", status`
};

function initPlayground() {
  const editor = document.getElementById('codeEditor');
  const terminal = document.getElementById('terminalOutput');
  const runBtn = document.getElementById('runCodeBtn');
  const clearBtn = document.getElementById('clearOutputBtn');
  const presetBtns = document.querySelectorAll('.preset-btn');

  // Load default preset
  if (editor && CODE_PRESETS.fibonacci) {
    editor.value = CODE_PRESETS.fibonacci;
  }

  // Preset buttons
  presetBtns.forEach(btn => {
    btn.addEventListener('click', () => {
      presetBtns.forEach(b => b.classList.remove('active'));
      btn.classList.add('active');
      const presetName = btn.getAttribute('data-preset');
      if (CODE_PRESETS[presetName]) {
        editor.value = CODE_PRESETS[presetName];
      }
    });
  });

  if (clearBtn) {
    clearBtn.addEventListener('click', () => {
      terminal.innerHTML = '<span class="term-dim">// Terminal cleared. Press Run Code to execute.</span>';
    });
  }

  if (runBtn) {
    runBtn.addEventListener('click', () => {
      executeEnlngInBrowser(editor.value, terminal);
    });
  }
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

    sandboxFunction(smartDisplay, smartDisplay, cat);

    if (outputLines.length === 0) {
      terminal.innerHTML = '<span class="term-dim">// Execution completed with 0 output statements</span>';
    } else {
      terminal.textContent = outputLines.join('\n');
    }
  } catch (err) {
    terminal.innerHTML = `<span class="term-err">Enlng Runtime Error: ${escapeHtml(err.message)}</span>`;
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
    title: 'Core Backend (.enlng)',
    desc: 'General-purpose algorithms, collections, math, OOP, exceptions, and I/O with 100% Python parity.',
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
    title: 'Frontend Markup & UI (.enlngf)',
    desc: 'Declarative component trees, props, stateful reactive hooks, and DOM events compiled directly to Web UI.',
    code: `type enlngf

component UserProfile with username, status:
    create container styled as "profile-card":
        create text username with style "heading-1"
        create badge status with style "status-online"
        create button "Message":
            on click: emit open_direct_message(username)`
  },
  enlngs: {
    title: 'Server & API Routes (.enlngs)',
    desc: 'Zero-overhead HTTP REST endpoints, WebSockets, microservices, and middleware routing engine.',
    code: `type enlngs

listen on port 8080

route get "/api/v1/health":
    respond with status 200 and json {"status": "healthy"}

route post "/api/v1/auth" with body payload:
    verify payload.token and respond with session`
  },
  enlngd: {
    title: 'Design Tokens & Styles (.enlngd)',
    desc: 'Sovereign design tokens, responsive grid/flex layouts, color matrices, and GPU-accelerated keyframe animations.',
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
    title: 'Mobile Apps & HAL (.enlngm)',
    desc: 'Hardware Abstraction Layer for Android (NDK/Vulkan) and iOS (Metal) with 120 FPS native activities.',
    code: `type enlngm

screen Dashboard:
    on device orientation change:
        realign layout to current orientation
        
    on hardware back button pressed:
        navigate back or prompt exit confirmation`
  },
  enlngdb: {
    title: 'Declarative Database (.enlngdb)',
    desc: 'Type-safe relational models, migration trees, and high-concurrency declarative queries with ACID compliance.',
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

  tabs.forEach(tab => {
    tab.addEventListener('click', () => {
      tabs.forEach(t => t.classList.remove('active'));
      tab.classList.add('active');
      const domainKey = tab.getAttribute('data-domain');
      const data = DOMAINS_DATA[domainKey];
      if (data && titleElem && descElem && codeElem) {
        titleElem.textContent = data.title;
        descElem.textContent = data.desc;
        codeElem.textContent = data.code;
      }
    });
  });
}

// --- 4. Interactive 3D Animated Book ---
function initAnimatedBook() {
  const bookStage = document.getElementById('bookStage');
  const previewBtn = document.getElementById('previewBookBtn');
  const hintText = document.querySelector('.open-hint');

  if (!bookStage) return;

  const toggleBook = (e) => {
    if (e && e.target && e.target.closest('a')) return;
    bookStage.classList.toggle('book-open');
    if (hintText) {
      if (bookStage.classList.contains('book-open')) {
        hintText.textContent = 'Click to Close ✕';
      } else {
        hintText.textContent = 'Hover / Click to Open →';
      }
    }
  };

  bookStage.addEventListener('click', toggleBook);

  if (previewBtn) {
    previewBtn.addEventListener('click', (e) => {
      e.preventDefault();
      toggleBook();
    });
  }
}

