# =====================================================================
#   Enlangg Sovereign Toolchain - Portable ANSI C99 Makefile
#   Builds the 7 pure-C toolchain binaries, standard library, and headers
# =====================================================================

PREFIX  ?= /usr/local/enlangg
BINDIR  ?= /usr/local/bin
DESTDIR ?=

CC      ?= cc
CFLAGS  ?= -O2
LDFLAGS ?= -lm -pthread

BUILD_DIR := build
BIN_OUT   := $(BUILD_DIR)/bin
LIB_OUT   := $(BUILD_DIR)/lib
INC_OUT   := $(BUILD_DIR)/include

TOOLS := \
	$(BIN_OUT)/enlangg \
	$(BIN_OUT)/enlng \
	$(BIN_OUT)/enlngdb \
	$(BIN_OUT)/enlngf \
	$(BIN_OUT)/enlngd \
	$(BIN_OUT)/enlngs \
	$(BIN_OUT)/enlngm

STDLIB_FILES := \
	lib_math.enlng \
	lib_strings.enlng \
	lib_ds.enlng \
	lib_file.enlng \
	lib_fs.enlng \
	lib_ml.enlng \
	lib_net.enlng \
	lib_db.enlng \
	lib_concurrency.enlng \
	lib_std.enlng \
	lib_enlngdb.enlng

.PHONY: all check-cc stage install uninstall clean

all: check-cc $(TOOLS) stage

check-cc:
	@command -v $(CC) >/dev/null 2>&1 || { \
		echo "Error: No C compiler ('$(CC)') found on PATH."; \
		echo "Please install a C compiler (e.g. 'sudo apt install build-essential', 'sudo dnf install gcc', or 'xcode-select --install') and retry."; \
		exit 1; \
	}

$(BIN_OUT):
	mkdir -p $(BIN_OUT)

$(BIN_OUT)/enlangg: enlangg.c enlngdb/c/enlngdb.c enlngdb/c/enlngdb_parser.c enlng/c/enlng_emitter.c src/enlangg_version.h | $(BIN_OUT)
	$(CC) $(CFLAGS) -I. -Isrc -Ienlngdb/c -Ienlng/c enlangg.c enlngdb/c/enlngdb.c enlngdb/c/enlngdb_parser.c enlng/c/enlng_emitter.c $(LDFLAGS) -o $@

$(BIN_OUT)/enlng: enlng.c src/enlng_main.c src/enlng_lexer.c src/enlng_parser.c src/enlng_codegen.c src/enlng_interp.c src/enlng_common.h src/enlng_runtime.h src/enlng_runtime_embed.h src/enlangg_version.h | $(BIN_OUT)
	$(CC) $(CFLAGS) -I. -Isrc enlng.c $(LDFLAGS) -o $@

$(BIN_OUT)/enlngdb: enlngdb/c/main.c enlngdb/c/enlngdb.c enlngdb/c/enlngdb_parser.c enlngdb/c/enlngdb.h src/enlangg_version.h | $(BIN_OUT)
	$(CC) $(CFLAGS) -I. -Isrc -Ienlngdb/c enlngdb/c/main.c enlngdb/c/enlngdb.c enlngdb/c/enlngdb_parser.c $(LDFLAGS) -o $@

$(BIN_OUT)/enlngf: enlngf.c enlngf/c/main.c enlngf/c/enlngf_engine.c enlngf/c/enlngf.h src/enlangg_version.h | $(BIN_OUT)
	$(CC) $(CFLAGS) -I. -Isrc -Ienlngf/c enlngf.c $(LDFLAGS) -o $@

$(BIN_OUT)/enlngd: enlngd.c enlngd/c/main.c enlngd/c/enlngd_engine.c enlngd/c/enlngd.h src/enlangg_version.h | $(BIN_OUT)
	$(CC) $(CFLAGS) -I. -Isrc -Ienlngd/c enlngd.c $(LDFLAGS) -o $@

$(BIN_OUT)/enlngs: enlngs.c enlngs/c/main.c enlngs/c/enlngs_engine.c enlngs/c/enlngs.h src/enlangg_version.h | $(BIN_OUT)
	$(CC) $(CFLAGS) -I. -Isrc -Ienlngs/c enlngs.c $(LDFLAGS) -o $@

$(BIN_OUT)/enlngm: enlngm.c enlngm/c/main.c enlngm/c/enlngm_engine.c enlngm/c/enlngm.h src/enlangg_version.h | $(BIN_OUT)
	$(CC) $(CFLAGS) -I. -Isrc -Ienlngm/c enlngm.c $(LDFLAGS) -o $@

stage: $(TOOLS)
	mkdir -p $(LIB_OUT) $(INC_OUT)
	cp $(STDLIB_FILES) $(LIB_OUT)/
	cp src/enlng_runtime.h $(INC_OUT)/
	cp enlng/c/enlng_mem.h $(INC_OUT)/
	cp LICENSE README.md $(BUILD_DIR)/

install: all
	@if ! command -v gcc >/dev/null 2>&1; then \
		echo "Note: 'gcc' was not found on PATH. 'enlng run' works without it (built-in interpreter); 'enlng build' and 'enlng run --native' need it."; \
		echo "         Install gcc (e.g. 'sudo apt install gcc', 'sudo dnf install gcc', or 'xcode-select --install') if you want native executables."; \
	fi
	mkdir -p "$(DESTDIR)$(PREFIX)/bin" "$(DESTDIR)$(PREFIX)/lib" "$(DESTDIR)$(PREFIX)/include" "$(DESTDIR)$(BINDIR)"
	install -m 0755 $(BIN_OUT)/enlangg "$(DESTDIR)$(PREFIX)/bin/enlangg"
	install -m 0755 $(BIN_OUT)/enlng   "$(DESTDIR)$(PREFIX)/bin/enlng"
	install -m 0755 $(BIN_OUT)/enlngdb "$(DESTDIR)$(PREFIX)/bin/enlngdb"
	install -m 0755 $(BIN_OUT)/enlngf  "$(DESTDIR)$(PREFIX)/bin/enlngf"
	install -m 0755 $(BIN_OUT)/enlngd  "$(DESTDIR)$(PREFIX)/bin/enlngd"
	install -m 0755 $(BIN_OUT)/enlngs  "$(DESTDIR)$(PREFIX)/bin/enlngs"
	install -m 0755 $(BIN_OUT)/enlngm  "$(DESTDIR)$(PREFIX)/bin/enlngm"
	install -m 0644 $(LIB_OUT)/*.enlng "$(DESTDIR)$(PREFIX)/lib/"
	install -m 0644 $(INC_OUT)/*.h     "$(DESTDIR)$(PREFIX)/include/"
	install -m 0644 LICENSE README.md  "$(DESTDIR)$(PREFIX)/"
	for tool in enlangg enlng enlngdb enlngf enlngd enlngs enlngm; do \
		ln -sf "$(PREFIX)/bin/$$tool" "$(DESTDIR)$(BINDIR)/$$tool"; \
	done

uninstall:
	for tool in enlangg enlng enlngdb enlngf enlngd enlngs enlngm; do \
		rm -f "$(DESTDIR)$(BINDIR)/$$tool"; \
	done
	rm -rf "$(DESTDIR)$(PREFIX)"

clean:
	rm -rf $(BUILD_DIR)
