// =====================================================================
//   Enlangg Official Website - Interactive Engine & In-Browser Runner
// =====================================================================

document.addEventListener('DOMContentLoaded', () => {
  initInstallerTabs();
  initPlayground();
  initDomainTabs();
});

// --- 1. Multi-OS Installer Tabs & Clipboard Copy ---
const INSTALL_COMMANDS = {
  powershell: 'powershell -ExecutionPolicy ByPass -c "irm https://raw.githubusercontent.com/Aero99op/enlang-main/main/install.ps1 | iex"',
  cmd: 'curl -fsSL https://raw.githubusercontent.com/Aero99op/enlang-main/main/install.cmd -o install.cmd && install.cmd',
  bash: 'curl -fsSL https://raw.githubusercontent.com/Aero99op/enlang-main/main/install.sh | bash'
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
        // Fallback for older browsers
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

create a n of 10
create a first of 0
create a second of 1
create a count of 0

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
      terminal.innerHTML = '<span class="term-dim">Terminal cleared. Ready to execute.</span>';
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
      terminal.innerHTML = '<span class="term-dim">[Execution completed with 0 output statements]</span>';
    } else {
      terminal.textContent = outputLines.join('\n');
    }
  } catch (err) {
    terminal.innerHTML = `<span class="term-err">Enlng Runtime Error: ${escapeHtml(err.message)}</span>`;
  }
}

function transpileEnlngToJS(lines) {
  const jsLines = [];
  
  for (let line of lines) {
    const indentLen = line.length - line.trimStart().length;
    const indent = ' '.repeat(indentLen);
    let trimmed = line.trim();

    if (!trimmed || trimmed.startsWith('#')) {
      jsLines.push(`${indent}// ${trimmed.replace(/^#\s*/, '')}`);
      continue;
    }

    if (trimmed.startsWith('type ') || trimmed.startsWith('hint ')) {
      jsLines.push(`${indent}// ${trimmed}`);
      continue;
    }

    // Replace English operators
    trimmed = trimmed.replace(/\bis equal to\b/gi, '===');
    trimmed = trimmed.replace(/\bis not equal to\b/gi, '!==');
    trimmed = trimmed.replace(/\bis at least\b/gi, '>=');
    trimmed = trimmed.replace(/\bis at most\b/gi, '<=');
    trimmed = trimmed.replace(/\bis greater than or equal to\b/gi, '>=');
    trimmed = trimmed.replace(/\bis less than or equal to\b/gi, '<=');
    trimmed = trimmed.replace(/\bis greater than\b/gi, '>');
    trimmed = trimmed.replace(/\bis less than\b/gi, '<');
    trimmed = trimmed.replace(/\bgreater than\b/gi, '>');
    trimmed = trimmed.replace(/\bless than\b/gi, '<');
    trimmed = trimmed.replace(/\bequal to\b/gi, '===');
    trimmed = trimmed.replace(/\bequals\b/gi, '===');
    trimmed = trimmed.replace(/\bmultiplied by\b/gi, '*');
    trimmed = trimmed.replace(/\bdivided by\b/gi, '/');
    trimmed = trimmed.replace(/\bplus\b/gi, '+');
    trimmed = trimmed.replace(/\bminus\b/gi, '-');
    trimmed = trimmed.replace(/\b(mod|modulo|modulus|modulous|modoulous)\b/gi, '%');
    trimmed = trimmed.replace(/\b(?:length of|count of)\s+([a-zA-Z0-9_\[\]]+)/gi, '$1.length');

    // Display statement
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

      // Convert '+' touching strings to cat(...)
      if (body.includes('+') && (body.includes('"') || body.includes("'"))) {
        // check string concat
        const parts = body.split(',').map(p => {
          if (p.includes('+')) {
            const sumParts = p.split('+').map(x => x.trim());
            return `cat(${sumParts.join(', ')})`;
          }
          return p.trim();
        });
        jsLines.push(`${indent}smartDisplay(${parts.join(', ')}${sepArg});`);
        continue;
      }

      // Auto comma for comma-less space-separated tokens: "the square of" num "is" sq
      if (!body.includes(',')) {
        body = body.replace(/("[^"]*"|'[^']*')\s+([a-zA-Z0-9_\(\[\{])/g, '$1, $2');
        body = body.replace(/([a-zA-Z0-9_\]\)\}])\s+("[^"]*"|'[^']*')/g, '$1, $2');
        body = body.replace(/("[^"]*"|'[^']*')\s+("[^"]*"|'[^']*')/g, '$1, $2');
      }

      jsLines.push(`${indent}smartDisplay(${body}${sepArg});`);
      continue;
    }

    // Loops (while)
    const whileMatch = trimmed.match(/^(?:while|repeat\s+while)\s+(.*?):$/i);
    if (whileMatch) {
      jsLines.push(`${indent}while (${whileMatch[1]}) {`);
      continue;
    }

    // Conditionals (if / elif / else)
    const ifMatch = trimmed.match(/^(?:if|when)\s+(.*?):$/i);
    if (ifMatch) {
      jsLines.push(`${indent}if (${ifMatch[1]}) {`);
      continue;
    }

    const elifMatch = trimmed.match(/^(?:else\s+if|elif)\s+(.*?):$/i);
    if (elifMatch) {
      jsLines.push(`${indent}} else if (${elifMatch[1]}) {`);
      continue;
    }

    if (/^else:$/i.test(trimmed)) {
      jsLines.push(`${indent}} else {`);
      continue;
    }

    // Variable Declarations (create / declare / initialize / let)
    const createMatch = trimmed.match(/^(?:create\s+(?:a\s+|an\s+|the\s+)?|declare\s+|initialize\s+|let\s+)([a-zA-Z0-9_]+)\s+(?:of|as|to|=)\s+(.*)$/i);
    if (createMatch) {
      jsLines.push(`${indent}let ${createMatch[1]} = ${createMatch[2]};`);
      continue;
    }

    // Variable Assignments (set / update / assign)
    const setMatch = trimmed.match(/^(?:set|update|assign)\s+([a-zA-Z0-9_\[\]\.]+)\s+(?:to|=)\s+(.*)$/i);
    if (setMatch) {
      jsLines.push(`${indent}${setMatch[1]} = ${setMatch[2]};`);
      continue;
    }

    // Direct assignment: a = b
    if (/^[a-zA-Z0-9_\[\]\.]+\s*=/.test(trimmed)) {
      jsLines.push(`${indent}${trimmed};`);
      continue;
    }

    jsLines.push(`${indent}${trimmed};`);
  }

  // Handle block closes based on indentation
  const structuredJS = [];
  const indentStack = [0];

  for (let l of jsLines) {
    const rawIndent = l.search(/\S/);
    if (rawIndent === -1) {
      structuredJS.push(l);
      continue;
    }

    while (indentStack.length > 1 && rawIndent < indentStack[indentStack.length - 1]) {
      indentStack.pop();
      structuredJS.push(' '.repeat(indentStack[indentStack.length - 1]) + '}');
    }

    if (l.trimEnd().endsWith('{')) {
      indentStack.push(rawIndent + 4);
    }

    structuredJS.push(l);
  }

  while (indentStack.length > 1) {
    indentStack.pop();
    structuredJS.push(' '.repeat(indentStack[indentStack.length - 1]) + '}');
  }

  return structuredJS.join('\n');
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
    display "palindrome"`
  },
  enlngf: {
    title: 'Frontend Markup & UI (.enlngf)',
    desc: 'Declarative component trees, props, stateful reactive hooks, and DOM events compiled directly to Web UI.',
    code: `type enlngf

component UserCard with name, role:
    create container styled as "card":
        create text name with style "bold title"
        create text role with style "muted subtitle"
        create button "Follow":
            on click: emit follow_event(name)`
  },
  enlngs: {
    title: 'Server & API Routes (.enlngs)',
    desc: 'Zero-overhead HTTP REST endpoints, WebSockets, microservices, and middleware routing engine.',
    code: `type enlngs

listen on port 8080

route get "/api/users":
    fetch all users from database
    respond with status 200 and json users

route post "/api/login" with body credentials:
    verify credentials and return jwt token`
  },
  enlngd: {
    title: 'Design Tokens & Styles (.enlngd)',
    desc: 'Sovereign design tokens, responsive grid/flex layouts, color matrices, and GPU-accelerated keyframe animations.',
    code: `type enlngd

define theme dark_mode:
    primary_color is #00f0ff
    background is #07090e
    card_surface is rgba(15, 23, 42, 0.8)

style class "card":
    background is card_surface
    border_radius is 14px
    backdrop_blur is 12px`
  },
  enlngm: {
    title: 'Mobile Apps & HAL (.enlngm)',
    desc: 'Hardware Abstraction Layer for Android (NDK/Vulkan) and iOS (Metal) with 120 FPS native activities.',
    code: `type enlngm

screen MainFeed:
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
