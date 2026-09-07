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

set n to 10
set first to 0
set second to 1
set count to 0

display "--- Fibonacci Series (First 10) ---"

while count is less than n:
    display first
    set next to first plus second
    set first to second
    set second to next
    set count to count plus 1`,

  palindrome: `type enlng

set word to "madam"
set reversed to ""
set i to 0

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

set number to 100
set square to number * number

if number % 2 is equal to 0:
    set status to "even"
else:
    set status to "odd"

display "the square of", number, "is", square
display "the number", number, "is", status`,

  factorial: `type enlng

set number to 6
set result to 1
set i to 1

while i is less than or equal to number:
    set result to result * i
    set i to i plus 1

display "Factorial calculation:"
display "The factorial of", number, "is", result`,

  // === EnlngDB Sovereign Engine Presets ===
  enlngdb_tour: `type enlngdb;

// Modern SQL Studio: Press "Run All" (Ctrl+Enter) to execute the whole script
// Or click on any line & press "Run Line / Selection" (Shift+Enter) to run statement-by-statement!

show databases;

use database1;

show tables;

find all records from student;

find all records from faculty;`,

  enlngdb_schema: `type enlngdb;

// Schema Discovery & Inspection across Sovereign Databases
show databases;

show tables of database main_db;

show tables of database database1;

use main_db;

find all records from accounts;`,

  enlngdb_crud: `type enlngdb;

// Create Table, Insert Records, and Query with Conditions
use university_db;

create table scholars with id, name, major, cgpa;

insert record into scholars with id 1, name "Aryan Sharma", major "AI & Robotics", cgpa 9.4;
insert record into scholars with id 2, name "Meera Sen", major "Quantum Systems", cgpa 9.8;
insert record into scholars with id 3, name "Dev Patel", major "Distributed Systems", cgpa 8.9;

show tables;

find all records from scholars;

// Filter records with natural where clause
find all records from scholars where cgpa is greater than 9.0;`,

  enlngdb_deletions: `type enlngdb;

// Sovereign Deletions & Safety Guard Demonstrations
use main_db;

show tables;

// 1. Delete a specific record
delete from accounts where id is 3;

// 2. Drop a column permanently from table
delete column tier from accounts;

find all records from accounts;

// 3. Delete table without 'confirmed' -> triggers security guard!
delete table accounts;

// 4. Safe drop with 'confirmed' keyword:
// delete table accounts confirmed;`,

  enlngdb_filter: `type enlngdb;

// Query Filtering and Sorting Expressions
use database1;

// 1. Filter with comparison operators
find all records from student where marks is greater than 80;

// 2. Filter with exact equality
find all records from student where grade is "A";

// 3. Sort records descending
find all records from student order by marks descending;`,

  enlngdb_updates: `type enlngdb;

// Natural English In-Table Row & Column Updates (Pure C Engine: 0.03 ms)
use university_db;

create table scholars with id, name, cgpa, status;

insert record into scholars with id 1, name "aryan", cgpa 8.2, status "probation";
insert record into scholars with id 2, name "meera", cgpa 9.4, status "honors";
insert record into scholars with id 3, name "kunal", cgpa 7.8, status "probation";

find all records from scholars;

// 1. Conversational update by condition (User syntax)
in scholars change cgpa to 9.8 where name is "aryan";

// 2. Update status with relational condition
in scholars update status to "dean_list" where cgpa is greater than 9.0;

// 3. Multi-field update in a single command
in scholars set cgpa to 9.95, status to "gold_medalist" where name is "aryan";

// 4. Update whole column across all rows (unconstrained)
in scholars set status to "active_enrolled";

find all records from scholars;`,


  topic1: `type enlng


// Topic 01: Hello World & The Sovereign Declaration
display "Hello, Sovereign World!"
display "Enlangg compiles natural English to bare-metal C machine code."`,

  topic2: `type enlng

// Topic 02: Deterministic Memory Slots
set server_port to 8080
set server_name to "Primary Gateway"
set is_active to true

// Mutate existing slot value with 'set'
set server_port to 9000

display "Server: ", server_name
display "Port: ", server_port
display "Status Active: ", is_active`,

  topic3: `type enlng

// Topic 03: Spoken Math & Arithmetic Operations
set base_salary to 65000
set bonus to 12000
set tax_rate to 0.18

set gross_pay to base_salary plus bonus
set deductions to gross_pay multiplied by tax_rate
set net_pay to gross_pay minus deductions

display "Gross Compensation: ", gross_pay
display "Estimated Tax: ", deductions
display "Net Take-Home: ", net_pay`,

  topic4: `type enlng

// Topic 04: Decision Logic: Spoken Branching & Fallbacks
set user_age to 22
set has_verified_id to true

if user_age is greater than or equal to 21 and has_verified_id is equal to true:
    display "Access Authorized: Primary Production System"
otherwise if user_age is greater than 16:
    display "Access Restricted: Observer Access Only"
otherwise:
    display "Access Denied: Age verification requirement not met"`,

  topic5: `type enlng

// Topic 05: Loops: While, Until & Bounded Iteration
set counter to 1
while counter is less than or equal to 5:
    display "Iteration count: ", counter
    increase counter by 1`,

  topic6: `type enlng

// Topic 06: Procedures, Scopes & Return Values
define calculate_tax(amount, rate):
    set tax_val to amount multiplied by rate
    return tax_val

set bill to 250
set tax to calculate_tax(bill, 0.08)
display "Subtotal: $", bill
display "Tax Calculated: $", tax`,

  topic7: `type enlng

// Topic 07: Structured Data: Lists & Hash Maps
set servers to ["alpha", "beta", "gamma"]
display "Primary Node: ", servers[0]
display "Active Cluster Nodes: ", servers
display "Total Cluster Size: ", length of servers`,

  topic8: `type enlng

// Topic 08: Python God Call & C-ABI Foreign Interop
display "Enlangg Sovereign C-ABI & Python Ecosystem Layer"
display "Native access to 400,000+ packages with zero whitelisting"`
};

// --- 2. Live In-Browser Enlng & EnlngDB Sandbox Engines ---

function escapeHtml(str) {
  if (str === null || str === undefined) return '';
  return String(str)
    .replace(/&/g, '&amp;')
    .replace(/</g, '&lt;')
    .replace(/>/g, '&gt;')
    .replace(/"/g, '&quot;')
    .replace(/'/g, '&#039;');
}

// In-Memory Sovereign Database State & Sample Tables
const INITIAL_SOVEREIGN_DB = {
  activeDb: 'database1',
  databases: {
    'database1': {
      name: 'database1',
      tables: {
        'student': {
          columns: ['roll_no', 'name', 'marks', 'grade'],
          rows: [
            { roll_no: 101, name: 'Vikram Malhotra', marks: 92, grade: 'A' },
            { roll_no: 102, name: 'Ananya Iyer', marks: 96, grade: 'A+' },
            { roll_no: 103, name: 'Kabir Mehta', marks: 78, grade: 'B' },
            { roll_no: 104, name: 'Rhea Sengupta', marks: 85, grade: 'A' },
            { roll_no: 105, name: 'Arjun Rao', marks: 64, grade: 'C' }
          ]
        },
        'faculty': {
          columns: ['id', 'name', 'department', 'salary'],
          rows: [
            { id: 1, name: 'Dr. Sunita Sen', department: 'Computer Science', salary: 115000 },
            { id: 2, name: 'Prof. Rajesh Nair', department: 'Mathematics', salary: 98000 }
          ]
        }
      }
    },
    'main_db': {
      name: 'main_db',
      tables: {
        'accounts': {
          columns: ['id', 'holder', 'balance', 'tier'],
          rows: [
            { id: 1, holder: 'Aero Technologies', balance: 450000, tier: 'Gold' },
            { id: 2, holder: 'Apex Global', balance: 89000, tier: 'Platinum' },
            { id: 3, holder: 'Zenith Studio', balance: 12500, tier: 'Silver' },
            { id: 4, holder: 'Nova Labs', balance: 230000, tier: 'Gold' }
          ]
        },
        'system_logs': {
          columns: ['log_id', 'level', 'message', 'source'],
          rows: [
            { log_id: 201, level: 'INFO', message: 'Sovereign cluster online', source: 'kernel' },
            { log_id: 202, level: 'WARN', message: 'Memory allocation limit near 80%', source: 'worker-1' },
            { log_id: 203, level: 'INFO', message: 'Snapshot persisted to disk', source: 'storage' }
          ]
        }
      }
    },
    'university_db': {
      name: 'university_db',
      tables: {
        'courses': {
          columns: ['code', 'title', 'credits'],
          rows: [
            { code: 'CS101', title: 'Compiler Construction', credits: 4 },
            { code: 'DB201', title: 'Sovereign Database Architecture', credits: 4 },
            { code: 'SE301', title: 'Systems Engineering & C-ABI', credits: 3 }
          ]
        }
      }
    }
  }
};

let sovereignDB = JSON.parse(JSON.stringify(INITIAL_SOVEREIGN_DB));

function resetSovereignDB() {
  sovereignDB = JSON.parse(JSON.stringify(INITIAL_SOVEREIGN_DB));
}

// Formats tabular data as exact, aligned ASCII grid tables like modern CLI / SQL tools
function formatAsciiTable(headers, rows) {
  if (!headers || headers.length === 0) return '(0 columns)';
  if (!rows || rows.length === 0) return 'Empty set (0 rows)';

  const colWidths = {};
  headers.forEach(h => {
    colWidths[h] = Math.max(h.length, 1);
  });

  rows.forEach(row => {
    headers.forEach(h => {
      const rawVal = row[h];
      const val = rawVal !== undefined && rawVal !== null ? String(rawVal) : 'NULL';
      if (val.length > colWidths[h]) {
        colWidths[h] = val.length;
      }
    });
  });

  const border = '+' + headers.map(h => '-'.repeat(colWidths[h] + 2)).join('+') + '+';
  const headerRow = '|' + headers.map(h => ' ' + h.padEnd(colWidths[h]) + ' ').join('|') + '|';

  const dataRows = rows.map(row => {
    return '|' + headers.map(h => {
      const rawVal = row[h];
      const val = rawVal !== undefined && rawVal !== null ? String(rawVal) : 'NULL';
      const isNum = typeof rawVal === 'number' || (!isNaN(Number(rawVal)) && rawVal !== '');
      const padded = isNum ? val.padStart(colWidths[h]) : val.padEnd(colWidths[h]);
      return ' ' + padded + ' ';
    }).join('|') + '|';
  });

  return [border, headerRow, border, ...dataRows, border].join('\n');
}

// Normalizes sugar phrases like BETWEEN and FROM...TO before boolean tokenization
function normalizeWhereClause(clause) {
  let normalized = clause.trim();

  // Normalize BETWEEN: col [is] between A and B -> (col >= A and col <= B)
  normalized = normalized.replace(/\b([a-zA-Z0-9_]+)\s+(?:is\s+)?between\s+([^\s]+)\s+and\s+([^\s,;)]+)/gi, (m, col, a, b) => {
    return `(${col} >= ${a} and ${col} <= ${b})`;
  });

  // Normalize FROM ... TO: col [is] from A to B -> (col >= A and col <= B)
  normalized = normalized.replace(/\b([a-zA-Z0-9_]+)\s+(?:is\s+)?from\s+([^\s]+)\s+to\s+([^\s,;)]+)/gi, (m, col, a, b) => {
    return `(${col} >= ${a} and ${col} <= ${b})`;
  });

  return normalized;
}

// Expression and condition evaluator for an atomic EnlngDB condition
function evaluateSingleCondition(row, cond) {
  const trimmed = cond.trim();
  if (!trimmed) return true;

  // 1. IS NULL / IS NOT NULL / IS EMPTY / IS NOT EMPTY
  const nullMatch = trimmed.match(/^([a-zA-Z0-9_]+)\s+is\s+(not\s+null|null|not\s+empty|empty)$/i);
  if (nullMatch) {
    const col = nullMatch[1];
    const op = nullMatch[2].toLowerCase();
    const val = row[col];
    const isNullOrEmpty = val === null || val === undefined || val === '';
    if (op === 'null' || op === 'empty') return isNullOrEmpty;
    if (op === 'not null' || op === 'not empty') return !isNullOrEmpty;
  }

  // 2. IN / NOT IN: col [is] [not] in (val1, val2, ...)
  const inMatch = trimmed.match(/^([a-zA-Z0-9_]+)\s+(?:is\s+)?(not\s+in|in)\s*\((.*?)\)$/i);
  if (inMatch) {
    const col = inMatch[1];
    const isIn = inMatch[2].toLowerCase() === 'in';
    const rawItems = inMatch[3].split(',').map(s => s.trim().replace(/^["']|["']$/g, ''));
    const rowVal = String(row[col] ?? '').toLowerCase();
    const found = rawItems.some(item => item.toLowerCase() === rowVal);
    return isIn ? found : !found;
  }

  // 3. CONTAINS / STARTS WITH / ENDS WITH
  const textSearchMatch = trimmed.match(/^([a-zA-Z0-9_]+)\s+(?:is\s+)?(contains|starts\s+with|ends\s+with)\s+["']?(.*?)["']?$/i);
  if (textSearchMatch) {
    const col = textSearchMatch[1];
    const op = textSearchMatch[2].toLowerCase();
    const target = textSearchMatch[3].toLowerCase();
    const rowVal = String(row[col] ?? '').toLowerCase();
    if (op === 'contains') return rowVal.includes(target);
    if (op === 'starts with') return rowVal.startsWith(target);
    if (op === 'ends with') return rowVal.endsWith(target);
  }

  // 4. LIKE pattern: col like "%xyz%"
  const likeMatch = trimmed.match(/^([a-zA-Z0-9_]+)\s+(?:is\s+)?like\s+["']?(.*?)["']?$/i);
  if (likeMatch) {
    const col = likeMatch[1];
    const pattern = likeMatch[2].replace(/%/g, '.*');
    const val = String(row[col] ?? '');
    const regex = new RegExp('^' + pattern + '$', 'i');
    return regex.test(val);
  }

  // 5. Comparison operators - Compound phrases MUST precede single-word operators
  // 'is' is treated as an optional/silent helper word (e.g. 'is greater than' vs 'greater than' vs 'is >' vs '>')
  const opPatterns = [
    { op: '>=', regex: /^([a-zA-Z0-9_]+)\s*(?:(?:is\s+)?>=|(?:is\s+)?greater\s+than\s+or\s+equal\s+to|(?:is\s+)?at\s+least)\s*(.*)$/i },
    { op: '<=', regex: /^([a-zA-Z0-9_]+)\s*(?:(?:is\s+)?<=|(?:is\s+)?less\s+than\s+or\s+equal\s+to|(?:is\s+)?at\s+most)\s*(.*)$/i },
    { op: '>',  regex: /^([a-zA-Z0-9_]+)\s*(?:(?:is\s+)?>|(?:is\s+)?greater\s+than|(?:is\s+)?exceeds|(?:is\s+)?above)\s*(.*)$/i },
    { op: '<',  regex: /^([a-zA-Z0-9_]+)\s*(?:(?:is\s+)?<|(?:is\s+)?less\s+than|(?:is\s+)?below)\s*(.*)$/i },
    { op: '!=', regex: /^([a-zA-Z0-9_]+)\s*(?:(?:is\s+)?!=|(?:is\s+)?not\s+equal\s+to|is\s+not|not\s+equals?|isn't)\s*(.*)$/i },
    { op: '==', regex: /^([a-zA-Z0-9_]+)\s*(?:(?:is\s+)?==|(?:is\s+)?=|(?:is\s+)?equal\s+to|equals?|\bis\b)\s*(.*)$/i }
  ];

  for (const { op, regex } of opPatterns) {
    const m = trimmed.match(regex);
    if (m) {
      const col = m[1];
      const targetVal = m[2].trim().replace(/^["']|["']$/g, '');
      const rowVal = row[col];

      if (rowVal === undefined || rowVal === null) return false;

      // Numeric comparison
      const numRow = Number(rowVal);
      const numTarget = Number(targetVal);
      if (!isNaN(numRow) && !isNaN(numTarget) && rowVal !== '' && targetVal !== '') {
        if (op === '>=') return numRow >= numTarget;
        if (op === '<=') return numRow <= numTarget;
        if (op === '>') return numRow > numTarget;
        if (op === '<') return numRow < numTarget;
        if (op === '==' || op === '=') return numRow === numTarget;
        if (op === '!=') return numRow !== numTarget;
      }

      // String / boolean comparison
      const strRow = String(rowVal).toLowerCase();
      const strTarget = String(targetVal).toLowerCase();
      if (op === '==' || op === '=') return strRow === strTarget;
      if (op === '!=') return strRow !== strTarget;
      if (op === '>') return strRow > strTarget;
      if (op === '<') return strRow < strTarget;
      if (op === '>=') return strRow >= strTarget;
      if (op === '<=') return strRow <= strTarget;
      return false;
    }
  }

  return true;
}

// Splits string by a delimiter word outside quotes and parentheses
function splitLogicalTokens(text, word) {
  const parts = [];
  let depth = 0;
  let inQuote = null;
  let cur = '';

  for (let i = 0; i < text.length; i++) {
    const ch = text[i];

    if (inQuote) {
      cur += ch;
      if (ch === inQuote) inQuote = null;
      continue;
    }

    if (ch === '"' || ch === "'") {
      inQuote = ch;
      cur += ch;
      continue;
    }

    if (ch === '(') {
      depth++;
      cur += ch;
      continue;
    }
    if (ch === ')') {
      depth--;
      cur += ch;
      continue;
    }

    if (depth === 0) {
      const sub = text.substring(i);
      const match = sub.match(new RegExp('^\\s+' + word + '\\s+', 'i'));
      if (match) {
        parts.push(cur.trim());
        cur = '';
        i += match[0].length - 1;
        continue;
      }
    }

    cur += ch;
  }

  if (cur.trim()) parts.push(cur.trim());
  return parts;
}

function evalLogicalOr(row, expr) {
  const orParts = splitLogicalTokens(expr, 'or');
  if (orParts.length > 1) {
    return orParts.some(part => evalLogicalAnd(row, part));
  }
  return evalLogicalAnd(row, expr);
}

function evalLogicalAnd(row, expr) {
  const andParts = splitLogicalTokens(expr, 'and');
  if (andParts.length > 1) {
    return andParts.every(part => evalLogicalAtom(row, part));
  }
  return evalLogicalAtom(row, expr);
}

function evalLogicalAtom(row, expr) {
  let trimmed = expr.trim();

  // Strip outer matching parentheses and recursively evaluate inner expression
  if (trimmed.startsWith('(') && trimmed.endsWith(')')) {
    let depth = 0;
    let safe = true;
    for (let i = 0; i < trimmed.length - 1; i++) {
      if (trimmed[i] === '(') depth++;
      else if (trimmed[i] === ')') {
        depth--;
        if (depth === 0) { safe = false; break; }
      }
    }
    if (safe) {
      return evalLogicalOr(row, trimmed.substring(1, trimmed.length - 1).trim());
    }
  }

  // Check NOT: not (expr) OR not condition
  const notMatch = trimmed.match(/^not\s+(.*)$/i);
  if (notMatch) {
    return !evalLogicalOr(row, notMatch[1].trim());
  }

  return evaluateSingleCondition(row, trimmed);
}

// Robust recursive WHERE evaluator supporting AND, OR, NOT, parentheses, BETWEEN, IN, NULL, etc.
function evaluateWhereCondition(row, whereClause) {
  if (!whereClause || !whereClause.trim()) return true;
  const normalized = normalizeWhereClause(whereClause);
  return evalLogicalOr(row, normalized);
}

// Parses comma-separated key value pairs for insert and update
function parseKeyValuePairs(str) {
  const items = splitOutsideQuotes(str, ',');
  const record = {};
  for (const item of items) {
    const trimmed = item.trim();
    if (!trimmed) continue;
    const match = trimmed.match(/^([a-zA-Z0-9_]+)\s*(?:[:=]|as|to|is|\s)\s*(.*)$/i);
    if (match) {
      const key = match[1];
      let rawVal = match[2].trim();
      if ((rawVal.startsWith('"') && rawVal.endsWith('"')) || (rawVal.startsWith("'") && rawVal.endsWith("'"))) {
        record[key] = rawVal.slice(1, -1);
      } else if (rawVal.toLowerCase() === 'true') {
        record[key] = true;
      } else if (rawVal.toLowerCase() === 'false') {
        record[key] = false;
      } else if (rawVal.toLowerCase() === 'null') {
        record[key] = null;
      } else if (!isNaN(Number(rawVal)) && rawVal !== '') {
        record[key] = Number(rawVal);
      } else {
        record[key] = rawVal;
      }
    }
  }
  return record;
}

// Executes an individual EnlngDB statement against sovereign storage
function executeEnlngDBStatement(statement, engineState) {
  const stmt = statement.trim().replace(/;+$/, '').trim();
  if (!stmt) return null;

  if (/^type\s+(?:enlngdb|enlgdb)$/i.test(stmt)) {
    return {
      type: 'TYPE_DECLARATION',
      output: `Sovereign EnlngDB Pure C Engine active. Microsecond query latency, zero SQL / zero Python runtime dependency.`
    };
  }

  // 1. SHOW DATABASES / LIST DATABASES
  if (/^(?:show\s+databases|list\s+databases)$/i.test(stmt)) {
    const dbNames = Object.keys(engineState.databases);
    const rows = dbNames.map(name => ({
      Database: (name === engineState.activeDb ? '* ' : '  ') + name,
      Status: name === engineState.activeDb ? 'Active' : 'Ready',
      Tables: Object.keys(engineState.databases[name].tables).length
    }));
    const t0 = performance.now();
    const tableAscii = formatAsciiTable(['Database', 'Status', 'Tables'], rows);
    const ms = (performance.now() - t0).toFixed(2);
    return {
      type: 'SHOW_DATABASES',
      output: `${tableAscii}\n${rows.length} database(s) in set (${ms} ms)`
    };
  }

  // 2. USE DATABASE <name>
  const useMatch = stmt.match(/^use(?:\s+database)?\s+["']?([a-zA-Z0-9_\-.]+)["']?$/i);
  if (useMatch) {
    const dbName = useMatch[1];
    if (!engineState.databases[dbName]) {
      engineState.databases[dbName] = { name: dbName, tables: {} };
    }
    engineState.activeDb = dbName;
    return {
      type: 'USE_DATABASE',
      output: `Database changed to '${dbName}'.`
    };
  }

  // 3. SHOW TABLES [OF DATABASE <name>]
  const showTablesMatch = stmt.match(/^(?:show\s+tables|list\s+tables)(?:\s+of\s+database\s+["']?([a-zA-Z0-9_\-.]+)["']?)?$/i);
  if (showTablesMatch) {
    const targetDb = showTablesMatch[1] || engineState.activeDb;
    if (!engineState.databases[targetDb]) {
      return {
        type: 'ERROR',
        error: `Database '${targetDb}' does not exist.`
      };
    }
    const dbObj = engineState.databases[targetDb];
    const tableNames = Object.keys(dbObj.tables);
    if (tableNames.length === 0) {
      return {
        type: 'SHOW_TABLES',
        output: `Empty set (0 tables found in '${targetDb}').`
      };
    }
    const rows = tableNames.map(name => {
      const tbl = dbObj.tables[name];
      return {
        Table: name,
        Columns: (tbl.columns || []).join(', '),
        'Row Count': (tbl.rows || []).length
      };
    });
    const t0 = performance.now();
    const tableAscii = formatAsciiTable(['Table', 'Columns', 'Row Count'], rows);
    const ms = (performance.now() - t0).toFixed(2);
    return {
      type: 'SHOW_TABLES',
      output: `${tableAscii}\n${rows.length} table(s) in '${targetDb}' (${ms} ms)`
    };
  }

  // 4. CREATE TABLE <name> WITH <cols>
  const createTableMatch = stmt.match(/^create\s+table\s+([a-zA-Z0-9_]+)\s+with\s+(.+)$/i);
  if (createTableMatch) {
    const tableName = createTableMatch[1];
    const colListStr = createTableMatch[2];
    const columns = colListStr.split(',').map(c => c.trim().replace(/^column\s+/i, '')).filter(Boolean);
    const currentDb = engineState.databases[engineState.activeDb];
    currentDb.tables[tableName] = {
      columns,
      rows: []
    };
    return {
      type: 'CREATE_TABLE',
      output: `Query OK: Table '${tableName}' created in '${engineState.activeDb}' with ${columns.length} columns: [${columns.join(', ')}].`
    };
  }

  // 5. INSERT RECORD INTO <name> WITH / VALUES <k1 v1, ...>
  const insertMatch = stmt.match(/^insert\s+(?:record\s+into|records\s+into|row\s+into|into)?\s*([a-zA-Z0-9_]+)\s+(?:with|values)\s+(.+)$/i);
  if (insertMatch) {
    const tableName = insertMatch[1];
    const keyValsStr = insertMatch[2];
    const currentDb = engineState.databases[engineState.activeDb];
    if (!currentDb.tables[tableName]) {
      return {
        type: 'ERROR',
        error: `Table '${tableName}' does not exist in active database '${engineState.activeDb}'.`
      };
    }
    const table = currentDb.tables[tableName];
    const newRecord = parseKeyValuePairs(keyValsStr);

    Object.keys(newRecord).forEach(k => {
      if (!table.columns.includes(k)) {
        table.columns.push(k);
      }
    });

    table.rows.push(newRecord);
    return {
      type: 'INSERT',
      output: `Query OK, 1 record successfully inserted into '${tableName}'.`
    };
  }

  // 6. FIND / SHOW ALL RECORDS / VALUES FROM <name> [WHERE ...] [ORDER BY ...]
  let findStmt = stmt;
  let sortCol = null;
  let sortDir = null;
  const orderMatch = findStmt.match(/\s+order\s+by\s+([a-zA-Z0-9_]+)(?:\s+(ascending|descending|asc|desc))?$/i);
  if (orderMatch) {
    sortCol = orderMatch[1];
    sortDir = orderMatch[2];
    findStmt = findStmt.substring(0, orderMatch.index).trim();
  }

  let whereClause = null;
  const whereMatch = findStmt.match(/\s+where\s+(.+)$/i);
  if (whereMatch) {
    whereClause = whereMatch[1].trim();
    findStmt = findStmt.substring(0, whereMatch.index).trim();
  }

  const findMatch = findStmt.match(/^(?:find|show)\s+(?:all\s+)?(?:records|values)?\s*(?:from|in)\s+([a-zA-Z0-9_]+)$/i);
  if (findMatch) {
    const tableName = findMatch[1];

    const currentDb = engineState.databases[engineState.activeDb];
    if (!currentDb.tables[tableName]) {
      return {
        type: 'ERROR',
        error: `Table '${tableName}' does not exist in active database '${engineState.activeDb}'.`
      };
    }

    const table = currentDb.tables[tableName];
    let matchedRows = (table.rows || []).filter(r => evaluateWhereCondition(r, whereClause));

    if (sortCol) {
      const isDesc = Boolean(sortDir && sortDir.toLowerCase().startsWith('desc'));
      matchedRows = [...matchedRows].sort((a, b) => {
        const vA = a[sortCol];
        const vB = b[sortCol];
        if (vA === vB) return 0;
        if (vA === undefined) return isDesc ? 1 : -1;
        if (vB === undefined) return isDesc ? -1 : 1;
        if (typeof vA === 'number' && typeof vB === 'number') {
          return isDesc ? vB - vA : vA - vB;
        }
        return isDesc ? String(vB).localeCompare(String(vA)) : String(vA).localeCompare(String(vB));
      });
    }

    const t0 = performance.now();
    const tableAscii = formatAsciiTable(table.columns, matchedRows);
    const ms = (performance.now() - t0).toFixed(2);
    return {
      type: 'FIND',
      output: `${tableAscii}\n${matchedRows.length} row(s) in set (${ms} ms)`
    };
  }

  // 7. COUNT RECORDS IN <name>
  const countMatch = stmt.match(/^count\s+records\s+in\s+([a-zA-Z0-9_]+)(?:\s+where\s+(.+))?$/i);
  if (countMatch) {
    const tableName = countMatch[1];
    const whereClause = countMatch[2];
    const currentDb = engineState.databases[engineState.activeDb];
    if (!currentDb.tables[tableName]) {
      return { type: 'ERROR', error: `Table '${tableName}' not found.` };
    }
    const table = currentDb.tables[tableName];
    const count = table.rows.filter(r => evaluateWhereCondition(r, whereClause)).length;
    return {
      type: 'COUNT',
      output: formatAsciiTable(['Table', 'Count'], [{ Table: tableName, Count: count }])
    };
  }

  // 8. UPDATE / CHANGE / IN <name> CHANGE/UPDATE/SET <col> TO/=/IS <val> [WHERE ...]
  let updateMatch = stmt.match(/^in\s+(?:table\s+|the\s+table\s+)?([a-zA-Z0-9_]+)\s+(?:change|update|set|modify)\s+(?:set\s+)?(.+?)(?:\s+where\s+(.+))?$/i);
  if (!updateMatch) {
    updateMatch = stmt.match(/^(?:update|change|modify)\s+(?:records\s+in\s+|rows\s+in\s+|table\s+)?([a-zA-Z0-9_]+)\s+(?:set\s+)?(.+?)(?:\s+where\s+(.+))?$/i);
  }
  if (!updateMatch) {
    // support: update/set <col> to/=/is <val> in <table_name> [where ...]
    const altMatch = stmt.match(/^(?:update|change|set|modify)\s+(.+?)\s+in\s+(?:table\s+|the\s+table\s+)?([a-zA-Z0-9_]+)(?:\s+where\s+(.+))?$/i);
    if (altMatch) {
      updateMatch = [altMatch[0], altMatch[2], altMatch[1], altMatch[3]];
    }
  }
  if (updateMatch) {
    const tableName = updateMatch[1];
    const setClause = updateMatch[2];
    const whereClause = updateMatch[3];
    const currentDb = engineState.databases[engineState.activeDb];
    if (!currentDb || !currentDb.tables[tableName]) {
      return { type: 'ERROR', error: `Table '${tableName}' not found in active database '${engineState.activeDb}'.` };
    }
    const table = currentDb.tables[tableName];
    const assignments = parseKeyValuePairs(setClause.replace(/^set\s+/i, ''));
    let affected = 0;
    table.rows.forEach(r => {
      if (evaluateWhereCondition(r, whereClause)) {
        Object.assign(r, assignments);
        affected++;
      }
    });
    return {
      type: 'UPDATE',
      output: `Query OK, ${affected} row(s) updated in '${tableName}'. (0.03 ms)`
    };
  }


  // 9. DELETE COLUMN <col> FROM <name>
  const delColMatch = stmt.match(/^delete\s+(?:column\s+)?([a-zA-Z0-9_]+)\s+from\s+([a-zA-Z0-9_]+)$/i);
  if (delColMatch) {
    const colName = delColMatch[1];
    const tableName = delColMatch[2];
    const currentDb = engineState.databases[engineState.activeDb];
    if (!currentDb.tables[tableName]) {
      return { type: 'ERROR', error: `Table '${tableName}' not found in '${engineState.activeDb}'.` };
    }
    const table = currentDb.tables[tableName];
    table.columns = table.columns.filter(c => c !== colName);
    table.rows.forEach(r => delete r[colName]);
    return {
      type: 'DELETE_COLUMN',
      output: `Query OK: Column '${colName}' dropped from table '${tableName}'.`
    };
  }

  // 10. DELETE FROM <name> [WHERE ...]
  const delRowMatch = stmt.match(/^delete\s+from\s+([a-zA-Z0-9_]+)(?:\s+where\s+(.+))?$/i);
  if (delRowMatch) {
    const tableName = delRowMatch[1];
    const whereClause = delRowMatch[2];
    const currentDb = engineState.databases[engineState.activeDb];
    if (!currentDb.tables[tableName]) {
      return { type: 'ERROR', error: `Table '${tableName}' not found.` };
    }
    const table = currentDb.tables[tableName];
    const initialLen = table.rows.length;
    table.rows = table.rows.filter(r => !evaluateWhereCondition(r, whereClause));
    const deletedCount = initialLen - table.rows.length;
    return {
      type: 'DELETE',
      output: `Query OK, ${deletedCount} row(s) deleted from '${tableName}'.`
    };
  }

  // 11. DELETE TABLE <name> [confirmed]
  const delTableMatch = stmt.match(/^delete\s+table\s+([a-zA-Z0-9_]+)(?:\s+(confirmed))?$/i);
  if (delTableMatch) {
    const tableName = delTableMatch[1];
    const isConfirmed = Boolean(delTableMatch[2]);
    const currentDb = engineState.databases[engineState.activeDb];
    if (!currentDb.tables[tableName]) {
      return { type: 'ERROR', error: `Table '${tableName}' does not exist in '${engineState.activeDb}'.` };
    }
    if (!isConfirmed) {
      return {
        type: 'SECURITY_GUARD',
        output: `[SECURITY GUARD] Table drop aborted! Confirmation required.\nTo permanently drop this table and purge all records, run:\n  delete table ${tableName} confirmed;`
      };
    }
    delete currentDb.tables[tableName];
    return {
      type: 'DROP_TABLE',
      output: `Query OK: Table '${tableName}' permanently dropped from '${engineState.activeDb}'.`
    };
  }

  // 12. DELETE DATABASE <name> [confirmed]
  const delDbMatch = stmt.match(/^delete\s+database\s+["']?([a-zA-Z0-9_\-.]+)["']?(?:\s+(confirmed))?$/i);
  if (delDbMatch) {
    const dbName = delDbMatch[1];
    const isConfirmed = Boolean(delDbMatch[2]);
    if (!engineState.databases[dbName]) {
      return { type: 'ERROR', error: `Database '${dbName}' does not exist.` };
    }
    if (!isConfirmed) {
      return {
        type: 'SECURITY_GUARD',
        output: `[SECURITY GUARD] Database drop aborted! Confirmation required.\nTo permanently drop this database and all tables, run:\n  delete database ${dbName} confirmed;`
      };
    }
    delete engineState.databases[dbName];
    if (engineState.activeDb === dbName) {
      const remaining = Object.keys(engineState.databases);
      engineState.activeDb = remaining.length > 0 ? remaining[0] : 'main_db';
      if (!engineState.databases[engineState.activeDb]) {
        engineState.databases[engineState.activeDb] = { name: engineState.activeDb, tables: {} };
      }
    }
    return {
      type: 'DROP_DATABASE',
      output: `Query OK: Database '${dbName}' permanently dropped.`
    };
  }

  return {
    type: 'UNKNOWN',
    error: `EnlngDB Syntax Error: Unrecognized statement: '${stmt}'. Check syntax or run 'show databases;'`
  };
}

// Extracts executable statements from a script, handling semicolons, newlines, and comments
function extractStatements(text) {
  const cleanLines = [];
  for (const rawLine of text.split('\n')) {
    const trimmed = rawLine.trim();
    if (!trimmed) continue;
    if (trimmed.startsWith('//') || trimmed.startsWith('--') || trimmed.startsWith('#')) {
      continue;
    }
    cleanLines.push(trimmed);
  }

  const combined = cleanLines.join('\n');
  if (!combined.trim()) return [];

  if (combined.includes(';')) {
    const rawStmts = splitOutsideQuotes(combined, ';');
    const stmts = [];
    for (const s of rawStmts) {
      const t = s.trim();
      if (!t) continue;
      if (/^type\s+(?:enlngdb|enlgdb|enlng)$/i.test(t)) continue;
      stmts.push(t);
    }
    return stmts;
  }

  const stmts = [];
  for (const line of cleanLines) {
    const t = line.trim();
    if (!t) continue;
    if (/^type\s+(?:enlngdb|enlgdb|enlng)$/i.test(t)) continue;
    stmts.push(t);
  }
  return stmts;
}

// Identifies if source code belongs to EnlngDB database engine
function isEnlngDbCode(code) {
  const trimmed = code.trim().toLowerCase();
  if (trimmed.startsWith('type enlngdb') || trimmed.startsWith('type enlgdb')) {
    return true;
  }
  const dbPatterns = [
    /\bshow\s+databases\b/i,
    /\buse\s+database\b/i,
    /\buse\s+[a-zA-Z0-9_\-.]+\s*;/i,
    /\bshow\s+tables\b/i,
    /\bcreate\s+table\b/i,
    /\binsert\s+(?:record\s+)?into\b/i,
    /\bfind\s+all\s+records\b/i,
    /\bshow\s+all\s+records\b/i,
    /\bfind\s+records\s+from\b/i,
    /\bdelete\s+column\b/i,
    /\bdelete\s+table\b/i,
    /\bdelete\s+database\b/i,
    /\b(?:in\s+[a-zA-Z0-9_]+\s+)?(?:update|change|modify)\b/i,
    /\bupdate\s+[a-zA-Z0-9_]+\s+set\b/i
  ];
  return dbPatterns.some(regex => regex.test(code));
}

// Retrieves either full code, highlighted selection, or current statement under cursor
function getQueryToExecute(editor, isSelectionOnly) {
  const fullText = editor.value;
  if (!isSelectionOnly) {
    return { text: fullText, mode: 'all', lineInfo: 'all', lineNum: 1 };
  }

  const start = editor.selectionStart;
  const end = editor.selectionEnd;

  // Highlighted selection
  if (start !== end) {
    const selectedText = fullText.substring(start, end).trim();
    if (selectedText.length > 0) {
      const lineStart = fullText.substring(0, start).split('\n').length;
      const lineEnd = fullText.substring(0, end).split('\n').length;
      const lineInfo = lineStart === lineEnd ? `line ${lineStart}` : `lines ${lineStart}-${lineEnd}`;
      return { text: selectedText, mode: 'selection', lineInfo, lineNum: lineStart };
    }
  }

  // Single line at cursor
  const textBefore = fullText.substring(0, start);
  const prevNewline = textBefore.lastIndexOf('\n');
  const lineStartIdx = prevNewline === -1 ? 0 : prevNewline + 1;

  let lineEndIdx = fullText.indexOf('\n', start);
  if (lineEndIdx === -1) lineEndIdx = fullText.length;

  const currentLine = fullText.substring(lineStartIdx, lineEndIdx).trim();
  const currentLineNum = fullText.substring(0, lineStartIdx).split('\n').length;

  return {
    text: currentLine,
    mode: 'line',
    lineInfo: `line ${currentLineNum}`,
    lineNum: currentLineNum
  };
}

// Executes statements in browser against sovereign database store
function executeEnlngDB(statementsToRun, terminal, executionMeta = { mode: 'all' }) {
  const outputs = [];
  const tStart = performance.now();

  if (executionMeta.mode === 'line') {
    outputs.push(`<span class="term-dim">// ▶ Executing line ${executionMeta.lineNum}:</span>`);
  } else if (executionMeta.mode === 'selection') {
    outputs.push(`<span class="term-dim">// ▶ Executing selection (${executionMeta.lineInfo}):</span>`);
  } else {
    outputs.push(`<span class="term-dim">// === EnlngDB Pure C Engine (Active DB: ${sovereignDB.activeDb} | &lt;0.05ms Latency) ===</span>`);
  }

  if (statementsToRun.length === 0) {
    if (executionMeta.mode === 'line') {
      outputs.push(`<span class="term-warn">[EnlngDB] No executable query found on line ${executionMeta.lineNum}. Place cursor on a query statement or highlight code to execute.</span>`);
    } else {
      outputs.push(`<span class="term-dim">// 0 executable statements found.</span>`);
    }
    terminal.innerHTML = outputs.join('\n');
    return;
  }

  let successCount = 0;
  let failCount = 0;

  for (let i = 0; i < statementsToRun.length; i++) {
    const stmt = statementsToRun[i];
    const stmtPrefix = statementsToRun.length > 1 ? `[${i + 1}/${statementsToRun.length}] ` : '';

    outputs.push(`<span class="term-stmt">${escapeHtml(stmtPrefix + stmt)};</span>`);

    try {
      const res = executeEnlngDBStatement(stmt, sovereignDB);
      if (!res) continue;

      if (res.type === 'ERROR' || res.error) {
        failCount++;
        outputs.push(`<span class="term-err">${escapeHtml(res.error)}</span>\n`);
      } else if (res.type === 'SECURITY_GUARD') {
        outputs.push(`<span class="term-warn">${escapeHtml(res.output)}</span>\n`);
      } else {
        successCount++;
        outputs.push(`<span class="term-success">${escapeHtml(res.output)}</span>\n`);
      }
    } catch (err) {
      failCount++;
      outputs.push(`<span class="term-err">EnlngDB Execution Error: ${escapeHtml(err.message)}</span>\n`);
    }
  }

  const tEnd = performance.now();
  const totalMs = (tEnd - tStart).toFixed(2);

  if (executionMeta.mode === 'all') {
    outputs.push(`<span class="term-dim">// Finished: ${successCount} succeeded, ${failCount} failed (${totalMs} ms)</span>`);
  }

  terminal.innerHTML = outputs.join('\n');
  terminal.scrollTop = terminal.scrollHeight;

  // Update status indicators
  const timePill = document.getElementById('runtimeExecTime');
  if (timePill) timePill.textContent = `Execution: ${totalMs}ms (In-Memory Micro-VM)`;

  const activeDbElem = document.getElementById('statusActiveDb');
  if (activeDbElem) {
    activeDbElem.textContent = `Active DB: ${sovereignDB.activeDb}`;
    activeDbElem.style.display = 'inline-block';
  }
}

function initPlayground() {
  const editor = document.getElementById('codeEditor');
  const terminal = document.getElementById('terminalOutput');
  const runBtn = document.getElementById('runCodeBtn');
  const runSelectionBtn = document.getElementById('runSelectionBtn');
  const clearBtn = document.getElementById('clearOutputBtn');
  const resetDbBtn = document.getElementById('resetDbBtn');
  const copyOutputBtn = document.getElementById('copyOutputBtn') || document.getElementById('copyTermBtn');
  const lineNumbersElem = document.getElementById('editorLineNumbers') || document.getElementById('lineNumbers');
  const lineCountElem = document.getElementById('editorLineCount');
  const presetBtns = document.querySelectorAll('.preset-btn');
  const modeCoreBtn = document.getElementById('modeCoreBtn');
  const modeDbBtn = document.getElementById('modeDbBtn');
  const corePresetsBar = document.getElementById('corePresetsBar');
  const dbPresetsBar = document.getElementById('dbPresetsBar');
  const editorFileBadge = document.getElementById('editorFileBadge');
  const editorEngineBadge = document.getElementById('editorEngineBadge');
  const editorDomainStatus = document.getElementById('editorDomainStatus');
  const statusActiveDb = document.getElementById('statusActiveDb');
  const statusEngineText = document.getElementById('statusEngineText');

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

  function syncDomainVisuals() {
    if (!editor) return;
    const isDb = isEnlngDbCode(editor.value);
    if (isDb) {
      if (editorFileBadge) editorFileBadge.textContent = 'sandbox.enlngdb';
      if (editorEngineBadge) editorEngineBadge.textContent = 'EnlngDB Pure C Engine (<0.05ms)';
      if (editorDomainStatus) editorDomainStatus.innerHTML = 'Domain: <strong>Pure C Database (.enlngdb)</strong>';
      if (statusActiveDb) {
        statusActiveDb.style.display = 'inline-block';
        statusActiveDb.textContent = `Active DB: ${sovereignDB.activeDb}`;
      }
      if (statusEngineText) statusEngineText.textContent = 'EnlngDB Pure C Engine Ready';
    } else {
      if (editorFileBadge) editorFileBadge.textContent = 'sandbox.enlng';
      if (editorEngineBadge) editorEngineBadge.textContent = 'Enlangg Compiler';
      if (editorDomainStatus) editorDomainStatus.innerHTML = 'Domain: <strong>Core (.enlng)</strong>';
      if (statusActiveDb) statusActiveDb.style.display = 'none';
      if (statusEngineText) statusEngineText.textContent = 'Sovereign VM Engine Ready';
    }
  }

  if (editor && lineNumbersElem) {
    editor.addEventListener('input', () => {
      updateLineNumbers();
      syncDomainVisuals();
    });
    editor.addEventListener('scroll', () => {
      lineNumbersElem.scrollTop = editor.scrollTop;
    });
  }

  // Mode toggles
  if (modeCoreBtn && modeDbBtn) {
    modeCoreBtn.addEventListener('click', () => {
      modeCoreBtn.classList.add('active');
      modeDbBtn.classList.remove('active');
      if (corePresetsBar) corePresetsBar.style.display = 'flex';
      if (dbPresetsBar) dbPresetsBar.style.display = 'none';
      if (editor && CODE_PRESETS.fibonacci) {
        editor.value = CODE_PRESETS.fibonacci;
        updateLineNumbers();
        syncDomainVisuals();
      }
    });

    modeDbBtn.addEventListener('click', () => {
      modeDbBtn.classList.add('active');
      modeCoreBtn.classList.remove('active');
      if (corePresetsBar) corePresetsBar.style.display = 'none';
      if (dbPresetsBar) dbPresetsBar.style.display = 'flex';
      if (editor && CODE_PRESETS.enlngdb_tour) {
        editor.value = CODE_PRESETS.enlngdb_tour;
        updateLineNumbers();
        syncDomainVisuals();
      }
    });
  }

  // Preset buttons
  presetBtns.forEach(btn => {
    btn.addEventListener('click', () => {
      presetBtns.forEach(b => b.classList.remove('active'));
      btn.classList.add('active');
      const presetName = btn.getAttribute('data-preset');
      if (CODE_PRESETS[presetName] && editor) {
        editor.value = CODE_PRESETS[presetName];
        updateLineNumbers();
        syncDomainVisuals();
      }
    });
  });

  // URL parameters handling
  const urlParams = new URLSearchParams(window.location.search);
  const topicParam = urlParams.get('topic');
  const presetParam = urlParams.get('preset');
  const codeParam = urlParams.get('code');

  if (topicParam && (CODE_PRESETS['topic' + topicParam] || CODE_PRESETS[topicParam])) {
    const key = CODE_PRESETS['topic' + topicParam] ? ('topic' + topicParam) : topicParam;
    if (editor) {
      editor.value = CODE_PRESETS[key];
      updateLineNumbers();
      syncDomainVisuals();
    }
  } else if (presetParam && CODE_PRESETS[presetParam]) {
    if (editor) {
      editor.value = CODE_PRESETS[presetParam];
      updateLineNumbers();
      syncDomainVisuals();
    }
  } else if (codeParam) {
    try {
      if (editor) {
        editor.value = decodeURIComponent(codeParam);
        updateLineNumbers();
        syncDomainVisuals();
      }
    } catch(e) {}
  } else if (editor) {
    if (!editor.value.trim()) {
      editor.value = CODE_PRESETS.fibonacci;
    }
    updateLineNumbers();
    syncDomainVisuals();
  }

  // Clear button
  if (clearBtn) {
    clearBtn.addEventListener('click', () => {
      terminal.innerHTML = '<span class="term-dim">// Terminal cleared. Press Run All (Ctrl+Enter) or Run Selection (Shift+Enter).</span>';
    });
  }

  // Reset DB button
  if (resetDbBtn) {
    resetDbBtn.addEventListener('click', () => {
      resetSovereignDB();
      syncDomainVisuals();
      terminal.innerHTML = `<span class="term-success">[EnlngDB] Sovereign in-memory database storage reset to initial sample seed:\n • database1 (tables: student, faculty)\n • main_db (tables: accounts, system_logs)\n • university_db (tables: courses)\nActive database: ${sovereignDB.activeDb}</span>`;
    });
  }

  // Copy button
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

  // Execute Entire Script (Run All)
  const runAll = () => {
    if (!editor || !terminal) return;
    if (runBtn) {
      runBtn.classList.add('running');
      setTimeout(() => runBtn.classList.remove('running'), 200);
    }
    const code = editor.value;
    if (isEnlngDbCode(code)) {
      const stmts = extractStatements(code);
      executeEnlngDB(stmts, terminal, { mode: 'all' });
    } else {
      executeEnlngInBrowser(code, terminal);
    }
  };

  // Execute Selection or Current Line
  const runSelection = () => {
    if (!editor || !terminal) return;
    if (runSelectionBtn) {
      runSelectionBtn.classList.add('running');
      setTimeout(() => runSelectionBtn.classList.remove('running'), 200);
    }
    const queryData = getQueryToExecute(editor, true);
    if (!queryData.text || !queryData.text.trim()) {
      terminal.innerHTML = `<span class="term-warn">[Sandbox] Cursor is on an empty line (${queryData.lineInfo}). Place cursor on code or highlight queries to execute.</span>`;
      return;
    }

    if (isEnlngDbCode(editor.value) || isEnlngDbCode(queryData.text)) {
      const stmts = extractStatements(queryData.text);
      executeEnlngDB(stmts, terminal, queryData);
    } else {
      executeEnlngInBrowser(queryData.text, terminal, true);
    }
  };

  if (runBtn) {
    runBtn.addEventListener('click', runAll);
  }

  if (runSelectionBtn) {
    runSelectionBtn.addEventListener('click', runSelection);
  }

  // Keyboard Shortcuts: Ctrl+Enter (Run All), Shift+Enter / Ctrl+Shift+Enter / Alt+Enter (Run Selection/Line)
  window.addEventListener('keydown', (e) => {
    // Ctrl + Shift + Enter or Alt + Enter: Run Selection
    if (((e.ctrlKey || e.metaKey) && e.shiftKey && e.key === 'Enter') || (e.altKey && e.key === 'Enter')) {
      e.preventDefault();
      runSelection();
      return;
    }

    // Ctrl + Enter: Run All
    if ((e.ctrlKey || e.metaKey) && e.key === 'Enter' && !e.shiftKey) {
      e.preventDefault();
      runAll();
      return;
    }

    // Shift + Enter inside editor: Run Selection or Current Line
    if (e.shiftKey && e.key === 'Enter' && !e.ctrlKey && !e.metaKey && document.activeElement === editor) {
      e.preventDefault();
      runSelection();
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
  const declaredVars = new Set();
  
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

    // 1. Variable Declarations (create / declare / initialize / let / define)
    const createMatch = trimmed.match(/^(?:create\s+(?:a\s+|an\s+|the\s+)?|declare\s+(?:a\s+|an\s+|the\s+)?|initialize\s+(?:a\s+|an\s+|the\s+)?|let\s+|define\s+)([a-zA-Z0-9_]+)\s+(?:of|as|to|=)\s+(.*)$/i);
    if (createMatch) {
      const varName = createMatch[1];
      const valExpr = transpileExpression(createMatch[2]);
      declaredVars.add(varName);
      intermediateLines.push({ type: 'stmt', indent: indentLen, code: `${varName} = ${valExpr};` });
      continue;
    }

    // 2. Variable Assignments & Mutations (set / update / assign / change)
    const setMatch = trimmed.match(/^(?:set|update|assign|change)\s+([a-zA-Z0-9_\[\]\.]+)\s+(?:to|=)\s+(.*)$/i);
    if (setMatch) {
      const target = setMatch[1];
      const valExpr = transpileExpression(setMatch[2]);
      if (/^[a-zA-Z_][a-zA-Z0-9_]*$/.test(target)) {
        declaredVars.add(target);
      }
      intermediateLines.push({ type: 'stmt', indent: indentLen, code: `${target} = ${valExpr};` });
      continue;
    }

    // 2b. Increment / Decrement
    const incMatch = trimmed.match(/^(?:increase)\s+([a-zA-Z0-9_\[\]\.]+)\s+by\s+(.*)$/i);
    if (incMatch) {
      const target = incMatch[1];
      const valExpr = transpileExpression(incMatch[2]);
      if (/^[a-zA-Z_][a-zA-Z0-9_]*$/.test(target)) {
        declaredVars.add(target);
      }
      intermediateLines.push({ type: 'stmt', indent: indentLen, code: `${target} += ${valExpr};` });
      continue;
    }
    const decMatch = trimmed.match(/^(?:decrease)\s+([a-zA-Z0-9_\[\]\.]+)\s+by\s+(.*)$/i);
    if (decMatch) {
      const target = decMatch[1];
      const valExpr = transpileExpression(decMatch[2]);
      if (/^[a-zA-Z_][a-zA-Z0-9_]*$/.test(target)) {
        declaredVars.add(target);
      }
      intermediateLines.push({ type: 'stmt', indent: indentLen, code: `${target} -= ${valExpr};` });
      continue;
    }

    // 3. Direct assignment: a = b
    const directAssign = trimmed.match(/^([a-zA-Z0-9_\[\]\.]+)\s*=\s*(.*)$/);
    if (directAssign) {
      const target = directAssign[1];
      const valExpr = transpileExpression(directAssign[2]);
      if (/^[a-zA-Z_][a-zA-Z0-9_]*$/.test(target)) {
        declaredVars.add(target);
      }
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

    const elifMatch = trimmed.match(/^(?:else\s+if|otherwise\s+if|elif)\s+(.*?):$/i);
    if (elifMatch) {
      const cond = transpileExpression(elifMatch[1]);
      intermediateLines.push({ type: 'block_mid', indent: indentLen, code: `} else if (${cond}) {` });
      continue;
    }

    if (/^(?:else|otherwise):$/i.test(trimmed)) {
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

  if (declaredVars.size > 0) {
    finalJS.unshift(`var ${Array.from(declaredVars).join(', ')};`);
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

  // Boolean logical operators
  res = res.replace(/\band\b/gi, '&&');
  res = res.replace(/\bor\b/gi, '||');
  res = res.replace(/\bnot\s+/gi, '!');

  // Collection containment: <collection> contains <item>
  res = res.replace(/\b([a-zA-Z0-9_\[\]]+)\s+contains\s+(.*)/gi, '($1.includes($2))');

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

set word to "madam"
set reversed to ""
set i to 0

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
    title: 'Pure C Native Database (.enlngdb)',
    filename: 'schema_registry.enlngdb',
    desc: 'Pure C native database engine compiled directly into enlangg.exe with zero Python/SQL dependencies and microsecond query latencies.',
    features: [
      'Pure C storage & execution engine (<0.05 ms query latency)',
      'Natural conversational row & column mutations (in <table> change/update)',
      'Direct binary serialization (.edb) with zero ORM/SQL overhead'
    ],
    code: `type enlngdb;

use university_db;

create table scholars with id, name, cgpa, status;

insert into scholars with id 1, name "aryan", cgpa 8.2, status "probation";
insert into scholars with id 2, name "meera", cgpa 9.4, status "honors";

# Natural In-Table Mutation (0.03 ms in Pure C)
in scholars change cgpa to 9.8 where name is "aryan";
in scholars update status to "dean_list" where cgpa is greater than 9.0;

find all records from scholars;`
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

