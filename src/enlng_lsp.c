/*
 * =====================================================================
 *   ENLANG SOVEREIGN PROGRAMMING LANGUAGE (Pure C99 Native)
 *   src/enlng_lsp.c - Editor help: `enlng lsp`
 * =====================================================================
 *
 * Speaks the Language Server Protocol on standard input and output, so any
 * editor with LSP support can show mistakes while you type, explain a word
 * on hover, finish names, jump to where a name is made and list the
 * functions of a file. The mistakes come from the real parser, so they are
 * exactly the ones `enlng run` reports.
 */

#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif

/* ----------------------------------------------------------------- JSON */
typedef struct JV {
  int t; /* 0 null, 1 bool, 2 number, 3 text, 4 list, 5 object */
  double num;
  char *str;
  struct JV **items;
  char **keys;
  int n;
} JV;

static void js_ws(const char **p) {
  while (**p == ' ' || **p == '\t' || **p == '\n' || **p == '\r')
    (*p)++;
}

static int js_hex(const char *p) {
  int v = 0;
  for (int i = 0; i < 4; i++) {
    char c = p[i];
    v <<= 4;
    if (c >= '0' && c <= '9') v |= c - '0';
    else if (c >= 'a' && c <= 'f') v |= c - 'a' + 10;
    else if (c >= 'A' && c <= 'F') v |= c - 'A' + 10;
    else return -1;
  }
  return v;
}

static int js_utf8(char *out, unsigned cp) {
  if (cp < 0x80) { out[0] = (char)cp; return 1; }
  if (cp < 0x800) { out[0] = (char)(0xC0 | (cp >> 6)); out[1] = (char)(0x80 | (cp & 0x3F)); return 2; }
  if (cp < 0x10000) {
    out[0] = (char)(0xE0 | (cp >> 12)); out[1] = (char)(0x80 | ((cp >> 6) & 0x3F));
    out[2] = (char)(0x80 | (cp & 0x3F)); return 3;
  }
  out[0] = (char)(0xF0 | (cp >> 18)); out[1] = (char)(0x80 | ((cp >> 12) & 0x3F));
  out[2] = (char)(0x80 | ((cp >> 6) & 0x3F)); out[3] = (char)(0x80 | (cp & 0x3F)); return 4;
}

static char *js_string(const char **p) {
  (*p)++; /* the opening quote */
  size_t cap = 64, n = 0;
  char *out = (char *)malloc(cap);
  while (**p && **p != '"') {
    if (n + 8 >= cap) { cap *= 2; out = (char *)realloc(out, cap); }
    if (**p == '\\') {
      (*p)++;
      switch (**p) {
      case 'n': out[n++] = '\n'; break;
      case 't': out[n++] = '\t'; break;
      case 'r': out[n++] = '\r'; break;
      case 'b': out[n++] = '\b'; break;
      case 'f': out[n++] = '\f'; break;
      case 'u': {
        int cp = js_hex(*p + 1);
        if (cp < 0) { out[n++] = '?'; break; }
        *p += 4;
        if (cp >= 0xD800 && cp < 0xDC00 && (*p)[1] == '\\' && (*p)[2] == 'u') {
          int lo = js_hex(*p + 3);
          if (lo >= 0xDC00 && lo < 0xE000) {
            cp = 0x10000 + ((cp - 0xD800) << 10) + (lo - 0xDC00);
            *p += 6;
          }
        }
        n += (size_t)js_utf8(out + n, (unsigned)cp);
        break;
      }
      default: out[n++] = **p; break;
      }
      if (**p) (*p)++;
    } else {
      out[n++] = *(*p)++;
    }
  }
  if (**p == '"') (*p)++;
  out[n] = '\0';
  return out;
}

static JV *js_parse(const char **p, int depth) {
  JV *v = (JV *)calloc(1, sizeof(JV));
  js_ws(p);
  if (depth > 64) return v;
  if (**p == '"') {
    v->t = 3;
    v->str = js_string(p);
  } else if (**p == '{') {
    v->t = 5;
    (*p)++;
    js_ws(p);
    while (**p && **p != '}') {
      js_ws(p);
      if (**p != '"') break;
      char *key = js_string(p);
      js_ws(p);
      if (**p == ':') (*p)++;
      JV *val = js_parse(p, depth + 1);
      v->items = (JV **)realloc(v->items, sizeof(JV *) * (size_t)(v->n + 1));
      v->keys = (char **)realloc(v->keys, sizeof(char *) * (size_t)(v->n + 1));
      v->items[v->n] = val;
      v->keys[v->n++] = key;
      js_ws(p);
      if (**p == ',') (*p)++;
    }
    if (**p == '}') (*p)++;
  } else if (**p == '[') {
    v->t = 4;
    (*p)++;
    js_ws(p);
    while (**p && **p != ']') {
      JV *val = js_parse(p, depth + 1);
      v->items = (JV **)realloc(v->items, sizeof(JV *) * (size_t)(v->n + 1));
      v->items[v->n++] = val;
      js_ws(p);
      if (**p == ',') (*p)++;
      js_ws(p);
    }
    if (**p == ']') (*p)++;
  } else if (!strncmp(*p, "true", 4)) {
    v->t = 1; v->num = 1; *p += 4;
  } else if (!strncmp(*p, "false", 5)) {
    v->t = 1; *p += 5;
  } else if (!strncmp(*p, "null", 4)) {
    *p += 4;
  } else {
    char *end;
    v->t = 2;
    v->num = strtod(*p, &end);
    *p = end == *p ? *p + 1 : end;
  }
  return v;
}

static void js_free(JV *v) {
  if (!v) return;
  for (int i = 0; i < v->n; i++) {
    js_free(v->items[i]);
    if (v->keys) free(v->keys[i]);
  }
  free(v->items);
  free(v->keys);
  free(v->str);
  free(v);
}

static JV *js_get(JV *v, const char *key) {
  if (!v || v->t != 5) return NULL;
  for (int i = 0; i < v->n; i++)
    if (!strcmp(v->keys[i], key)) return v->items[i];
  return NULL;
}

static const char *js_str(JV *v, const char *key) {
  JV *x = js_get(v, key);
  return x && x->t == 3 ? x->str : NULL;
}

static int js_int(JV *v, const char *key, int dflt) {
  JV *x = js_get(v, key);
  return x && x->t == 2 ? (int)x->num : dflt;
}

/* A growing text for the reply. */
typedef struct {
  char *s;
  size_t n, cap;
} LspBuf;

static void lb_add(LspBuf *b, const char *t) {
  size_t l = strlen(t);
  if (b->n + l + 1 > b->cap) {
    b->cap = (b->n + l + 1) * 2;
    b->s = (char *)realloc(b->s, b->cap);
  }
  memcpy(b->s + b->n, t, l + 1);
  b->n += l;
}

static void lb_quote(LspBuf *b, const char *t) {
  lb_add(b, "\"");
  for (; *t; t++) {
    char tmp[8];
    unsigned char c = (unsigned char)*t;
    if (c == '"') lb_add(b, "\\\"");
    else if (c == '\\') lb_add(b, "\\\\");
    else if (c == '\n') lb_add(b, "\\n");
    else if (c == '\r') lb_add(b, "\\r");
    else if (c == '\t') lb_add(b, "\\t");
    else if (c < 0x20) { snprintf(tmp, sizeof(tmp), "\\u%04x", c); lb_add(b, tmp); }
    else { tmp[0] = (char)c; tmp[1] = '\0'; lb_add(b, tmp); }
  }
  lb_add(b, "\"");
}

static void lb_int(LspBuf *b, long v) {
  char tmp[32];
  snprintf(tmp, sizeof(tmp), "%ld", v);
  lb_add(b, tmp);
}

static void lsp_send(const char *body) {
  printf("Content-Length: %lu\r\n\r\n%s", (unsigned long)strlen(body), body);
  fflush(stdout);
}

/* --------------------------------------------------------------- documents */
typedef struct {
  char *uri;
  char *text;
} LspDoc;

static LspDoc g_docs[64];
static int g_ndocs;

static LspDoc *lsp_doc(const char *uri) {
  for (int i = 0; i < g_ndocs; i++)
    if (!strcmp(g_docs[i].uri, uri)) return &g_docs[i];
  return NULL;
}

static int lsp_count_lines(const char *t) {
  int n = 1;
  for (; *t; t++)
    if (*t == '\n') n++;
  return n;
}

static LspDoc *lsp_store(const char *uri, const char *text) {
  LspDoc *d = lsp_doc(uri);
  if (!d) {
    if (g_ndocs >= 64) { /* too many open: forget the one open longest */
      free(g_docs[0].uri);
      free(g_docs[0].text);
      memmove(&g_docs[0], &g_docs[1], sizeof(LspDoc) * 63);
      g_ndocs--;
    }
    d = &g_docs[g_ndocs++];
    d->uri = enlng_strdup(uri);
    d->text = NULL;
  }
  free(d->text);
  d->text = enlng_strdup(text);
  return d;
}

/* file:///c%3A/dir/a.enlng -> c:/dir/a.enlng */
static void lsp_uri_path(const char *uri, char *out, size_t sz) {
  const char *p = !strncmp(uri, "file://", 7) ? uri + 7 : uri;
  size_t n = 0;
  for (; *p && n + 1 < sz; p++) {
    if (*p == '%' && isxdigit((unsigned char)p[1]) && isxdigit((unsigned char)p[2])) {
      char h[3] = {p[1], p[2], 0};
      out[n++] = (char)strtol(h, NULL, 16);
      p += 2;
    } else {
      out[n++] = *p;
    }
  }
  out[n] = '\0';
  if (out[0] == '/' && isalpha((unsigned char)out[1]) && out[2] == ':')
    memmove(out, out + 1, strlen(out)); /* a Windows drive */
}

/* The start of line `line` (counted from 0), and its length without the newline. */
static const char *lsp_line(const char *t, int line, int *len) {
  const char *p = t;
  for (int l = 0; l < line && *p; p++)
    if (*p == '\n') l++;
  const char *e = p;
  while (*e && *e != '\n' && *e != '\r') e++;
  *len = (int)(e - p);
  return p;
}

/* An editor counts a column in UTF-16 units; the text is UTF-8. */
static int lsp_col_to_byte(const char *line, int len, int col) {
  int b = 0, units = 0;
  while (b < len && units < col) {
    unsigned char c = (unsigned char)line[b];
    int w = c < 0x80 ? 1 : c < 0xE0 ? 2 : c < 0xF0 ? 3 : 4;
    units += w == 4 ? 2 : 1;
    b += w;
  }
  return b < len ? b : len;
}

static int lsp_byte_to_col(const char *line, int bytes) {
  int b = 0, units = 0;
  while (b < bytes) {
    unsigned char c = (unsigned char)line[b];
    int w = c < 0x80 ? 1 : c < 0xE0 ? 2 : c < 0xF0 ? 3 : 4;
    units += w == 4 ? 2 : 1;
    b += w;
  }
  return units;
}

static int lsp_word_char(char c) { return isalnum((unsigned char)c) || c == '_'; }

/* The word at a position, copied into out; false when there is none. */
static bool lsp_word_at(const char *text, int line, int col, char *out, size_t sz) {
  int len;
  const char *l = lsp_line(text, line, &len);
  int b = lsp_col_to_byte(l, len, col);
  int s = b, e = b;
  while (s > 0 && lsp_word_char(l[s - 1])) s--;
  while (e < len && lsp_word_char(l[e])) e++;
  if (e <= s || (size_t)(e - s) >= sz) return false;
  memcpy(out, l + s, (size_t)(e - s));
  out[e - s] = '\0';
  return true;
}

/* ----------------------------------------------------------- definitions */
typedef struct {
  char name[96];
  int kind; /* LSP symbol kind: 12 function, 5 class, 13 variable */
  int line, col, len;
} LspDef;

/* The functions, classes, methods and names a file makes, found by reading
   its lines. */
static int lsp_defs(const char *text, LspDef *out, int max) {
  int n = 0, line = 0;
  const char *p = text;
  while (*p && n < max) {
    int len;
    const char *l = lsp_line(text, line, &len);
    (void)p;
    int i = 0;
    while (i < len && (l[i] == ' ' || l[i] == '\t')) i++;
    int indent = i;
    static const struct { const char *w; int kind; } heads[] = {
        {"function", 12}, {"method", 6}, {"blueprint", 5}, {"class", 5},
        {"remember", 13}, {"freeze", 14}};
    for (size_t h = 0; h < sizeof(heads) / sizeof(heads[0]); h++) {
      size_t wl = strlen(heads[h].w);
      if (i + (int)wl >= len || strncmp(l + i, heads[h].w, wl) != 0 || l[i + wl] != ' ') continue;
      int j = i + (int)wl;
      for (;;) { /* skip spaces and the silent words */
        while (j < len && l[j] == ' ') j++;
        if (!strncmp(l + j, "the ", 4)) j += 4;
        else if (!strncmp(l + j, "an ", 3)) j += 3;
        else if (!strncmp(l + j, "a ", 2) && heads[h].kind >= 13) j += 2;
        else break;
      }
      int s = j;
      while (j < len && lsp_word_char(l[j])) j++;
      if (j > s && j - s < 96 && (isalpha((unsigned char)l[s]) || l[s] == '_')) {
        memcpy(out[n].name, l + s, (size_t)(j - s));
        out[n].name[j - s] = '\0';
        out[n].kind = heads[h].kind;
        out[n].line = line;
        out[n].col = s;
        out[n].len = j - s;
        /* names made inside a function are not at the top of the file */
        if (!(heads[h].kind >= 13 && indent > 0)) n++;
      }
      break;
    }
    line++;
    if (!l[len] || (l[len] == '\r' && !l[len + 1])) break;
    p = l + len;
  }
  return n;
}

/* ---------------------------------------------------------- word help */
static const struct { const char *word; const char *doc; } LSP_DOCS[] = {
    {"remember", "`remember name as value`\nMakes a name for a value, or changes it."},
    {"freeze", "`freeze NAME as value`\nMakes a name that cannot be changed, and a list or map that cannot be changed."},
    {"show", "`show value, value`\nPrints the values with a space between them."},
    {"when", "`when condition:`\nRuns the block when the condition holds. `when the color is:` followed by one value per line chooses by a value. `otherwise` takes the rest."},
    {"otherwise", "`otherwise:` / `otherwise when condition:`\nThe block that runs when no earlier `when` held. In `a if c otherwise b` it picks one of two values."},
    {"for", "`for each item in list:` / `for i from 1 to 10:`\nRuns the block once for each item, or for each number."},
    {"repeat", "`repeat 3 times:` / `repeat while condition:` / `repeat until condition:`\nRuns the block again and again."},
    {"function", "`function name with a, b:`\nMakes a function. An input can have a default: `with name, greeting as \"Hello\"`. Call it with inputs by name: `area(width as 3, height as 4)`."},
    {"give", "`give value`\nEnds the function and gives the value back."},
    {"check", "`check that condition`\nStops with a message when the condition does not hold. `check that x is a number` also tells a call that gives a wrong kind before the program runs."},
    {"test", "`test \"name\":`\nA named group of checks. `enlng test` runs the files ending in _test.enlng."},
    {"try", "`try:` ... `on error as problem:`\nRuns the block, and runs the second block if something goes wrong."},
    {"fail", "`fail \"message\"`\nStops with a message that `try` can catch."},
    {"use", "`use \"file.enlng\"` / `use \"package\"`\nBrings in the functions of another file or of a package installed with `enlng add`."},
    {"stop", "`stop the program`\nEnds the program."},
    {"run", "`run together:`\nThe lines inside run at the same time and the program goes on when all are done."},
    {"ask", "`ask name \"question\"`\nReads a line the user types into the name."},
    {"swap", "`swap a with b`\nExchanges two values."},
    {"sort", "`sort list`\nPuts the list in order."},
    {"serve", "`serve handler on port 8080`\nA small web server: the function is called with each request."},
    {"blueprint", "`blueprint Name:`\nDescribes what a kind of object has and can do."},
    {"class", "`class Name:`\nMakes a kind of object."},
    {"method", "`method name with inputs:`\nA function that belongs to a class."},
    {"is", "`x is 5`, `x is a number`, `x is in list`\nCompares, or checks the kind of a value (number, whole number, decimal, text, list, map, function)."},
    {"contains", "`list contains value`\nTrue when the value is in the list or text."},
    {"every", "`every n in list where n is greater than 10`\nMakes a list from the items that match. `n times 2 for every n in list` makes a list from every item."},
};

/* A few built-in functions, to offer when finishing a word. */
static const char *LSP_BUILTINS[] = {
    "count", "sum", "abs", "round", "floor", "ceil", "sqrt", "pow", "min", "max", "unique",
    "to_number", "to_text", "append", "length", "upper", "lower", "split", "join", "replace",
    "read_file", "write_file", "delete_file", "sleep", "time_now", "random_number", "json_parse",
    "json_text", "run_together", "map", "filter", "range", "reduce", "keys", "values",
    "http_get", "http_post", "run_command", "check", "is_kind", "print", "type_of"};

static const char *LSP_KEYWORDS[] = {
    "remember", "freeze", "show", "display", "when", "otherwise", "for each", "for", "repeat",
    "function", "give", "check that", "test", "try", "on error as", "fail", "use", "stop the program",
    "run together:", "ask", "swap", "sort", "serve", "blueprint", "class", "method", "global",
    "is a number", "is a text", "is a list", "is a map", "is in", "contains", "every", "true",
    "false", "null", "and", "or", "not", "plus", "minus", "times", "divided by", "type enlng"};

static const char *lsp_doc_for(const char *word) {
  for (size_t i = 0; i < sizeof(LSP_DOCS) / sizeof(LSP_DOCS[0]); i++)
    if (!strcmp(LSP_DOCS[i].word, word)) return LSP_DOCS[i].doc;
  return NULL;
}

/* ---------------------------------------------------------------- reply */
static void lsp_reply(JV *id, const char *result_json) {
  LspBuf b = {0};
  lb_add(&b, "{\"jsonrpc\":\"2.0\",\"id\":");
  if (id && id->t == 3) lb_quote(&b, id->str);
  else if (id && id->t == 2) lb_int(&b, (long)id->num);
  else lb_add(&b, "null");
  lb_add(&b, ",\"result\":");
  lb_add(&b, result_json);
  lb_add(&b, "}");
  lsp_send(b.s);
  free(b.s);
}

static void lsp_error(JV *id, int code, const char *msg) {
  LspBuf b = {0};
  lb_add(&b, "{\"jsonrpc\":\"2.0\",\"id\":");
  if (id && id->t == 3) lb_quote(&b, id->str);
  else if (id && id->t == 2) lb_int(&b, (long)id->num);
  else lb_add(&b, "null");
  lb_add(&b, ",\"error\":{\"code\":");
  lb_int(&b, code);
  lb_add(&b, ",\"message\":");
  lb_quote(&b, msg);
  lb_add(&b, "}}");
  lsp_send(b.s);
  free(b.s);
}

/* Reads the file as the parser does and publishes what it finds wrong. */
static void lsp_diagnose(LspDoc *d) {
  LspBuf b = {0};
  lb_add(&b, "{\"jsonrpc\":\"2.0\",\"method\":\"textDocument/publishDiagnostics\",\"params\":{\"uri\":");
  lb_quote(&b, d->uri);
  lb_add(&b, ",\"diagnostics\":[");
  Lexer lx;
  lexer_init(&lx, d->text);
  lexer_tokenize(&lx);
  Parser ps;
  parser_init(&ps, lx.tokens, lx.token_count);
  char path[1024];
  lsp_uri_path(d->uri, path, sizeof(path));
  parser_set_source_file(path);
  g_lift_counter = 0;
  g_hidden_n = 0;
  ASTNode *prog = parser_parse_program(&ps);
  if (ps.has_error || !prog) {
    int line = ps.error_line > 0 ? ps.error_line - 1 : 0, len;
    const char *l = lsp_line(d->text, line, &len);
    int s = 0;
    while (s < len && (l[s] == ' ' || l[s] == '\t')) s++;
    lb_add(&b, "{\"range\":{\"start\":{\"line\":");
    lb_int(&b, line);
    lb_add(&b, ",\"character\":");
    lb_int(&b, lsp_byte_to_col(l, s));
    lb_add(&b, "},\"end\":{\"line\":");
    lb_int(&b, line);
    lb_add(&b, ",\"character\":");
    lb_int(&b, lsp_byte_to_col(l, len));
    lb_add(&b, "}},\"severity\":1,\"source\":\"enlng\",\"message\":");
    lb_quote(&b, ps.error_msg);
    lb_add(&b, "}");
  }
  lb_add(&b, "]}}");
  lsp_send(b.s);
  free(b.s);
}

static void lsp_hover(JV *id, JV *params) {
  JV *td = js_get(params, "textDocument"), *pos = js_get(params, "position");
  LspDoc *d = lsp_doc(js_str(td, "uri") ? js_str(td, "uri") : "");
  char word[96];
  if (!d || !lsp_word_at(d->text, js_int(pos, "line", 0), js_int(pos, "character", 0), word, sizeof(word))) {
    lsp_reply(id, "null");
    return;
  }
  LspDef defs[512];
  int nd = lsp_defs(d->text, defs, 512);
  LspBuf b = {0};
  for (int i = 0; i < nd; i++)
    if (!strcmp(defs[i].name, word) && defs[i].kind != 13) {
      int len;
      const char *l = lsp_line(d->text, defs[i].line, &len);
      while (len > 0 && (*l == ' ' || *l == '\t')) { l++; len--; }
      char head[300];
      snprintf(head, sizeof(head), "%.*s", len > 280 ? 280 : len, l);
      char md[600];
      snprintf(md, sizeof(md), "```enlng\n%s\n```\nMade on line %d.", head, defs[i].line + 1);
      lb_add(&b, "{\"contents\":{\"kind\":\"markdown\",\"value\":");
      lb_quote(&b, md);
      lb_add(&b, "}}");
      lsp_reply(id, b.s);
      free(b.s);
      return;
    }
  const char *doc = lsp_doc_for(word);
  if (doc) {
    lb_add(&b, "{\"contents\":{\"kind\":\"markdown\",\"value\":");
    lb_quote(&b, doc);
    lb_add(&b, "}}");
    lsp_reply(id, b.s);
  } else {
    lsp_reply(id, "null");
  }
  free(b.s);
}

static void lsp_item(LspBuf *b, int *first, const char *label, int kind, const char *detail) {
  if (!*first) lb_add(b, ",");
  *first = 0;
  lb_add(b, "{\"label\":");
  lb_quote(b, label);
  lb_add(b, ",\"kind\":");
  lb_int(b, kind);
  if (detail) {
    lb_add(b, ",\"detail\":");
    lb_quote(b, detail);
  }
  lb_add(b, "}");
}

static void lsp_complete(JV *id, JV *params) {
  JV *td = js_get(params, "textDocument");
  LspDoc *d = lsp_doc(js_str(td, "uri") ? js_str(td, "uri") : "");
  LspBuf b = {0};
  int first = 1;
  lb_add(&b, "[");
  if (d) {
    LspDef defs[512];
    int nd = lsp_defs(d->text, defs, 512);
    for (int i = 0; i < nd; i++)
      lsp_item(&b, &first, defs[i].name, defs[i].kind == 12 ? 3 : defs[i].kind == 6 ? 2 : defs[i].kind == 5 ? 7 : 6,
               defs[i].kind == 12 ? "function" : defs[i].kind == 5 ? "class" : "name");
  }
  for (size_t i = 0; i < sizeof(LSP_BUILTINS) / sizeof(LSP_BUILTINS[0]); i++)
    lsp_item(&b, &first, LSP_BUILTINS[i], 3, "built-in");
  for (size_t i = 0; i < sizeof(LSP_KEYWORDS) / sizeof(LSP_KEYWORDS[0]); i++)
    lsp_item(&b, &first, LSP_KEYWORDS[i], 14, NULL);
  lb_add(&b, "]");
  lsp_reply(id, b.s);
  free(b.s);
}

static void lsp_range(LspBuf *b, const char *text, const LspDef *d) {
  int len;
  const char *l = lsp_line(text, d->line, &len);
  lb_add(b, "{\"start\":{\"line\":");
  lb_int(b, d->line);
  lb_add(b, ",\"character\":");
  lb_int(b, lsp_byte_to_col(l, d->col));
  lb_add(b, "},\"end\":{\"line\":");
  lb_int(b, d->line);
  lb_add(b, ",\"character\":");
  lb_int(b, lsp_byte_to_col(l, d->col + d->len));
  lb_add(b, "}}");
}

static void lsp_definition(JV *id, JV *params) {
  JV *td = js_get(params, "textDocument"), *pos = js_get(params, "position");
  LspDoc *d = lsp_doc(js_str(td, "uri") ? js_str(td, "uri") : "");
  char word[96];
  if (!d || !lsp_word_at(d->text, js_int(pos, "line", 0), js_int(pos, "character", 0), word, sizeof(word))) {
    lsp_reply(id, "null");
    return;
  }
  LspDef defs[512];
  int nd = lsp_defs(d->text, defs, 512);
  for (int i = 0; i < nd; i++)
    if (!strcmp(defs[i].name, word)) {
      LspBuf b = {0};
      lb_add(&b, "{\"uri\":");
      lb_quote(&b, d->uri);
      lb_add(&b, ",\"range\":");
      lsp_range(&b, d->text, &defs[i]);
      lb_add(&b, "}");
      lsp_reply(id, b.s);
      free(b.s);
      return;
    }
  lsp_reply(id, "null");
}

static void lsp_symbols(JV *id, JV *params) {
  JV *td = js_get(params, "textDocument");
  LspDoc *d = lsp_doc(js_str(td, "uri") ? js_str(td, "uri") : "");
  LspBuf b = {0};
  lb_add(&b, "[");
  if (d) {
    LspDef defs[512];
    int nd = lsp_defs(d->text, defs, 512), first = 1;
    for (int i = 0; i < nd; i++) {
      if (!first) lb_add(&b, ",");
      first = 0;
      lb_add(&b, "{\"name\":");
      lb_quote(&b, defs[i].name);
      lb_add(&b, ",\"kind\":");
      lb_int(&b, defs[i].kind);
      lb_add(&b, ",\"range\":");
      lsp_range(&b, d->text, &defs[i]);
      lb_add(&b, ",\"selectionRange\":");
      lsp_range(&b, d->text, &defs[i]);
      lb_add(&b, "}");
    }
  }
  lb_add(&b, "]");
  lsp_reply(id, b.s);
  free(b.s);
}

static void lsp_handle(JV *msg) {
  const char *method = js_str(msg, "method");
  JV *id = js_get(msg, "id");
  JV *params = js_get(msg, "params");
  if (!method) return;
  if (!strcmp(method, "initialize")) {
    char reply[600];
    snprintf(reply, sizeof(reply),
             "{\"capabilities\":{\"textDocumentSync\":1,\"hoverProvider\":true,"
             "\"completionProvider\":{},\"definitionProvider\":true,\"documentSymbolProvider\":true},"
             "\"serverInfo\":{\"name\":\"enlng\",\"version\":\"%s\"}}",
             ENLNG_VERSION);
    lsp_reply(id, reply);
  } else if (!strcmp(method, "shutdown")) {
    lsp_reply(id, "null");
  } else if (!strcmp(method, "exit")) {
    exit(0);
  } else if (!strcmp(method, "textDocument/didOpen")) {
    JV *td = js_get(params, "textDocument");
    const char *uri = js_str(td, "uri"), *text = js_str(td, "text");
    if (uri && text) {
      LspDoc *d = lsp_store(uri, text);
      if (d) lsp_diagnose(d);
    }
  } else if (!strcmp(method, "textDocument/didChange")) {
    JV *td = js_get(params, "textDocument"), *chg = js_get(params, "contentChanges");
    const char *uri = js_str(td, "uri");
    if (uri && chg && chg->t == 4 && chg->n > 0) {
      const char *text = js_str(chg->items[chg->n - 1], "text");
      LspDoc *old = lsp_doc(uri);
      int lines_before = old && old->text ? lsp_count_lines(old->text) : -1;
      if (text) {
        LspDoc *d = lsp_store(uri, text);
        /* every key press would read the whole file again, so a change inside
           a line waits for the next Enter or save */
        if (d && lsp_count_lines(text) != lines_before) lsp_diagnose(d);
      }
    }
  } else if (!strcmp(method, "textDocument/didSave")) {
    JV *td = js_get(params, "textDocument");
    const char *uri = js_str(td, "uri");
    LspDoc *d = uri ? lsp_doc(uri) : NULL;
    const char *text = js_str(params, "text");
    if (d && text) lsp_store(uri, text);
    if (d) lsp_diagnose(lsp_doc(uri));
  } else if (!strcmp(method, "textDocument/didClose")) {
    JV *td = js_get(params, "textDocument");
    const char *uri = js_str(td, "uri");
    if (uri) {
      LspBuf b = {0};
      lb_add(&b, "{\"jsonrpc\":\"2.0\",\"method\":\"textDocument/publishDiagnostics\",\"params\":{\"uri\":");
      lb_quote(&b, uri);
      lb_add(&b, ",\"diagnostics\":[]}}");
      lsp_send(b.s);
      free(b.s);
      for (int i = 0; i < g_ndocs; i++)
        if (!strcmp(g_docs[i].uri, uri)) {
          free(g_docs[i].uri);
          free(g_docs[i].text);
          g_docs[i] = g_docs[--g_ndocs];
          break;
        }
    }
  } else if (!strcmp(method, "textDocument/hover")) {
    lsp_hover(id, params);
  } else if (!strcmp(method, "textDocument/completion")) {
    lsp_complete(id, params);
  } else if (!strcmp(method, "textDocument/definition")) {
    lsp_definition(id, params);
  } else if (!strcmp(method, "textDocument/documentSymbol")) {
    lsp_symbols(id, params);
  } else if (id) {
    lsp_error(id, -32601, "This request is not supported.");
  }
}

static int lsp_is_length(const char *h) {
  static const char name[] = "content-length:";
  for (size_t i = 0; i < sizeof(name) - 1; i++)
    if (tolower((unsigned char)h[i]) != name[i]) return 0;
  return 1;
}

int enlng_lsp(void) {
#ifdef _WIN32
  _setmode(_fileno(stdin), _O_BINARY);
  _setmode(_fileno(stdout), _O_BINARY);
#endif
  char header[512];
  for (;;) {
    long length = -1;
    while (fgets(header, sizeof(header), stdin)) {
      if (header[0] == '\r' || header[0] == '\n') break;
      if (lsp_is_length(header)) length = atol(header + 15);
    }
    if (length < 0 || feof(stdin)) return 0;
    if (length > 64 * 1024 * 1024) return 1;
    char *body = (char *)malloc((size_t)length + 1);
    size_t got = fread(body, 1, (size_t)length, stdin);
    body[got] = '\0';
    const char *p = body;
    JV *msg = js_parse(&p, 0);
    lsp_handle(msg);
    js_free(msg);
    free(body);
  }
}
