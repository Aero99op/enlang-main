import os
from reportlab.lib import colors
from reportlab.lib.pagesizes import letter
from reportlab.lib.units import inch
from reportlab.lib.styles import getSampleStyleSheet, ParagraphStyle
from reportlab.platypus import (
    Paragraph, Spacer, Table, TableStyle, PageBreak, Preformatted, HRFlowable
)
from reportlab.pdfgen import canvas

class NumberedCanvas(canvas.Canvas):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self._saved_page_states = []

    def showPage(self):
        self._saved_page_states.append(dict(self.__dict__))
        self._startPage()

    def save(self):
        num_pages = len(self._saved_page_states)
        for state in self._saved_page_states:
            self.__dict__.update(state)
            self.draw_page_decorations(num_pages)
            super().showPage()
        super().save()

    def draw_page_decorations(self, page_count):
        if self._pageNumber == 1:
            return  # Suppress headers/footers on cover

        self.saveState()
        self.setFont("Helvetica-Bold", 8)
        self.setFillColor(colors.HexColor("#64748b"))

        width = 8.5 * inch
        height = 11.0 * inch
        left_margin = 54
        right_margin = width - 54

        # Running Header
        header_y = height - 36
        self.drawString(left_margin, header_y, "ENLNGDB: ZERO-SQL MANUAL — THE DEFINITIVE CANONICAL SPECIFICATION")
        self.drawRightString(right_margin, header_y, "MASTER REFERENCE")
        self.setStrokeColor(colors.HexColor("#cbd5e1"))
        self.setLineWidth(0.5)
        self.line(left_margin, header_y - 4, right_margin, header_y - 4)

        # Running Footer
        footer_y = 36
        self.line(left_margin, footer_y + 12, right_margin, footer_y + 12)
        page_str = f"Page {self._pageNumber} of {page_count}"
        if self._pageNumber % 2 == 0:
            self.drawString(left_margin, footer_y, "ENLANG FOUNDATION // SOVEREIGN ARCHITECTURAL COUNCIL")
            self.drawRightString(right_margin, footer_y, page_str)
        else:
            self.drawString(left_margin, footer_y, page_str)
            self.drawRightString(right_margin, footer_y, "ENLANG FOUNDATION // SOVEREIGN DATA LABS")

        self.restoreState()

def create_db_styles():
    styles = getSampleStyleSheet()

    styles.add(ParagraphStyle(
        'DbCoverSuper', fontName='Helvetica-Bold', fontSize=14, leading=18,
        textColor=colors.HexColor("#0284c7"), alignment=1, spaceAfter=18
    ))
    styles.add(ParagraphStyle(
        'DbCoverTitle', fontName='Helvetica-Bold', fontSize=34, leading=40,
        textColor=colors.HexColor("#0f172a"), alignment=1, spaceAfter=14
    ))
    styles.add(ParagraphStyle(
        'DbCoverSubtitle', fontName='Helvetica', fontSize=14, leading=19,
        textColor=colors.HexColor("#475569"), alignment=1, spaceAfter=28
    ))
    styles.add(ParagraphStyle(
        'DbCoverAuthor', fontName='Helvetica-Bold', fontSize=12, leading=16,
        textColor=colors.HexColor("#1e293b"), alignment=1, spaceAfter=6
    ))
    styles.add(ParagraphStyle(
        'DbCoverMeta', fontName='Helvetica', fontSize=9.5, leading=13.5,
        textColor=colors.HexColor("#64748b"), alignment=1
    ))

    # Part & Chapter Headers
    styles.add(ParagraphStyle(
        'DbPartRoman', fontName='Helvetica-Bold', fontSize=15, leading=19,
        textColor=colors.HexColor("#0284c7"), spaceAfter=8, keepWithNext=True
    ))
    styles.add(ParagraphStyle(
        'DbPartTitle', fontName='Helvetica-Bold', fontSize=24, leading=28,
        textColor=colors.HexColor("#0f172a"), spaceAfter=14, keepWithNext=True
    ))
    styles.add(ParagraphStyle(
        'DbPartEpigraph', fontName='Helvetica-Oblique', fontSize=10.5, leading=15,
        textColor=colors.HexColor("#475569"), spaceAfter=20, keepWithNext=True
    ))

    styles.add(ParagraphStyle(
        'DbChapterNum', fontName='Helvetica-Bold', fontSize=11, leading=15,
        textColor=colors.HexColor("#0284c7"), spaceAfter=4, keepWithNext=True
    ))
    styles.add(ParagraphStyle(
        'DbChapterHeading', fontName='Helvetica-Bold', fontSize=18, leading=22,
        textColor=colors.HexColor("#0f172a"), spaceAfter=6, keepWithNext=True
    ))
    styles.add(ParagraphStyle(
        'DbChapterSubHeading', fontName='Helvetica', fontSize=10, leading=14,
        textColor=colors.HexColor("#64748b"), spaceAfter=12, keepWithNext=True
    ))

    # Section Headers
    styles.add(ParagraphStyle(
        'DbH1', fontName='Helvetica-Bold', fontSize=12.5, leading=16,
        textColor=colors.HexColor("#0f172a"), spaceBefore=12, spaceAfter=6, keepWithNext=True
    ))
    styles.add(ParagraphStyle(
        'DbH2', fontName='Helvetica-Bold', fontSize=10.5, leading=14,
        textColor=colors.HexColor("#1e293b"), spaceBefore=9, spaceAfter=4, keepWithNext=True
    ))
    styles.add(ParagraphStyle(
        'DbH3', fontName='Helvetica-Bold', fontSize=9.5, leading=12.5,
        textColor=colors.HexColor("#334155"), spaceBefore=6, spaceAfter=3, keepWithNext=True
    ))

    # Body Prose
    styles.add(ParagraphStyle(
        'DbBody', fontName='Helvetica', fontSize=8.8, leading=12.5,
        textColor=colors.HexColor("#1e293b"), spaceAfter=6
    ))
    styles.add(ParagraphStyle(
        'DbBodyLead', fontName='Helvetica', fontSize=9.5, leading=13.5,
        textColor=colors.HexColor("#0f172a"), spaceAfter=7
    ))
    styles.add(ParagraphStyle(
        'DbBullet', fontName='Helvetica', fontSize=8.5, leading=12.0,
        textColor=colors.HexColor("#1e293b"), leftIndent=14, spaceAfter=3
    ))
    styles.add(ParagraphStyle(
        'DbTOCPart', fontName='Helvetica-Bold', fontSize=10, leading=13.5,
        textColor=colors.HexColor("#0284c7"), spaceBefore=6, spaceAfter=3
    ))
    styles.add(ParagraphStyle(
        'DbTOCLine', fontName='Helvetica', fontSize=8.0, leading=10.5,
        textColor=colors.HexColor("#1e293b"), leftIndent=10, spaceAfter=1.5
    ))

    return styles

def make_code_box(code_text):
    clean_code = code_text.strip("\r\n")
    lines = clean_code.split("\n")
    CHUNK_SIZE = 26
    flowables = []
    for i in range(0, len(lines), CHUNK_SIZE):
        chunk = "\n".join(lines[i:i+CHUNK_SIZE])
        p = Preformatted(
            chunk,
            ParagraphStyle(
                f'DbCodeFont_{i}', fontName='Courier', fontSize=7.4, leading=9.6,
                textColor=colors.HexColor("#f8fafc")
            )
        )
        t = Table([[p]], colWidths=[500])
        t.setStyle(TableStyle([
            ('BACKGROUND', (0,0), (-1,-1), colors.HexColor("#090d16")),
            ('BOX', (0,0), (-1,-1), 1, colors.HexColor("#1e293b")),
            ('TOPPADDING', (0,0), (-1,-1), 5),
            ('BOTTOMPADDING', (0,0), (-1,-1), 5),
            ('LEFTPADDING', (0,0), (-1,-1), 8),
            ('RIGHTPADDING', (0,0), (-1,-1), 8),
        ]))
        flowables.append(t)
        if i + CHUNK_SIZE < len(lines):
            flowables.append(Spacer(1, 4))
    return flowables

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
    elif callout_type == "ARCH":
        accent = colors.HexColor("#0ea5e9")
        bg = colors.HexColor("#f0fdf4")

    content = [
        Paragraph(f"<b>{callout_type}: {title}</b>", ParagraphStyle(
            'CallTitle', fontName='Helvetica-Bold', fontSize=8.8, leading=12.0,
            textColor=accent, spaceAfter=2
        )),
        Paragraph(text, ParagraphStyle(
            'CallBody', fontName='Helvetica', fontSize=8.2, leading=11.5,
            textColor=colors.HexColor("#1e293b")
        ))
    ]
    t = Table([[content]], colWidths=[500])
    t.setStyle(TableStyle([
        ('BACKGROUND', (0,0), (-1,-1), bg),
        ('BOX', (0,0), (-1,-1), 1, accent),
        ('LINELEFT', (0,0), (0,0), 3.5, accent),
        ('TOPPADDING', (0,0), (-1,-1), 5),
        ('BOTTOMPADDING', (0,0), (-1,-1), 5),
        ('LEFTPADDING', (0,0), (-1,-1), 8),
        ('RIGHTPADDING', (0,0), (-1,-1), 8),
    ]))
    return t
