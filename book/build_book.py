"""Build the Enlangg manual PDF.

Source files live in book/src/*.txt. Format (one chapter per file):

    # Chapter title                 (first line)
    ## Section title
    plain paragraph lines...        (blank line ends a paragraph)
    - bullet
    | a | b |                       (table rows; first row is the header)
    ::: code                        (program, run with enlng; output captured)
    ...
    :::
    ::: code stdin=3\\n4            (same, with stdin)
    ::: src                         (shown only, never run)
    ::: sh                          (shell text, shown only)
    ::: run <tool args> <<< file    (not used)

Every `code` block is executed with the real toolchain, so the output printed
in the book is what the tool produced.

Usage: python book/build_book.py [--no-run]
"""
import glob
import hashlib
import json
import os
import re
import subprocess
import sys
import tempfile

from reportlab.lib import colors
from reportlab.lib.enums import TA_LEFT
from reportlab.lib.pagesizes import A5
from reportlab.lib.styles import ParagraphStyle
from reportlab.lib.units import mm

SIDE, TOPB, BOTB = 15, 14.5, 16.5
from reportlab.pdfbase import pdfmetrics
from reportlab.pdfbase.ttfonts import TTFont
from reportlab.platypus import (BaseDocTemplate, Frame, KeepTogether, PageBreak, PageTemplate,
                                Paragraph, Preformatted, Spacer, Table, TableStyle, XPreformatted)
from reportlab.platypus.tableofcontents import TableOfContents

NL = chr(10)
CRLF = chr(13) + chr(10)
BSL = chr(92)
HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
ENLNG = r'D:/enlangg/scratch/w551/bin/enlng.exe'
CACHE = os.path.join(HERE, '.runcache.json')
OUT = os.path.join(HERE, 'out', 'enlangg_the_enlng.pdf')
os.makedirs(os.path.dirname(OUT), exist_ok=True)
VERSION = '5.5.1'

F = 'C:/Windows/Fonts/'
pdfmetrics.registerFont(TTFont('Body', F + 'georgia.ttf'))
pdfmetrics.registerFont(TTFont('Body-B', F + 'georgiab.ttf'))
pdfmetrics.registerFont(TTFont('Body-I', F + 'georgiai.ttf'))
pdfmetrics.registerFont(TTFont('Body-BI', F + 'georgiaz.ttf'))
pdfmetrics.registerFontFamily('Body', normal='Body', bold='Body-B', italic='Body-I', boldItalic='Body-BI')
pdfmetrics.registerFont(TTFont('Sans', F + 'segoeui.ttf'))
pdfmetrics.registerFont(TTFont('Sans-B', F + 'segoeuib.ttf'))
pdfmetrics.registerFont(TTFont('Mono', F + 'consola.ttf'))
pdfmetrics.registerFont(TTFont('Mono-B', F + 'consolab.ttf'))
pdfmetrics.registerFontFamily('Mono', normal='Mono', bold='Mono-B', italic='Mono', boldItalic='Mono-B')

INK = colors.HexColor('#1b1f24')
MUTED = colors.HexColor('#5a636e')
ACCENT = colors.HexColor('#0b6b4d')
CODEBG = colors.HexColor('#f3f5f7')
OUTBG = colors.HexColor('#e9f3ee')
RULE = colors.HexColor('#c9d1d9')

S = {
    'body': ParagraphStyle('body', fontName='Body', fontSize=9.6, leading=13.4, textColor=INK, spaceAfter=5, alignment=TA_LEFT),
    'h1': ParagraphStyle('h1', fontName='Sans-B', fontSize=21, leading=25, textColor=ACCENT, spaceBefore=0, spaceAfter=10),
    'h3': ParagraphStyle('h3', fontName='Sans-B', fontSize=10.5, leading=14, textColor=INK, spaceBefore=8, spaceAfter=3),
    'h2': ParagraphStyle('h2', fontName='Sans-B', fontSize=12.5, leading=16, textColor=INK, spaceBefore=10, spaceAfter=4),
    'bullet': ParagraphStyle('bullet', fontName='Body', fontSize=9.6, leading=14, textColor=INK, leftIndent=13, bulletIndent=2, spaceAfter=2.5),
    'code': ParagraphStyle('code', fontName='Mono', fontSize=7.9, leading=9.8, textColor=INK, backColor=CODEBG, borderPadding=(5, 6, 5, 6), leftIndent=6, rightIndent=6, spaceBefore=3, spaceAfter=6),
    'out': ParagraphStyle('out', fontName='Mono', fontSize=7.9, leading=9.8, textColor=colors.HexColor('#12372a'), backColor=OUTBG, borderPadding=(5, 6, 5, 6), leftIndent=6, rightIndent=6, spaceBefore=2, spaceAfter=7),
    'label': ParagraphStyle('label', fontName='Sans-B', fontSize=7, leading=9, textColor=MUTED, spaceBefore=3, spaceAfter=0),
    'cell': ParagraphStyle('cell', fontName='Body', fontSize=8.6, leading=11.5, textColor=INK),
    'cellh': ParagraphStyle('cellh', fontName='Sans-B', fontSize=8.2, leading=11, textColor=colors.white),
    'toc1': ParagraphStyle('toc1', fontName='Sans', fontSize=9.4, leading=14, leftIndent=0),
    'tocT': ParagraphStyle('tocT', fontName='Sans-B', fontSize=18, textColor=ACCENT, spaceAfter=10),
}

WRAP = 66  # max characters per code line before manual wrap


def esc(t):
    return t.replace('&', '&amp;').replace('<', '&lt;').replace('>', '&gt;')


def inline(t):
    t = esc(t)
    t = re.sub(r'`([^`]+)`', r'<font name="Mono" size="8.6" color="#0b4f3a">\1</font>', t)
    t = re.sub(r'\*\*([^*]+)\*\*', r'<b>\1</b>', t)
    t = re.sub(r'(?<![\w*])_([^_]+)_(?![\w])', r'<i>\1</i>', t)
    return t


def wrap_lines(text):
    out = []
    for line in text.rstrip('\n').split('\n'):
        line = line.rstrip('\r').expandtabs(4)
        while len(line) > WRAP:
            out.append(line[:WRAP] + '\u21b5')
            line = '    ' + line[WRAP:]
        out.append(line)
    return '\n'.join(out)


def load_cache():
    try:
        return json.load(open(CACHE, encoding='utf-8'))
    except Exception:
        return {}


def run_program(src, stdin, cache, no_run):
    key = hashlib.sha256((VERSION + '\0' + src + '\0' + stdin).encode('utf-8')).hexdigest()
    if key in cache:
        return cache[key]
    if no_run:
        return {'out': '(not run)', 'code': 0}
    with tempfile.TemporaryDirectory() as d:
        path = os.path.join(d, 'prog.enlng')
        open(path, 'w', encoding='utf-8', newline='\n').write(src)
        try:
            r = subprocess.run([ENLNG, 'run', 'prog.enlng'], cwd=d, input=stdin.encode('utf-8'),
                               capture_output=True, timeout=20)
            out = (r.stdout + r.stderr).decode('utf-8', 'replace').replace('\r\n', '\n')
            code = r.returncode
        except subprocess.TimeoutExpired:
            out, code = '(timed out)', 124
    out = out.replace('prog.enlng', 'program.enlng')
    cache[key] = {'out': out, 'code': code}
    return cache[key]


def run_cmds(body, cache, no_run, files=None):
    """Run shell-style lines such as `enlng --version`; return text with real output."""
    key = hashlib.sha256((VERSION + 'cmd|' + body + repr(files)).encode()).hexdigest()
    if key in cache:
        return cache[key]['out']
    if no_run:
        return body
    env = dict(os.environ)
    env['PATH'] = os.path.dirname(ENLNG) + os.pathsep + env['PATH']
    out = []
    with tempfile.TemporaryDirectory() as d:
        for name, content in (files or {}).items():
            os.makedirs(os.path.dirname(os.path.join(d, name)), exist_ok=True)
            open(os.path.join(d, name), 'w', encoding='utf-8', newline=NL).write(content)
        for line in body.split(NL):
            if not line.strip():
                continue
            out.append('$ ' + line)
            try:
                r = subprocess.run(line, shell=True, cwd=d, env=env, capture_output=True, timeout=30)
                t = (r.stdout + r.stderr).decode('utf-8', 'replace').replace(CRLF, NL).rstrip(NL)
            except subprocess.TimeoutExpired:
                t = '(timed out)'
            t = t.replace(d, '<dir>').replace(d.replace(BSL, '/'), '<dir>')
            if t:
                out.append(t)
    text = NL.join(out)
    cache[key] = {'out': text, 'code': 0}
    return text


def parse_chapter(path):
    lines = open(path, encoding='utf-8').read().replace('\r\n', '\n').split('\n')
    title = lines[0].lstrip('# ').strip()
    blocks, i, para, table = [], 1, [], []

    def flush():
        nonlocal para, table
        if para:
            blocks.append(('p', ' '.join(para)))
            para = []
        if table:
            blocks.append(('table', table))
            table = []

    while i < len(lines):
        ln = lines[i]
        if ln.startswith('::: '):
            flush()
            kind = ln[4:].strip()
            body = []
            i += 1
            while i < len(lines) and lines[i] != ':::':
                body.append(lines[i])
                i += 1
            blocks.append(('block', kind, '\n'.join(body)))
        elif ln.startswith('### '):
            flush()
            blocks.append(('h3', ln[4:].strip()))
        elif ln.startswith('## '):
            flush()
            blocks.append(('h2', ln[3:].strip()))
        elif ln.startswith('- '):
            flush()
            blocks.append(('li', ln[2:].strip()))
        elif ln.startswith('|'):
            if para:
                flush()
            table.append([c.strip() for c in ln.strip().strip('|').split('|')])
        elif ln.strip() == '':
            flush()
        else:
            if table:
                flush()
            para.append(ln.strip())
        i += 1
    flush()
    return title, blocks


class Doc(BaseDocTemplate):
    def __init__(self, fn, **kw):
        super().__init__(fn, pagesize=A5, leftMargin=SIDE * mm, rightMargin=SIDE * mm, topMargin=TOPB * mm, bottomMargin=BOTB * mm,
                         title='Enlangg: The Enlng Book', author='Enlangg project', subject='Reference manual, toolchain ' + VERSION, **kw)
        fr = Frame(self.leftMargin, self.bottomMargin, self.width, self.height, id='f', leftPadding=0, rightPadding=0, topPadding=0, bottomPadding=0)
        self.addPageTemplates([PageTemplate(id='main', frames=[fr], onPage=self.decorate)])
        self.chapter_no = 0

    def decorate(self, canv, doc):
        canv.saveState()
        canv.setFont('Sans', 7.5)
        canv.setFillColor(MUTED)
        canv.drawString(doc.leftMargin, 9 * mm, 'Enlangg: The Enlng Book  ·  toolchain ' + VERSION)
        canv.drawRightString(A5[0] - doc.rightMargin, 9 * mm, str(doc.page))
        canv.setStrokeColor(RULE)
        canv.setLineWidth(0.4)
        canv.line(doc.leftMargin, 12.5 * mm, A5[0] - doc.rightMargin, 12.5 * mm)
        canv.restoreState()

    def afterFlowable(self, fl):
        if getattr(fl, '_toc', None):
            level, text = fl._toc
            self.notify('TOCEntry', (level, text, self.page))
            self.canv.bookmarkPage(text)
            self.canv.addOutlineEntry(text, text, level)


def chapter_heading(num, title):
    label = ('Chapter %d' % num) if isinstance(num, int) else num
    p = Paragraph('<font name="Sans" size="8.5" color="#5a636e">%s</font><br/>%s' % (esc(label.upper()), esc(title)), S['h1'])
    p._toc = (0, ('%d. %s' % (num, title)) if isinstance(num, int) else title)
    return p


def code_flow(text, style, label=None):
    items = []
    if label:
        items.append(Paragraph(label, S['label']))
    lines = wrap_lines(text).split('\n')
    # split long blocks so they can break across pages
    chunk = 46
    for k in range(0, len(lines), chunk):
        items.append(Preformatted('\n'.join(lines[k:k + chunk]), S[style]))
    return items


def build(no_run=False):
    cache = load_cache()
    files = sorted(glob.glob(os.path.join(HERE, 'src', '*.txt')))
    story = []
    toc = TableOfContents()
    toc.levelStyles = [S['toc1']]
    story.append(Spacer(1, 38 * mm))
    story.append(Paragraph('<font name="Sans-B" size="30" color="#0b6b4d">Enlangg</font>', ParagraphStyle('c1', parent=S['body'], leading=36)))
    story.append(Paragraph('<font name="Sans-B" size="17">The Enlng Book</font>', ParagraphStyle('c2', parent=S['body'], leading=22)))
    story.append(Spacer(1, 6 * mm))
    story.append(Paragraph('A reference manual for the Enlangg language and its seven tools. Every program in this book was run with toolchain %s, and the output shown is what it printed.' % VERSION, S['body']))
    story.append(Spacer(1, 70 * mm))
    story.append(Paragraph('<font name="Sans" size="8" color="#5a636e">github.com/Aero99op/enlang-main  ·  MIT License</font>', S['body']))
    story.append(PageBreak())
    story.append(Paragraph('Contents', S['tocT']))
    story.append(toc)
    story.append(PageBreak())

    failures = []
    for n, path in enumerate(files, 1):
        title, blocks = parse_chapter(path)
        base = os.path.basename(path)
        part = base.startswith('P')
        story.append(chapter_heading(n if not part else title, title))
        if part:
            story.pop()
            story.append(chapter_heading('Part', title))
        for b in blocks:
            if b[0] == 'p':
                story.append(Paragraph(inline(b[1]), S['body']))
            elif b[0] == 'h2':
                h = Paragraph(esc(b[1]), S['h2'])
                story.append(h)
            elif b[0] == 'h3':
                story.append(Paragraph(esc(b[1]), S['h3']))
            elif b[0] == 'li':
                story.append(Paragraph(inline(b[1]), S['bullet'], bulletText='\u2022'))
            elif b[0] == 'table':
                rows = b[1]
                if len(rows) > 1 and all(set(c) <= set('-: ') for c in rows[1]):
                    rows = [rows[0]] + rows[2:]
                ncol = len(rows[0])
                data = [[Paragraph(inline(c), S['cellh'] if r == 0 else S['cell']) for c in row + [''] * (ncol - len(row))] for r, row in enumerate(rows)]
                avail = A5[0] - 2 * SIDE * mm
                lens = [max(len(r[c]) if c < len(r) else 0 for r in rows) for c in range(ncol)]
                wts = [min(max(l, 6), 60) ** 0.8 for l in lens]
                colw = [avail * w / sum(wts) for w in wts]
                t = Table(data, colWidths=colw, repeatRows=1, hAlign='LEFT')
                t.setStyle(TableStyle([('BACKGROUND', (0, 0), (-1, 0), ACCENT), ('GRID', (0, 0), (-1, -1), 0.3, RULE),
                                       ('VALIGN', (0, 0), (-1, -1), 'TOP'), ('ROWBACKGROUNDS', (0, 1), (-1, -1), [colors.white, CODEBG]),
                                       ('LEFTPADDING', (0, 0), (-1, -1), 4), ('RIGHTPADDING', (0, 0), (-1, -1), 4),
                                       ('TOPPADDING', (0, 0), (-1, -1), 2.5), ('BOTTOMPADDING', (0, 0), (-1, -1), 2.5)]))
                story.append(t)
                story.append(Spacer(1, 7))
            elif b[0] == 'block':
                kind, body = b[1], b[2]
                kparts = kind.split(' ', 1)
                k = kparts[0]
                stdin = ''
                if len(kparts) > 1 and kparts[1].startswith('stdin='):
                    stdin = kparts[1][6:].replace('\\n', '\n') + '\n'
                if k == 'code':
                    res = run_program(body, stdin, cache, no_run)
                    if res['code'] not in (0,) and '-- Enlang' not in res['out'] and 'expect-fail' not in kind:
                        failures.append((base, body[:60], res))
                    story.extend(code_flow(body, 'code', 'PROGRAM'))
                    story.extend(code_flow(res['out'].rstrip('\n') or '(no output)', 'out', 'OUTPUT'))
                elif k == 'err':
                    res = run_program(body, stdin, cache, no_run)
                    story.extend(code_flow(body, 'code', 'PROGRAM WITH A MISTAKE'))
                    story.extend(code_flow(res['out'].rstrip('\n') or '(no output)', 'out', 'WHAT enlng PRINTS'))
                elif k == 'cmd':
                    # optional files: "@@ name" lines start a file; a line "@@@" ends the files
                    files, cmds = {}, body
                    if body.startswith('@@ '):
                        head, cmds = body.split(NL + '@@@' + NL, 1)
                        for part in head.split('@@ ')[1:]:
                            nm, _, content = part.partition(NL)
                            files[nm.strip()] = content
                    story.extend(code_flow(run_cmds(cmds, cache, no_run, files), 'out', 'TERMINAL'))
                elif k == 'src':
                    story.extend(code_flow(body, 'code'))
                elif k == 'sh':
                    story.extend(code_flow(body, 'code', 'TERMINAL'))
                elif k == 'out':
                    story.extend(code_flow(body, 'out', 'OUTPUT'))
        story.append(PageBreak())

    json.dump(cache, open(CACHE, 'w', encoding='utf-8'))
    doc = Doc(OUT)
    doc.multiBuild(story)
    return failures, len(files)


if __name__ == '__main__':
    fails, n = build('--no-run' in sys.argv)
    from pypdf import PdfReader
    try:
        pages = len(PdfReader(OUT).pages)
    except Exception:
        pages = '?'
    print('chapters:', n, 'pages:', pages)
    for f in fails:
        print('FAILED RUN:', f[0], repr(f[1]), f[2]['out'][:200])
