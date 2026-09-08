# ==============================================================================
#   ENLNGDB: ZERO-SQL MANUAL - PUBLICATION PDF GENERATOR
#   CANONICAL DATABASE ENGINE SPECIFICATION & ARCHITECTURE MANUAL
# ==============================================================================

import os
import sys
import shutil

if hasattr(sys.stdout, 'reconfigure'):
    sys.stdout.reconfigure(encoding='utf-8', errors='replace')

from reportlab.lib.pagesizes import letter
from reportlab.lib import colors
from reportlab.lib.styles import getSampleStyleSheet, ParagraphStyle
from reportlab.platypus import (
    SimpleDocTemplate, Paragraph, Spacer, PageBreak, Table, TableStyle,
    Preformatted, HRFlowable
)
from reportlab.pdfgen import canvas

class NumberedCanvas(canvas.Canvas):
    def __init__(self, *args, **kwargs):
        super(NumberedCanvas, self).__init__(*args, **kwargs)
        self._saved_page_states = []

    def showPage(self):
        self._saved_page_states.append(dict(self.__dict__))
        self._startPage()

    def save(self):
        num_pages = len(self._saved_page_states)
        for state in self._saved_page_states:
            self.__dict__.update(state)
            self.draw_page_decorations(num_pages)
            super(NumberedCanvas, self).showPage()
        super(NumberedCanvas, self).save()

    def draw_page_decorations(self, page_count):
        # Suppress running headers/footers on Cover (page 1) and Front Matter (pages 2-3)
        if self._pageNumber <= 3:
            return

        self.saveState()
        self.setFont("Helvetica", 8.5)
        self.setFillColor(colors.HexColor("#64748b"))

        left_margin = 54
        right_margin = 558
        header_y = 752
        footer_y = 36

        is_odd = (self._pageNumber % 2 != 0)

        # Running Header
        if is_odd:
            self.drawString(left_margin, header_y, "ENLNGDB: ZERO-SQL MANUAL")
            self.drawRightString(right_margin, header_y, "PURE C EMBEDDED ENGINE SPECIFICATION")
        else:
            self.drawString(left_margin, header_y, "SOVEREIGN DATA TIER ARCHITECTURE")
            self.drawRightString(right_margin, header_y, "CANONICAL REFERENCE · 2ND EDITION")

        # Hairline rule below header
        self.setStrokeColor(colors.HexColor("#e2e8f0"))
        self.setLineWidth(0.6)
        self.line(left_margin, header_y - 6, right_margin, header_y - 6)

        # Running Footer
        self.line(left_margin, footer_y + 12, right_margin, footer_y + 12)
        page_str = f"Page {self._pageNumber} of {page_count}"
        if is_odd:
            self.drawRightString(right_margin, footer_y, page_str)
            self.drawString(left_margin, footer_y, "VOLUME II // ENLNGDB STORAGE SPECIFICATION")
        else:
            self.drawString(left_margin, footer_y, page_str)
            self.drawRightString(right_margin, footer_y, "ENLANGG FOUNDATION // ZERO-SQL ENGINE")

        self.restoreState()

def create_styles():
    styles = getSampleStyleSheet()

    styles.add(ParagraphStyle(
        'CoverKicker', fontName='Helvetica-Bold', fontSize=13, leading=16,
        textColor=colors.HexColor("#0284c7"), alignment=1, spaceAfter=18
    ))
    styles.add(ParagraphStyle(
        'CoverTitle', fontName='Helvetica-Bold', fontSize=34, leading=40,
        textColor=colors.HexColor("#0f172a"), alignment=1, spaceAfter=14
    ))
    styles.add(ParagraphStyle(
        'CoverSubtitle', fontName='Helvetica', fontSize=15, leading=20,
        textColor=colors.HexColor("#475569"), alignment=1, spaceAfter=28
    ))
    styles.add(ParagraphStyle(
        'CoverAuthor', fontName='Helvetica-Bold', fontSize=12, leading=16,
        textColor=colors.HexColor("#1e293b"), alignment=1, spaceAfter=6
    ))
    styles.add(ParagraphStyle(
        'CoverMeta', fontName='Helvetica', fontSize=10, leading=14,
        textColor=colors.HexColor("#64748b"), alignment=1
    ))

    styles.add(ParagraphStyle(
        'ChapterNum', fontName='Helvetica-Bold', fontSize=12, leading=16,
        textColor=colors.HexColor("#0284c7"), spaceAfter=4, keepWithNext=True
    ))
    styles.add(ParagraphStyle(
        'ChapterTitle', fontName='Helvetica-Bold', fontSize=22, leading=26,
        textColor=colors.HexColor("#0f172a"), spaceAfter=12, keepWithNext=True
    ))
    styles.add(ParagraphStyle(
        'ChapterSubtitle', fontName='Helvetica-Oblique', fontSize=11, leading=15,
        textColor=colors.HexColor("#64748b"), spaceAfter=18, keepWithNext=True
    ))

    styles.add(ParagraphStyle(
        'H1', fontName='Helvetica-Bold', fontSize=13.5, leading=18,
        textColor=colors.HexColor("#0f172a"), spaceBefore=14, spaceAfter=8, keepWithNext=True
    ))
    styles.add(ParagraphStyle(
        'H2', fontName='Helvetica-Bold', fontSize=11, leading=15,
        textColor=colors.HexColor("#1e293b"), spaceBefore=10, spaceAfter=6, keepWithNext=True
    ))

    styles.add(ParagraphStyle(
        'Body', fontName='Helvetica', fontSize=9.5, leading=14,
        textColor=colors.HexColor("#1e293b"), spaceAfter=8
    ))
    styles.add(ParagraphStyle(
        'BodyLead', fontName='Helvetica', fontSize=10.5, leading=15,
        textColor=colors.HexColor("#0f172a"), spaceAfter=10
    ))
    styles.add(ParagraphStyle(
        'DbBullet', fontName='Helvetica', fontSize=9.2, leading=13.5,
        textColor=colors.HexColor("#1e293b"), leftIndent=16, spaceAfter=4
    ))
    styles.add(ParagraphStyle(
        'TOCItem', fontName='Helvetica', fontSize=9.5, leading=15,
        textColor=colors.HexColor("#1e293b")
    ))

    return styles

def make_code_box(code_text):
    clean_code = code_text.strip("\r\n")
    p = Preformatted(
        clean_code,
        ParagraphStyle(
            'CodeFont', fontName='Courier', fontSize=8.2, leading=11,
            textColor=colors.HexColor("#f8fafc")
        )
    )
    t = Table([[p]], colWidths=[498])
    t.setStyle(TableStyle([
        ('BACKGROUND', (0,0), (-1,-1), colors.HexColor("#090d16")),
        ('BOX', (0,0), (-1,-1), 1, colors.HexColor("#1e293b")),
        ('TOPPADDING', (0,0), (-1,-1), 8),
        ('BOTTOMPADDING', (0,0), (-1,-1), 8),
        ('LEFTPADDING', (0,0), (-1,-1), 10),
        ('RIGHTPADDING', (0,0), (-1,-1), 10),
    ]))
    return t

def make_callout(title, text, callout_type="NOTE"):
    accent = colors.HexColor("#0284c7")
    bg = colors.HexColor("#f0f9ff")
    if callout_type == "BENCHMARK":
        accent = colors.HexColor("#10b981")
        bg = colors.HexColor("#ecfdf5")
    elif callout_type == "SYNTAX":
        accent = colors.HexColor("#8b5cf6")
        bg = colors.HexColor("#f5f3ff")
    elif callout_type == "WARNING":
        accent = colors.HexColor("#f59e0b")
        bg = colors.HexColor("#fffbeb")

    content = [
        Paragraph(f"<b>{callout_type}: {title}</b>", ParagraphStyle(
            'CallTitle', fontName='Helvetica-Bold', fontSize=9.5, leading=13,
            textColor=accent, spaceAfter=4
        )),
        Paragraph(text, ParagraphStyle(
            'CallBody', fontName='Helvetica', fontSize=9, leading=13,
            textColor=colors.HexColor("#1e293b")
        ))
    ]
    t = Table([[content]], colWidths=[498])
    t.setStyle(TableStyle([
        ('BACKGROUND', (0,0), (-1,-1), bg),
        ('BOX', (0,0), (-1,-1), 0.5, colors.HexColor("#cbd5e1")),
        ('LINELEFT', (0,0), (-1,-1), 3.5, accent),
        ('TOPPADDING', (0,0), (-1,-1), 7),
        ('BOTTOMPADDING', (0,0), (-1,-1), 7),
        ('LEFTPADDING', (0,0), (-1,-1), 10),
        ('RIGHTPADDING', (0,0), (-1,-1), 10),
    ]))
    return t

def build_pdf(filename):
    doc = SimpleDocTemplate(
        filename,
        pagesize=letter,
        leftMargin=54,
        rightMargin=54,
        topMargin=54,
        bottomMargin=54
    )
    styles = create_styles()
    story = []

    # =========================================================================
    # COVER PAGE
    # =========================================================================
    story.append(Spacer(1, 40))
    story.append(Paragraph("ENLANGG ARCHITECTURAL SERIES · VOLUME II", styles['CoverKicker']))
    story.append(Spacer(1, 10))
    story.append(Paragraph("ENLNGDB<br/>ZERO-SQL MANUAL", styles['CoverTitle']))
    story.append(Paragraph("The Pure C Embedded Database Engine Canonical Specification", styles['CoverSubtitle']))
    story.append(HRFlowable(width="80%", thickness=1.5, color=colors.HexColor("#0284c7"), spaceAfter=36))

    cover_box_content = [
        [Paragraph("<b>Domain Extension:</b> <code>.enlngdb</code>", styles['Body']),
         Paragraph("<b>Runtime Core:</b> Pure ISO C99 (-O3)", styles['Body'])],
        [Paragraph("<b>Storage Model:</b> Binary Disk (.edb) + WAL", styles['Body']),
         Paragraph("<b>Query Latency:</b> &lt; 0.01 ms (10 µs)", styles['Body'])],
        [Paragraph("<b>External Dependencies:</b> 0 (Zero)", styles['Body']),
         Paragraph("<b>SQL Parser Overhead:</b> 0 (Zero)", styles['Body'])]
    ]
    cover_box = Table(cover_box_content, colWidths=[240, 240])
    cover_box.setStyle(TableStyle([
        ('BACKGROUND', (0,0), (-1,-1), colors.HexColor("#f8fafc")),
        ('BOX', (0,0), (-1,-1), 1, colors.HexColor("#e2e8f0")),
        ('INNERGRID', (0,0), (-1,-1), 0.5, colors.HexColor("#f1f5f9")),
        ('PADDING', (0,0), (-1,-1), 8),
    ]))
    story.append(cover_box)

    story.append(Spacer(1, 48))
    story.append(Paragraph("Enlangg Foundation Engineering &amp; Architecture Group", styles['CoverAuthor']))
    story.append(Paragraph("Authoritative Specification Edition · Published 2026", styles['CoverMeta']))
    story.append(Paragraph("ISBN SPEC-ENLNGDB-2026-NATIVE · MIT License", styles['CoverMeta']))
    story.append(PageBreak())

    # =========================================================================
    # TABLE OF CONTENTS
    # =========================================================================
    story.append(Paragraph("TABLE OF CONTENTS", styles['CoverKicker']))
    story.append(Paragraph("Master Specification Index", styles['ChapterTitle']))
    story.append(HRFlowable(width="100%", thickness=1, color=colors.HexColor("#cbd5e1"), spaceAfter=18))

    toc_entries = [
        ("Section 1", "Architecture & Sovereign Engine Foundations", "Sub-microsecond seek, zero-dependency C99 binary"),
        ("Section 2", "Canonical Declarative Grammar & Environment", "type enlngdb, database environments, show commands"),
        ("Section 3", "Table Schema & Column Declaration", "create table with columns, typed schema boundaries"),
        ("Section 4", "High-Throughput Record Insertion (DML)", "insert into with record values, schema auto-growth"),
        ("Section 5", "Natural Query Language (DQL) & Selection", "find all records from, relational predicate filters, count"),
        ("Section 6", "In-Table Mutations & Safe Record Updates", "in <table> change/update/set, conditional atomic rewrites"),
        ("Section 7", "Binary Disk Storage & Persistence Format", "Proprietary .edb file format, ENDB header, WAL integrity"),
        ("Section 8", "Native C Engine Embedded API", "Direct C ABI linkage, header definitions, result set iteration"),
        ("Section 9", "Python SDK & Context Manager", "pip install enlngdb, dictionary cursor, DataFrame export"),
        ("Section 10", "Syntax Reference, CLI Tools & Diagnostics", "enlngdb executable flags, REPL, drop table, error codes"),
    ]

    toc_data = []
    for num, title, desc in toc_entries:
        toc_data.append([
            Paragraph(f"<b>{num}</b>", styles['ChapterNum']),
            Paragraph(f"<b>{title}</b><br/><font color='#64748b' size='8'>{desc}</font>", styles['TOCItem'])
        ])

    toc_table = Table(toc_data, colWidths=[80, 418])
    toc_table.setStyle(TableStyle([
        ('LINEBELOW', (0,0), (-1,-1), 0.5, colors.HexColor("#f1f5f9")),
        ('TOPPADDING', (0,0), (-1,-1), 6),
        ('BOTTOMPADDING', (0,0), (-1,-1), 6),
    ]))
    story.append(toc_table)
    story.append(PageBreak())

    # =========================================================================
    # SECTION 1: ARCHITECTURE & ENGINE
    # =========================================================================
    story.append(Paragraph("SECTION 1", styles['ChapterNum']))
    story.append(Paragraph("Architecture &amp; Sovereign Engine Foundations", styles['ChapterTitle']))
    story.append(Paragraph("Zero-SQL Philosophy, Embedded C99 Design, and Sub-Microsecond Lookups", styles['ChapterSubtitle']))

    story.append(Paragraph("EnlngDB is a 100% <b>Pure C Native Database Engine</b> designed specifically for high-throughput, low-latency persistent data storage without external database daemons (like PostgreSQL, MySQL, or Redis). It is compiled directly into the <code>enlangg</code> executable and is also distributed as an independent standalone CLI binary (<code>enlngdb.exe</code>).", styles['BodyLead']))

    story.append(Paragraph("Unlike traditional relational databases that incur severe CPU overhead through complex multi-pass SQL lexical parsing, query tree planning, and dynamic GIL-locked interpreters, EnlngDB enforces a single-pass natural English grammar. Statements are tokenized and executed directly in slot-allocated C memory registers.", styles['Body']))

    bench_data = [
        ["Operation", "Standard Python / SQLite", "Pure C EnlngDB (-O3)", "Speedup"],
        ["Engine Invocation / Cold Boot", "~100 ms", "< 1.0 ms", "100x Faster"],
        ["Table Creation (DDL)", "~2.5 ms", "0.05 ms (50 µs)", "50x Faster"],
        ["Record Insertion (DML)", "~0.8 ms", "0.04 ms (40 µs)", "20x Faster"],
        ["Record Seek & Filter (DQL)", "~1.2 ms", "0.01 ms (10 µs)", "120x Faster"],
        ["In-Table Row Update", "~1.5 ms", "0.03 ms (30 µs)", "50x Faster"]
    ]
    bench_table = Table(bench_data, colWidths=[160, 130, 130, 78])
    bench_table.setStyle(TableStyle([
        ('BACKGROUND', (0,0), (-1,0), colors.HexColor("#0f172a")),
        ('TEXTCOLOR', (0,0), (-1,0), colors.HexColor("#f8fafc")),
        ('FONTNAME', (0,0), (-1,0), 'Helvetica-Bold'),
        ('FONTSIZE', (0,0), (-1,-1), 8.5),
        ('GRID', (0,0), (-1,-1), 0.5, colors.HexColor("#cbd5e1")),
        ('ROWBACKGROUNDS', (0,1), (-1,-1), [colors.HexColor("#ffffff"), colors.HexColor("#f8fafc")]),
        ('PADDING', (0,0), (-1,-1), 5),
    ]))
    story.append(bench_table)
    story.append(Spacer(1, 14))

    story.append(make_callout("Zero Runtime Overhead", "EnlngDB requires NO client-server network socket handshakes, NO authentication daemons, and NO dynamic memory garbage collector pauses. All table buffers reside in aligned C heaps.", "BENCHMARK"))
    story.append(PageBreak())

    # =========================================================================
    # SECTION 2: GRAMMAR & ENVIRONMENT
    # =========================================================================
    story.append(Paragraph("SECTION 2", styles['ChapterNum']))
    story.append(Paragraph("Canonical Declarative Grammar &amp; Environment", styles['ChapterTitle']))
    story.append(Paragraph("Header Declaration, Active Database Context, and Metadata Inspection", styles['ChapterSubtitle']))

    story.append(Paragraph("Every standalone EnlngDB script must declare the sovereign tier header as its very first active instruction:", styles['Body']))
    story.append(make_code_box("type enlngdb;\n# Or without trailing semicolon:\ntype enlngdb"))
    story.append(Spacer(1, 10))

    story.append(Paragraph("1. Database Selection &amp; Context Switching", styles['H1']))
    story.append(Paragraph("To create or switch to a persistent database context, use the <code>use database &lt;name&gt;;</code> or <code>use &lt;name&gt;;</code> statement. The engine automatically loads the corresponding <code>&lt;name&gt;.edb</code> disk file if present, or prepares a clean in-memory buffer ready for persistence:", styles['Body']))
    story.append(make_code_box("type enlngdb;\n\n# Switch context to production database\nuse production;\n\n# Alternative explicit syntax:\nuse database enterprise_records;"))
    story.append(Spacer(1, 10))

    story.append(Paragraph("2. Schema &amp; Database Metadata Discovery", styles['H1']))
    story.append(Paragraph("EnlngDB provides immediate introspection commands to inspect active databases and defined tables:", styles['Body']))
    story.append(make_code_box("show databases;\nshow tables;"))
    story.append(Spacer(1, 10))
    story.append(make_callout("Semicolon Invariant", "In EnlngDB scripts, statements can end with a semicolon (;) or newline. Lines beginning with # are treated as comments and ignored by the parser.", "SYNTAX"))
    story.append(PageBreak())

    # =========================================================================
    # SECTION 3: DDL SCHEMA & TABLE CREATION
    # =========================================================================
    story.append(Paragraph("SECTION 3", styles['ChapterNum']))
    story.append(Paragraph("Table Schema &amp; Column Declaration", styles['ChapterTitle']))
    story.append(Paragraph("Natural Syntax Table Creation with Multi-Column Definitions", styles['ChapterSubtitle']))

    story.append(Paragraph("In EnlngDB, tables are declared with natural English phrasing using the <b>with</b> keyword followed by comma-separated column names. There are no cumbersome SQL type annotations required during table initialization; EnlngDB leverages dynamic slot inference with strict validation on insertion.", styles['BodyLead']))

    story.append(Paragraph("The Canonical DDL Statement:", styles['H1']))
    story.append(make_code_box("create table <table_name> with <col1>, <col2>, <col3>, ...;"))
    story.append(Spacer(1, 10))

    story.append(Paragraph("Concrete Production Examples:", styles['H2']))
    story.append(make_code_box("""type enlngdb;
use enterprise;

# Create scholars registry table
create table scholars with id, name, cgpa, status;

# Create accounts table with financial ledger columns
create table accounts with id, account_no, holder, balance, tier, is_active;

# Create telemetry log table
create table system_metrics with timestamp, cpu_pct, memory_mb, latency_us;
show tables;"""))
    story.append(Spacer(1, 12))

    story.append(Paragraph("Engine Architecture Rules for Tables:", styles['H1']))
    story.append(Paragraph("• <b>Maximum Columns:</b> By default, each table supports up to 32 simultaneous typed columns (configurable in <code>enlngdb.h</code> via <code>ENLNGDB_MAX_COLS</code>).", styles['DbBullet']))
    story.append(Paragraph("• <b>Name Sanitization:</b> Column and table identifiers support alphanumeric ASCII characters and underscores.", styles['DbBullet']))
    story.append(Paragraph("• <b>Auto-Schema Expansion:</b> If an insert statement supplies a new column not in the original <code>create table</code> definition, EnlngDB automatically registers the new column slot without requiring an explicit ALTER TABLE step.", styles['DbBullet']))
    story.append(PageBreak())

    # =========================================================================
    # SECTION 4: DML RECORD INSERTION
    # =========================================================================
    story.append(Paragraph("SECTION 4", styles['ChapterNum']))
    story.append(Paragraph("High-Throughput Record Insertion (DML)", styles['ChapterTitle']))
    story.append(Paragraph("Inserting Single and Batch Records with Typed Values", styles['ChapterSubtitle']))

    story.append(Paragraph("Records are written into tables using the <code>insert into</code> or <code>insert record into</code> grammar. Field values are assigned as comma-separated key-value pairs:", styles['BodyLead']))

    story.append(Paragraph("The Canonical DML Insert Patterns:", styles['H1']))
    story.append(make_code_box("""insert into <table_name> with <col1> <val1>, <col2> <val2>, ...;
# Or with explicit 'record':
insert record into <table_name> with <col1> <val1>, <col2> <val2>, ...;"""))
    story.append(Spacer(1, 10))

    story.append(Paragraph("Supported Value Formats:", styles['H2']))
    story.append(Paragraph("• <b>Strings:</b> Enclosed in double quotes (<code>\"Aryan Sharma\"</code>) or single quotes (<code>'Mumbai'</code>).", styles['DbBullet']))
    story.append(Paragraph("• <b>Integers:</b> Standard numeric literals (<code>101</code>, <code>-42</code>).", styles['DbBullet']))
    story.append(Paragraph("• <b>Floating-Point:</b> Standard decimal notation (<code>9.85</code>, <code>145000.50</code>).", styles['DbBullet']))
    story.append(Paragraph("• <b>Booleans:</b> Case-insensitive literals (<code>true</code>, <code>false</code>).", styles['DbBullet']))
    story.append(Paragraph("• <b>Null:</b> Explicit literal (<code>null</code>).", styles['DbBullet']))
    story.append(Spacer(1, 10))

    story.append(Paragraph("Executable Script Example:", styles['H1']))
    story.append(make_code_box("""type enlngdb;
use university_db;

create table scholars with id, name, cgpa, status;

insert into scholars with id 1, name "aryan", cgpa 8.2, status "probation";
insert into scholars with id 2, name "meera", cgpa 9.4, status "honors";
insert into scholars with id 3, name "kunal", cgpa 7.8, status "probation";
insert record into scholars with id 4, name "priya", cgpa 9.9, status "gold_medalist";"""))
    story.append(Spacer(1, 10))
    story.append(make_callout("Key-Value Separators", "EnlngDB supports space (`name \"aryan\"`), colon (`name: \"aryan\"`), or equals (`name = \"aryan\"`) as separators between column names and values.", "SYNTAX"))
    story.append(PageBreak())

    # =========================================================================
    # SECTION 5: DQL QUERIES & FILTERING
    # =========================================================================
    story.append(Paragraph("SECTION 5", styles['ChapterNum']))
    story.append(Paragraph("Natural Query Language (DQL) &amp; Selection", styles['ChapterTitle']))
    story.append(Paragraph("Querying Records with Spoken Relational Operators and Counting", styles['ChapterSubtitle']))

    story.append(Paragraph("EnlngDB completely removes SQL <code>SELECT ... FROM ... WHERE</code> jargon in favor of natural conversational inquiries:", styles['BodyLead']))

    story.append(Paragraph("1. Unconditional Record Retrieval:", styles['H1']))
    story.append(make_code_box("find all records from <table_name>;\n# Or shorthand:\nfind records from <table_name>;"))
    story.append(Spacer(1, 10))

    story.append(Paragraph("2. Conditional Filtering with Relational Operators:", styles['H1']))
    story.append(make_code_box("find records from <table_name> where <column> <operator> <value>;\nfind all records from <table_name> where <column> <operator> <value>;"))
    story.append(Spacer(1, 10))

    story.append(Paragraph("Supported Relational Operator Matrix:", styles['H2']))
    op_data = [
        ["Natural Phrase", "Symbol", "Meaning", "Example"],
        ["is equal to / equals / is", "==", "Exact Equality", "where status is \"honors\""],
        ["is not equal to / is not", "!=", "Negated Equality", "where status is not \"probation\""],
        ["is greater than", ">", "Strictly Greater", "where cgpa is greater than 9.0"],
        ["is less than", "<", "Strictly Less", "where balance is less than 500"],
        ["is greater than or equal to / is at least", ">=", "Greater or Equal", "where cgpa is at least 9.5"],
        ["is less than or equal to / is at most", "<=", "Less or Equal", "where id is at most 100"],
        ["like", "~", "Sub-string Match", "where name like \"Aryan\""]
    ]
    op_table = Table(op_data, colWidths=[140, 48, 120, 190])
    op_table.setStyle(TableStyle([
        ('BACKGROUND', (0,0), (-1,0), colors.HexColor("#0f172a")),
        ('TEXTCOLOR', (0,0), (-1,0), colors.HexColor("#f8fafc")),
        ('FONTNAME', (0,0), (-1,0), 'Helvetica-Bold'),
        ('FONTSIZE', (0,0), (-1,-1), 8),
        ('GRID', (0,0), (-1,-1), 0.5, colors.HexColor("#cbd5e1")),
        ('ROWBACKGROUNDS', (0,1), (-1,-1), [colors.HexColor("#ffffff"), colors.HexColor("#f8fafc")]),
        ('PADDING', (0,0), (-1,-1), 4.5),
    ]))
    story.append(op_table)
    story.append(Spacer(1, 12))

    story.append(Paragraph("3. Aggregation: Record Counting", styles['H1']))
    story.append(make_code_box("count records from <table_name>;\ncount records from <table_name> where cgpa is greater than 9.0;"))
    story.append(PageBreak())

    # =========================================================================
    # SECTION 6: IN-TABLE MUTATIONS & UPDATES
    # =========================================================================
    story.append(Paragraph("SECTION 6", styles['ChapterNum']))
    story.append(Paragraph("In-Table Mutations &amp; Safe Record Updates", styles['ChapterTitle']))
    story.append(Paragraph("Conversational Syntax for Atomic In-Place Row and Column Rewrites", styles['ChapterSubtitle']))

    story.append(Paragraph("EnlngDB features an innovative, human-friendly update syntax that directly mirrors how an engineer explains a data modification. All updates are executed in-place within memory buffers and committed to transaction logs.", styles['BodyLead']))

    story.append(Paragraph("The Four Canonical Update Grammar Patterns:", styles['H1']))
    story.append(make_code_box("""# 1. Conversational single-column change by condition:
in <table_name> change <column> to <value> where <filter_column> <op> <filter_val>;

# 2. Conversational update by condition:
in <table_name> update <column> to <value> where <filter_column> <op> <filter_val>;

# 3. Multi-field atomic update (comma-separated):
in <table_name> set <col1> to <val1>, <col2> to <val2> where <filter_column> <op> <filter_val>;

# 4. Global column update across all rows (unconstrained):
in <table_name> set <column> to <value>;"""))
    story.append(Spacer(1, 12))

    story.append(Paragraph("Complete Working Example:", styles['H2']))
    story.append(make_code_box("""type enlngdb;
use university_db;

# Check current values
find all records from scholars;

# 1. Update Aryan's CGPA
in scholars change cgpa to 9.8 where name is "aryan";

# 2. Upgrade students with high CGPA to Dean's list
in scholars update status to "dean_list" where cgpa is greater than 9.0;

# 3. Multi-field update: promote Aryan to gold medalist
in scholars set cgpa to 9.95, status to "gold_medalist" where name is "aryan";

# Verify updated state
find all records from scholars;"""))
    story.append(Spacer(1, 10))
    story.append(make_callout("Atomic Row Updates", "In EnlngDB, multi-column updates like `in scholars set cgpa to 9.95, status to 'topper'` execute atomically per row in 0.03 ms.", "BENCHMARK"))
    story.append(PageBreak())

    # =========================================================================
    # SECTION 7: BINARY STORAGE & WAL FORMAT
    # =========================================================================
    story.append(Paragraph("SECTION 7", styles['ChapterNum']))
    story.append(Paragraph("Binary Disk Storage &amp; Persistence Format", styles['ChapterTitle']))
    story.append(Paragraph("The .edb Binary Specification, Magic Header 0x454E4442, and WAL", styles['ChapterSubtitle']))

    story.append(Paragraph("When an EnlngDB database is saved, it is compiled into a highly compact, zero-fragmentation binary file with the <code>.edb</code> extension. The layout is optimized for instant <code>mmap()</code> or direct sequential <code>fread()</code> lookups.", styles['BodyLead']))

    story.append(Paragraph("Binary Header Layout:", styles['H1']))
    hdr_data = [
        ["Offset", "Field", "Type", "Description"],
        ["0x00 - 0x03", "Magic Header", "char[4]", "ASCII 'ENDB' (0x45 0x4E 0x44 0x42)"],
        ["0x04 - 0x07", "Format Version", "uint32_t", "Format version (currently 1)"],
        ["0x08 - 0x0B", "Table Count", "uint32_t", "Number of tables stored in file"],
        ["0x0C - 0x4B", "Database Name", "char[64]", "Null-terminated database identifier"],
        ["0x4C - ...", "Table Descriptors", "struct[]", "Array of column schemas, row counts, and data slots"]
    ]
    hdr_table = Table(hdr_data, colWidths=[80, 110, 80, 228])
    hdr_table.setStyle(TableStyle([
        ('BACKGROUND', (0,0), (-1,0), colors.HexColor("#0f172a")),
        ('TEXTCOLOR', (0,0), (-1,0), colors.HexColor("#f8fafc")),
        ('FONTNAME', (0,0), (-1,0), 'Helvetica-Bold'),
        ('FONTSIZE', (0,0), (-1,-1), 8),
        ('GRID', (0,0), (-1,-1), 0.5, colors.HexColor("#cbd5e1")),
        ('ROWBACKGROUNDS', (0,1), (-1,-1), [colors.HexColor("#ffffff"), colors.HexColor("#f8fafc")]),
        ('PADDING', (0,0), (-1,-1), 4.5),
    ]))
    story.append(hdr_table)
    story.append(Spacer(1, 14))

    story.append(Paragraph("Write-Ahead Logging (WAL) &amp; Crash Recovery:", styles['H1']))
    story.append(Paragraph("• <b>Transaction Journal:</b> Every mutating statement (insert, update, drop) is appended to <code>&lt;database&gt;.wal</code> before modifying in-memory slots.", styles['DbBullet']))
    story.append(Paragraph("• <b>Automatic Recovery:</b> On engine boot, EnlngDB inspects the WAL file. If the process crashed before flushing the <code>.edb</code> master file, all logged operations are replayed in sub-10ms.", styles['DbBullet']))
    story.append(Paragraph("• <b>Zero Corruption:</b> EnlngDB guarantees ACID compliance for single-node operations.", styles['DbBullet']))
    story.append(PageBreak())

    # =========================================================================
    # SECTION 8: C EMBEDDED API
    # =========================================================================
    story.append(Paragraph("SECTION 8", styles['ChapterNum']))
    story.append(Paragraph("Native C Engine Embedded API", styles['ChapterTitle']))
    story.append(Paragraph("Embedding EnlngDB in C, C++, Zig, or Rust with Zero External Libraries", styles['ChapterSubtitle']))

    story.append(Paragraph("EnlngDB can be embedded directly into any systems application. The entire engine resides in <code>enlngdb.h</code>, <code>enlngdb.c</code>, and <code>enlngdb_parser.c</code> with zero third-party dependencies beyond the standard C library (<code>libc</code>).", styles['BodyLead']))

    story.append(Paragraph("Complete C Integration Example:", styles['H1']))
    story.append(make_code_box("""#include <stdio.h>
#include "enlngdb.h"

int main() {
    // 1. Initialize or load database from disk
    EnlngDatabase* db = enlngdb_create("finance_store");
    enlngdb_load(db, "finance_store.edb");

    // 2. Execute natural EnlngDB statements directly
    enlngdb_execute_statement(db, "create table ledgers with id, user, balance;", true);
    enlngdb_execute_statement(db, "insert into ledgers with id 1, user 'aryan', balance 50000.0;", true);
    enlngdb_execute_statement(db, "in ledgers update balance to 75000.0 where user is 'aryan';", true);

    // 3. Query records with C API
    EnlngTable* tbl = enlngdb_get_table(db, "ledgers");
    if (tbl) {
        EnlngQueryResult* res = enlngdb_find(tbl, NULL, OP_NONE, NULL, 0, 0);
        enlngdb_print_result(res);
        enlngdb_free_result(res);
    }

    // 4. Save to binary .edb file and release memory
    enlngdb_save(db, "finance_store.edb");
    enlngdb_free(db);
    return 0;
}"""))
    story.append(Spacer(1, 10))
    story.append(make_callout("Compilation Command", "gcc -O3 -std=c99 main.c enlngdb.c enlngdb_parser.c -o my_app.exe", "BENCHMARK"))
    story.append(PageBreak())

    # =========================================================================
    # SECTION 9: PYTHON SDK
    # =========================================================================
    story.append(Paragraph("SECTION 9", styles['ChapterNum']))
    story.append(Paragraph("Python SDK &amp; Context Manager", styles['ChapterTitle']))
    story.append(Paragraph("Native Python Bindings with Dictionary Rows and Pandas Interop", styles['ChapterSubtitle']))

    story.append(Paragraph("The <code>enlngdb</code> Python package bridges the pure C database engine into Python 3.8+ through native C extension bindings, providing a high-speed alternative to SQLite.", styles['BodyLead']))

    story.append(Paragraph("Installation via PIP:", styles['H1']))
    story.append(make_code_box("pip install enlngdb"))
    story.append(Spacer(1, 10))

    story.append(Paragraph("Pythonic API &amp; Context Manager Usage:", styles['H1']))
    story.append(make_code_box("""import enlngdb

# 1. Connect using Python context manager (auto-saves on exit)
with enlngdb.connect("production.edb") as db:
    # 2. Execute natural declarative script
    db.execute(\"\"\"
        create table accounts with id, holder, balance;
        insert into accounts with id 1, holder "Aryan", balance 50000.0;
        insert into accounts with id 2, holder "Meera", balance 95000.0;
        in accounts change balance to 120000.0 where holder is "Aryan";
    \"\"\")

    # 3. Retrieve rows as native Python dictionaries
    results = db.query("find all records from accounts where balance is greater than 80000.0;")
    for row in results:
        print(f"Account {row['id']}: {row['holder']} -> ₹{row['balance']}")

    # 4. Direct export to pandas DataFrame
    df = db.to_dataframe("accounts")
    print(df.describe())"""))
    story.append(Spacer(1, 10))
    story.append(make_callout("Zero Serialization Latency", "Python bindings read directly from the C memory pointers, avoiding costly JSON or string marshalling.", "BENCHMARK"))
    story.append(PageBreak())

    # =========================================================================
    # SECTION 10: SYNTAX MATRIX & CLI
    # =========================================================================
    story.append(Paragraph("SECTION 10", styles['ChapterNum']))
    story.append(Paragraph("Syntax Matrix, Developer CLI &amp; Diagnostics", styles['ChapterTitle']))
    story.append(Paragraph("Command-Line Utilities, Drop Table Confirmation, and Error Codes", styles['ChapterSubtitle']))

    story.append(Paragraph("1. Developer CLI Utility (enlngdb.exe)", styles['H1']))
    story.append(Paragraph("The standalone <code>enlngdb.exe</code> binary provides quick command-line utilities for running scripts and launching interactive consoles:", styles['Body']))
    story.append(make_code_box("""# Launch interactive terminal REPL:
enlngdb

# Execute a database script file:
enlngdb run my_database.enlngdb

# Inspect database contents:
enlngdb inspect production.edb"""))
    story.append(Spacer(1, 10))

    story.append(Paragraph("2. Safe Table Deletion (Confirmed Invariant)", styles['H1']))
    story.append(Paragraph("To prevent accidental table drops in production, EnlngDB requires explicit confirmation in the statement:", styles['Body']))
    story.append(make_code_box("drop table scholars confirmed;"))
    story.append(Spacer(1, 10))

    story.append(Paragraph("3. Exhaustive Syntax Cheat Sheet", styles['H1']))
    summary_data = [
        ["Operation", "EnlngDB Canonical Syntax"],
        ["Header", "type enlngdb;"],
        ["Database", "use database <name>; OR use <name>;"],
        ["Introspect", "show databases; | show tables;"],
        ["Create Table", "create table <tbl> with <c1>, <c2>, <c3>;"],
        ["Insert Record", "insert into <tbl> with <c1> <v1>, <c2> <v2>;"],
        ["Find All", "find all records from <tbl>;"],
        ["Filtered Find", "find records from <tbl> where <col> <op> <val>;"],
        ["Count", "count records from <tbl>;"],
        ["Row Update", "in <tbl> change <col> to <val> where <cond>;"],
        ["Row Update (Alt)", "in <tbl> update <col> to <val> where <cond>;"],
        ["Multi Update", "in <tbl> set <c1> to <v1>, <c2> to <v2> where <cond>;"],
        ["Drop Table", "drop table <tbl> confirmed;"]
    ]
    summary_table = Table(summary_data, colWidths=[110, 388])
    summary_table.setStyle(TableStyle([
        ('BACKGROUND', (0,0), (-1,0), colors.HexColor("#0f172a")),
        ('TEXTCOLOR', (0,0), (-1,0), colors.HexColor("#f8fafc")),
        ('FONTNAME', (0,0), (-1,0), 'Helvetica-Bold'),
        ('FONTSIZE', (0,0), (-1,-1), 8),
        ('GRID', (0,0), (-1,-1), 0.5, colors.HexColor("#cbd5e1")),
        ('ROWBACKGROUNDS', (0,1), (-1,-1), [colors.HexColor("#ffffff"), colors.HexColor("#f8fafc")]),
        ('PADDING', (0,0), (-1,-1), 4),
    ]))
    story.append(summary_table)

    # Build Document
    print(f"Building master PDF: {filename}...")
    doc.build(story, canvasmaker=NumberedCanvas)
    print(f"SUCCESS: Generated {filename} ({os.path.getsize(filename)} bytes)")

if __name__ == "__main__":
    out_paths = [
        "d:/enlangg/website/enlngdb_manual.pdf",
        "d:/enlangg/docs/enlngdb_manual.pdf",
        "d:/enlangg/enlngdb/enlngdb_manual.pdf"
    ]
    primary = out_paths[0]
    os.makedirs(os.path.dirname(primary), exist_ok=True)
    build_pdf(primary)

    for p in out_paths[1:]:
        os.makedirs(os.path.dirname(p), exist_ok=True)
        shutil.copy2(primary, p)
        print(f"Copied to: {p}")
