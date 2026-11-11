#if defined(_WIN32) && !defined(__EMSCRIPTEN__)
#include <winsock2.h>     /* before windows.h (which enlngdb.h brings), or the C library of Visual C++ has two kinds of socket */
#include <ws2tcpip.h>
#endif
#include "enlngdb.h"
#include <ctype.h>
#include <time.h>

#ifndef __EMSCRIPTEN__
/* ---- the server and its client ---------------------------------------------------------------------------
 * enlngdb --serve NAME [NAME...] [--port N] [--host ADDRESS] [--key FILE] [--plain]
 *                                serves NAME.edb (and more databases) to users who sign in
 * enlngdb --connect [HOST:]PORT[/NAME] --user USER [--trust FINGERPRINT] [--plain] [FILE]
 *                                signs in and runs FILE, or each line typed
 *
 * The connection is encrypted. The server has a key of its own (X25519, kept in enlngdb_server.key) and shows
 * its fingerprint when it starts; the client remembers the key of each server it trusts (in
 * ~/.enlngdb_known_servers, like ssh) and refuses a server whose key changed. Each connection agrees on fresh
 * keys (X25519 with a new key on both sides, HKDF with SHA-256), and every message is sealed with
 * ChaCha20-Poly1305 (RFC 8439): it cannot be read, changed, replayed or reordered on the way. '--plain' turns
 * this off, and only on this computer (127.0.0.1), for tools and tests.
 *
 * Inside, the conversation is messages: a line "<word> <length>" and then that many bytes. The client sends
 * "login" (the database, the name and the password on three lines; the database may be left out when the server
 * serves one), "run" (a script) or "bye"; the server answers "ok", "failed" (some statements failed), "denied"
 * or "busy", with the text the statements printed. One request runs at a time; while a client has a
 * transaction open in a database, the requests of others for that database wait for it (up to 10 seconds).
 * After a change, a database is saved as soon as no transaction is open in it. */
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <conio.h>
#include <io.h>
typedef SOCKET qsock;
#define QBAD INVALID_SOCKET
/* the functions of ws2_32.dll, found when the program needs them (nothing extra to link) */
static int (WSAAPI* p_WSAStartup)(WORD, LPWSADATA);
static SOCKET (WSAAPI* p_socket)(int, int, int);
static int (WSAAPI* p_bind)(SOCKET, const struct sockaddr*, int);
static int (WSAAPI* p_listen)(SOCKET, int);
static SOCKET (WSAAPI* p_accept)(SOCKET, struct sockaddr*, int*);
static int (WSAAPI* p_recv)(SOCKET, char*, int, int);
static int (WSAAPI* p_send)(SOCKET, const char*, int, int);
static int (WSAAPI* p_closesocket)(SOCKET);
static int (WSAAPI* p_select)(int, fd_set*, fd_set*, fd_set*, const struct timeval*);
static int (WSAAPI* p_connect)(SOCKET, const struct sockaddr*, int);
static int (WSAAPI* p_setsockopt)(SOCKET, int, int, const char*, int);
static int (WSAAPI* p_getaddrinfo)(const char*, const char*, const struct addrinfo*, struct addrinfo**);
static void (WSAAPI* p_freeaddrinfo)(struct addrinfo*);
static int (WSAAPI* p_getsockname)(SOCKET, struct sockaddr*, int*);

static bool net_start(void) {
    static int done = 0;
    if (done) return done > 0;
    HMODULE w = LoadLibraryA("ws2_32.dll");
    done = -1;
    if (!w) return false;
#define FIND(name) if (!(*(void**)&p_##name = (void*)GetProcAddress(w, #name))) return false
    FIND(WSAStartup); FIND(socket); FIND(bind); FIND(listen); FIND(accept); FIND(recv); FIND(send);
    FIND(closesocket); FIND(select); FIND(connect); FIND(setsockopt); FIND(getaddrinfo); FIND(freeaddrinfo); FIND(getsockname);
#undef FIND
    WSADATA wd;
    if (p_WSAStartup(MAKEWORD(2, 2), &wd) != 0) return false;
    done = 1;
    return true;
}
static bool in_set(qsock s, fd_set* set) {
    for (u_int i = 0; i < set->fd_count; i++) if (set->fd_array[i] == s) return true;
    return false;
}
static void set_add(qsock s, fd_set* set) { if (set->fd_count < FD_SETSIZE) set->fd_array[set->fd_count++] = s; }
#define q_close(s) p_closesocket(s)
#define q_recv(s, b, n) p_recv((s), (b), (int)(n), 0)
#define q_send(s, b, n) p_send((s), (b), (int)(n), 0)
static volatile LONG stop_now = 0;
static BOOL WINAPI on_ctrl(DWORD type) { (void)type; InterlockedExchange(&stop_now, 1); return TRUE; }
static void watch_stop(void) { SetConsoleCtrlHandler(on_ctrl, TRUE); }
static bool stopping(void) { return stop_now != 0; }
static bool is_terminal(void) { return _isatty(_fileno(stdin)) != 0; }
static void read_secret(char* out, size_t n) {
    size_t k = 0;
    for (;;) {
        int c = _getch();
        if (c == '\r' || c == '\n' || c == EOF) break;
        if (c == '\b') { if (k) k--; continue; }
        if (k + 1 < n) out[k++] = (char)c;
    }
    out[k] = '\0';
    printf("\n");
}
static const char* home_dir(void) { const char* h = getenv("USERPROFILE"); return h ? h : "."; }
static void keep_private(const char* path) { (void)path; }
#else
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <signal.h>
#include <termios.h>
typedef int qsock;
#define QBAD (-1)
#define p_socket socket
#define p_bind bind
#define p_listen listen
#define p_accept accept
#define p_select select
#define p_connect connect
#define p_setsockopt setsockopt
#define p_getaddrinfo getaddrinfo
#define p_freeaddrinfo freeaddrinfo
#define p_getsockname getsockname
static bool net_start(void) { signal(SIGPIPE, SIG_IGN); return true; }
static bool in_set(qsock s, fd_set* set) { return FD_ISSET(s, set) != 0; }
static void set_add(qsock s, fd_set* set) { FD_SET(s, set); }
#define q_close(s) close(s)
#define q_recv(s, b, n) recv((s), (b), (n), 0)
#define q_send(s, b, n) send((s), (b), (n), 0)
static volatile sig_atomic_t stop_now = 0;
static void on_signal(int sig) { (void)sig; stop_now = 1; }
static void watch_stop(void) { signal(SIGINT, on_signal); signal(SIGTERM, on_signal); }
static bool stopping(void) { return stop_now != 0; }
static bool is_terminal(void) { return isatty(0) != 0; }
static void read_secret(char* out, size_t n) {
    struct termios old, quiet;
    bool tty = tcgetattr(0, &old) == 0;
    if (tty) { quiet = old; quiet.c_lflag &= ~(tcflag_t)ECHO; tcsetattr(0, TCSANOW, &quiet); }
    if (!fgets(out, (int)n, stdin)) out[0] = '\0';
    out[strcspn(out, "\r\n")] = '\0';
    if (tty) { tcsetattr(0, TCSANOW, &old); printf("\n"); }
}
static const char* home_dir(void) { const char* h = getenv("HOME"); return h ? h : "."; }
static void keep_private(const char* path) { chmod(path, 0600); }
#endif
#define ENLNGDB_WRAP_ALLOC
#include "enlngdb.h"

/* ---- X25519 (RFC 7748), after TweetNaCl (public domain) ---- */
typedef int64_t gf[16];

static void car25519(gf o) {
    for (int i = 0; i < 16; i++) {
        o[i] += (int64_t)1 << 16;
        int64_t c = o[i] >> 16;
        o[(i + 1) * (i < 15)] += c - 1 + 37 * (c - 1) * (i == 15);
        o[i] -= c * 65536;
    }
}
static void sel25519(gf p, gf q, int b) {
    int64_t c = ~(int64_t)(b - 1);
    for (int i = 0; i < 16; i++) { int64_t t = c & (p[i] ^ q[i]); p[i] ^= t; q[i] ^= t; }
}
static void pack25519(uint8_t* o, const gf n) {
    gf m, t;
    for (int i = 0; i < 16; i++) t[i] = n[i];
    car25519(t); car25519(t); car25519(t);
    for (int j = 0; j < 2; j++) {
        m[0] = t[0] - 0xffed;
        for (int i = 1; i < 15; i++) { m[i] = t[i] - 0xffff - ((m[i - 1] >> 16) & 1); m[i - 1] &= 0xffff; }
        m[15] = t[15] - 0x7fff - ((m[14] >> 16) & 1);
        int b = (int)((m[15] >> 16) & 1);
        m[14] &= 0xffff;
        sel25519(t, m, 1 - b);
    }
    for (int i = 0; i < 16; i++) { o[2 * i] = (uint8_t)(t[i] & 0xff); o[2 * i + 1] = (uint8_t)(t[i] >> 8); }
}
static void unpack25519(gf o, const uint8_t* n) {
    for (int i = 0; i < 16; i++) o[i] = n[2 * i] + ((int64_t)n[2 * i + 1] << 8);
    o[15] &= 0x7fff;
}
static void fadd(gf o, const gf a, const gf b) { for (int i = 0; i < 16; i++) o[i] = a[i] + b[i]; }
static void fsub(gf o, const gf a, const gf b) { for (int i = 0; i < 16; i++) o[i] = a[i] - b[i]; }
static void fmul(gf o, const gf a, const gf b) {
    int64_t t[31];
    for (int i = 0; i < 31; i++) t[i] = 0;
    for (int i = 0; i < 16; i++) for (int j = 0; j < 16; j++) t[i + j] += a[i] * b[j];
    for (int i = 0; i < 15; i++) t[i] += 38 * t[i + 16];
    for (int i = 0; i < 16; i++) o[i] = t[i];
    car25519(o); car25519(o);
}
static void fsq(gf o, const gf a) { fmul(o, a, a); }
static void finv(gf o, const gf in) {
    gf c;
    for (int a = 0; a < 16; a++) c[a] = in[a];
    for (int a = 253; a >= 0; a--) { fsq(c, c); if (a != 2 && a != 4) fmul(c, c, in); }
    for (int a = 0; a < 16; a++) o[a] = c[a];
}
static void x25519(uint8_t q[32], const uint8_t n[32], const uint8_t p[32]) {
    static const gf k121665 = {0xDB41, 1};
    uint8_t z[32];
    int64_t x[80];
    gf a, b, c, d, e, f;
    for (int i = 0; i < 31; i++) z[i] = n[i];
    z[31] = (uint8_t)((n[31] & 127) | 64);
    z[0] &= 248;
    unpack25519(x, p);
    for (int i = 0; i < 16; i++) { b[i] = x[i]; d[i] = a[i] = c[i] = 0; }
    a[0] = d[0] = 1;
    for (int i = 254; i >= 0; --i) {
        int r = (z[i >> 3] >> (i & 7)) & 1;
        sel25519(a, b, r); sel25519(c, d, r);
        fadd(e, a, c); fsub(a, a, c); fadd(c, b, d); fsub(b, b, d);
        fsq(d, e); fsq(f, a); fmul(a, c, a); fmul(c, b, e);
        fadd(e, a, c); fsub(a, a, c); fsq(b, a); fsub(c, d, f);
        fmul(a, c, k121665); fadd(a, a, d); fmul(c, c, a); fmul(a, d, f);
        fmul(d, b, x); fsq(b, e);
        sel25519(a, b, r); sel25519(c, d, r);
    }
    for (int i = 0; i < 16; i++) { x[i + 16] = a[i]; x[i + 32] = c[i]; x[i + 48] = b[i]; x[i + 64] = d[i]; }
    finv(x + 32, x + 32);
    fmul(x + 16, x + 16, x + 32);
    pack25519(q, x + 16);
}
static void x25519_public(uint8_t pub[32], const uint8_t priv[32]) {
    static const uint8_t nine[32] = {9};
    x25519(pub, priv, nine);
}

/* ---- ChaCha20 and Poly1305 (RFC 8439) ---- */
static uint32_t le32(const uint8_t* p) { return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24); }
static void put32(uint8_t* p, uint32_t v) { p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8); p[2] = (uint8_t)(v >> 16); p[3] = (uint8_t)(v >> 24); }
/* ChaCha20-Poly1305 is in the engine (enlngdb.c), which also seals encrypted database files */
#define aead_seal enlngdb_aead_seal
#define aead_open enlngdb_aead_open

static void make_nonce(uint8_t nonce[12], uint64_t counter) {
    memset(nonce, 0, 12);
    for (int i = 0; i < 8; i++) nonce[4 + i] = (uint8_t)(counter >> (8 * i));
}

static bool all_zero(const uint8_t* p, size_t n) { uint8_t a = 0; for (size_t i = 0; i < n; i++) a |= p[i]; return a == 0; }

static void to_hex(const uint8_t* p, size_t n, char* out) {
    static const char* h = "0123456789abcdef";
    for (size_t i = 0; i < n; i++) { out[2 * i] = h[p[i] >> 4]; out[2 * i + 1] = h[p[i] & 15]; }
    out[2 * n] = '\0';
}

static bool from_hex(const char* s, uint8_t* out, size_t n) {
    for (size_t i = 0; i < n; i++) {
        int v = 0;
        for (int k = 0; k < 2; k++) {
            char c = s[2 * i + k];
            int d = c >= '0' && c <= '9' ? c - '0' : c >= 'a' && c <= 'f' ? c - 'a' + 10 : c >= 'A' && c <= 'F' ? c - 'A' + 10 : -1;
            if (d < 0) return false;
            v = v * 16 + d;
        }
        out[i] = (uint8_t)v;
    }
    return true;
}

/* what people compare: the first 16 bytes of the SHA-256 of the public key, in groups of four */
static void fingerprint(const uint8_t pub[32], char* out) {
    uint8_t h[32];
    enlngdb_sha256(pub, 32, h);
    char hex[33];
    to_hex(h, 16, hex);
    size_t k = 0;
    for (int i = 0; i < 32; i++) { if (i && i % 4 == 0) out[k++] = '-'; out[k++] = hex[i]; }
    out[k] = '\0';
}

/* the keys of a connection: HKDF (RFC 5869) with SHA-256 over both shared secrets, bound to what was sent */
static void derive_keys(const uint8_t dh1[32], const uint8_t dh2[32], const uint8_t e[32], const uint8_t s[32], const uint8_t f[32],
                        uint8_t c2s[32], uint8_t s2c[32]) {
    static const char salt[] = "enlngdb handshake 1";
    uint8_t ikm[64], prk[32], th[32], info[33];
    memcpy(ikm, dh1, 32);
    memcpy(ikm + 32, dh2, 32);
    enlngdb_hmac_sha256((const uint8_t*)salt, sizeof(salt) - 1, ikm, 64, prk);
    uint8_t seen[4 + 96];
    memcpy(seen, "EDB1", 4);
    memcpy(seen + 4, e, 32);
    memcpy(seen + 36, s, 32);
    memcpy(seen + 68, f, 32);
    enlngdb_sha256(seen, sizeof(seen), th);
    memcpy(info, th, 32);
    info[32] = 1;
    enlngdb_hmac_sha256(prk, 32, info, 33, c2s);
    uint8_t info2[65];
    memcpy(info2, c2s, 32);
    memcpy(info2 + 32, th, 32);
    info2[64] = 2;
    enlngdb_hmac_sha256(prk, 32, info2, 65, s2c);
    memset(ikm, 0, sizeof(ikm));
    memset(prk, 0, sizeof(prk));
}

/* the crypto, checked against fixed answers (RFC 7748, RFC 8439, and Python's hashlib and cryptography) */
static int self_test(void) {
    int bad = 0;
    uint8_t a[32], b[32], A[32], B[32], s1[32], s2[32];
    from_hex("77076d0a7318a57d3c16c17251b26645df4c2f87ebc0992ab177fba51db92c2a", a, 32);
    from_hex("5dab087e624a8a4b79e17f8b83800ee66f3bb1292618b6fd1c2f8b27ff88e0eb", b, 32);
    x25519_public(A, a);
    x25519_public(B, b);
    x25519(s1, a, B);
    x25519(s2, b, A);
    char hex[300];
    to_hex(A, 32, hex); if (strcmp(hex, "8520f0098930a754748b7ddcb43ef75a0dbf3a0d26381af4eba4a98eaa9b4e6a")) { printf("FAIL x25519 public key\n"); bad++; }
    to_hex(B, 32, hex); if (strcmp(hex, "de9edb7d7b7dc1b4d35b61c2ece435373f8343c85b78674dadfc7e146f882b4f")) { printf("FAIL x25519 public key 2\n"); bad++; }
    to_hex(s1, 32, hex); if (strcmp(hex, "4a5d9d5ba4ce2de1728e3bf480350f25e07e21c947d19e3376f09b3c1e161742") || memcmp(s1, s2, 32)) { printf("FAIL x25519 shared secret\n"); bad++; }
    uint8_t key[32], nonce[12] = {0, 0, 0, 0, 7, 0, 0, 0, 0x40, 0x41, 0x42, 0x43};
    for (int i = 0; i < 32; i++) key[i] = (uint8_t)i;
    const char* pt = "Ladies and Gentlemen of the class of '99: If I could offer you only one tip for the future, sunscreen would be it.";
    size_t n = strlen(pt);
    uint8_t buf[200];
    memcpy(buf, pt, n);
    aead_seal(key, nonce, buf, n);
    char big[400];
    to_hex(buf, n + 16, big);
    if (strcmp(big, "0d0aea7f6968cce828f123739e8d9f6c460204d6db9a7aa3ddb2d2c5330243cb639bb09188dbcd5103ea21908bb9e0c9ec91e587a712e675d2de7989b7f78a26b377b5e2caa3020fb94e88bfca486ee458c61d62b36e7c670c0082405dbf9b6ae7ec298f09109924a8ccd566149361b0c044a7f98c3a97a38bc4f9dcd0f52f84af19")) { printf("FAIL chacha20-poly1305 seal\n"); bad++; }
    if (!aead_open(key, nonce, buf, n) || memcmp(buf, pt, n)) { printf("FAIL chacha20-poly1305 open\n"); bad++; }
    aead_seal(key, nonce, buf, n);
    buf[5] ^= 1;
    if (aead_open(key, nonce, buf, n)) { printf("FAIL a changed message was not noticed\n"); bad++; }
    uint8_t h[32];
    enlngdb_sha256("abc", 3, h);
    to_hex(h, 32, hex); if (strcmp(hex, "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad")) { printf("FAIL sha-256\n"); bad++; }
    const char* msg = "The quick brown fox jumps over the lazy dog";
    enlngdb_hmac_sha256((const uint8_t*)"key", 3, (const uint8_t*)msg, strlen(msg), h);
    to_hex(h, 32, hex); if (strcmp(hex, "f7bc83f430538424b13298e6aa6fb143ef4d59a14946175997479dbc2d1a3cd8")) { printf("FAIL hmac-sha-256\n"); bad++; }
    enlngdb_pbkdf2_sha256("passwd", (const uint8_t*)"salt", 4, 1, h);
    to_hex(h, 32, hex); if (strcmp(hex, "55ac046e56e3089fec1691c22544b605f94185216dde0465e68b9d57c20dacbc")) { printf("FAIL pbkdf2-sha-256\n"); bad++; }
    if (bad) printf("%d check(s) of the crypto failed.\n", bad);
    else printf("The crypto gives the expected answers (X25519, ChaCha20-Poly1305, SHA-256, HMAC, PBKDF2).\n");
    return bad ? 1 : 0;
}

/* --x25519 SCALAR POINT and --seal KEY NONCE DATA (hex): for checking the crypto against other programs */
static int crypto_tool(int argc, char** argv) {
    uint8_t k[32], p[32], out[32];
    static char hex[2 * 70100 + 1];
    if (strcmp(argv[1], "--x25519") == 0 && argc == 4 && strlen(argv[2]) == 64 && strlen(argv[3]) == 64 && from_hex(argv[2], k, 32) && from_hex(argv[3], p, 32)) {
        x25519(out, k, p);
        to_hex(out, 32, hex);
        printf("%s\n", hex);
        return 0;
    }
    if (strcmp(argv[1], "--seal") == 0 && argc == 5 && strlen(argv[2]) == 64 && strlen(argv[3]) == 24 && from_hex(argv[2], k, 32)) {
        uint8_t nonce[12];
        size_t n = strlen(argv[4]) / 2;
        static uint8_t data[70100];
        if (!from_hex(argv[3], nonce, 12) || n > 70000 || !from_hex(argv[4], data, n)) return 1;
        aead_seal(k, nonce, data, n);
        to_hex(data, n + 16, hex);
        printf("%s\n", hex);
        return 0;
    }
    fprintf(stderr, "usage: --x25519 SCALAR POINT | --seal KEY NONCE DATA (all in hex)\n");
    return 1;
}

#define MAX_CLIENTS 64
#define MAX_DATABASES 16
#define MAX_MESSAGE (64u * 1024u * 1024u)
#define WAIT_FOR_TXN_SECONDS 10

static bool send_all(qsock s, const char* p, size_t n) {
    while (n > 0) {
        int k = q_send(s, p, n > 65536 ? 65536 : n);
        if (k <= 0) return false;
        p += k;
        n -= (size_t)k;
    }
    return true;
}

/* one end of a connection: sealed or plain, and the keys and counters when sealed */
typedef struct {
    qsock sock;
    bool sealed;
    uint8_t k_out[32], k_in[32];
    uint64_t n_out, n_in;
} Link;

static bool send_msg(Link* L, const char* word, const char* body, size_t n) {
    char head[64];
    int h = snprintf(head, sizeof(head), "%s %lu\n", word, (unsigned long)n);
    if (!L->sealed) return send_all(L->sock, head, (size_t)h) && send_all(L->sock, body, n);
    size_t len = (size_t)h + n;
    uint8_t* frame = (uint8_t*)malloc(4 + len + 16);
    if (!frame) return false;
    put32(frame, (uint32_t)(len + 16));
    memcpy(frame + 4, head, (size_t)h);
    if (n) memcpy(frame + 4 + h, body, n);
    uint8_t nonce[12];
    make_nonce(nonce, L->n_out++);
    aead_seal(L->k_out, nonce, frame + 4, len);
    bool ok = send_all(L->sock, (const char*)frame, 4 + len + 16);
    free(frame);
    return ok;
}

/* a whole message at the start of buf: its word, its body and the bytes it took; 0 if it is not all there yet, -1 if bad */
static long take_msg(const char* buf, size_t len, char* word, size_t wn, const char** body, size_t* bn) {
    const char* nl = memchr(buf, '\n', len < 100 ? len : 100);
    if (!nl) return len >= 100 ? -1 : 0;
    char head[100];
    size_t hl = (size_t)(nl - buf);
    memcpy(head, buf, hl);
    head[hl] = '\0';
    unsigned long n = 0;
    char w[32];
    if (sscanf(head, "%31s %lu", w, &n) != 2 || n > MAX_MESSAGE) return -1;
    if (len < hl + 1 + n) return 0;
    snprintf(word, wn, "%s", w);
    *body = nl + 1;
    *bn = n;
    return (long)(hl + 1 + n);
}

/* growing buffers */
typedef struct { char* p; size_t len, cap; } Buf;
static bool buf_add(Buf* b, const void* data, size_t n) {
    if (b->len + n + 1 > b->cap) {
        size_t cap = b->cap ? b->cap : 65536;
        while (cap < b->len + n + 1) cap *= 2;
        char* g = (char*)realloc(b->p, cap);
        if (!g) return false;
        b->p = g;
        b->cap = cap;
    }
    memcpy(b->p + b->len, data, n);
    b->len += n;
    return true;
}
static void buf_drop(Buf* b, size_t n) { memmove(b->p, b->p + n, b->len - n); b->len -= n; }

/* sealed frames in raw become plain text in out; false if one was changed or is too big */
static bool open_frames(Link* L, Buf* raw, Buf* out) {
    while (raw->len >= 4) {
        uint32_t fl = le32((const uint8_t*)raw->p);
        if (fl < 16 || fl > MAX_MESSAGE + 256) return false;
        if (raw->len < 4 + (size_t)fl) break;
        uint8_t nonce[12];
        make_nonce(nonce, L->n_in++);
        if (!aead_open(L->k_in, nonce, (uint8_t*)raw->p + 4, fl - 16)) return false;
        if (!buf_add(out, raw->p + 4, fl - 16)) return false;
        buf_drop(raw, 4 + fl);
    }
    return true;
}

static qsock open_socket(const char* host, const char* port, bool listening, char* err, size_t en) {
    if (!net_start()) { snprintf(err, en, "the network could not be used on this computer"); return QBAD; }
    struct addrinfo hints, *res = NULL;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    if (listening) hints.ai_flags = AI_PASSIVE;
    if (p_getaddrinfo(host, port, &hints, &res) != 0 || !res) { snprintf(err, en, "the address %s:%s could not be found", host, port); return QBAD; }
    qsock s = p_socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (s == QBAD) { p_freeaddrinfo(res); snprintf(err, en, "no socket could be made"); return QBAD; }
    if (listening) {
#ifndef _WIN32
        int yes = 1;
        p_setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (const char*)&yes, sizeof(yes));
#endif
        if (p_bind(s, res->ai_addr, (int)res->ai_addrlen) != 0 || p_listen(s, 16) != 0) {
            snprintf(err, en, "%s:%s is in use or cannot be used (another server on this port?)", host, port);
            q_close(s);
            s = QBAD;
        }
    } else if (p_connect(s, res->ai_addr, (int)res->ai_addrlen) != 0) {
        snprintf(err, en, "no server answered at %s:%s", host, port);
        q_close(s);
        s = QBAD;
    }
    p_freeaddrinfo(res);
    return s;
}

/* the server's own key: read from the key file, or made there the first time */
static bool server_key(const char* path, uint8_t priv[32], uint8_t pub[32], bool* made) {
    *made = false;
    FILE* f = fopen(path, "rb");
    if (f) {
        bool ok = fread(priv, 1, 32, f) == 32;
        fclose(f);
        if (!ok) return false;
    } else {
        if (!enlngdb_random_bytes(priv, 32)) return false;
        f = fopen(path, "wb");
        if (!f) return false;
        bool ok = fwrite(priv, 1, 32, f) == 32;
        if (fclose(f) != 0) ok = false;
        if (!ok) { remove(path); return false; }
        keep_private(path);
        *made = true;
    }
    x25519_public(pub, priv);
    return true;
}

typedef struct {
    char name[ENLNGDB_MAX_NAME];
    EnlngDatabase* db;
    char file[300];
    int txn_owner;              /* the client whose transaction is open in it */
    bool dirty;
    bool stale;                 /* the last save failed: the file is behind, so readers do not answer for it */
    unsigned long gen;          /* counts the saves: a reader opens the file again when it changed */
} Served;

typedef struct {
    Link link;
    int id;
    bool hello_done;            /* sealed: the keys are agreed */
    Buf raw, in;                /* bytes as they came; the plain messages */
    bool signed_in;
    bool admin;
    char user[ENLNGDB_MAX_NAME];
    int dbi;                    /* the database it signed in to */
    int fails;
    time_t waiting_since;       /* a request waits for another client's transaction (0: none) */
    int at_reader;              /* the reader running its request (-1: none); its next requests wait for the answer */
    char* held;                 /* the answer to a request that changed the database: sent when the change is on the disk */
    bool held_failed;
} Client;

static void drop_client(Client* c) {
    q_close(c->link.sock);
    free(c->raw.p);
    free(c->in.p);
    free(c->held);
    memset(c, 0, sizeof(*c));
    c->link.sock = QBAD;
    c->at_reader = -1;
}

static bool reply(Client* c, const char* word, const char* text) { return send_msg(&c->link, word, text, strlen(text)); }

/* the first message of a sealed connection: the client's fresh public key; the server answers with its own key,
   a fresh one, and a sealed "ready" that only the holder of the server's key can make */
static bool server_hello(Client* c, const uint8_t priv[32], const uint8_t pub[32]) {
    if (c->raw.len < 36) return true;
    if (memcmp(c->raw.p, "EDB1", 4) != 0) return false;
    uint8_t E[32], f[32], F[32], dh1[32], dh2[32];
    memcpy(E, c->raw.p + 4, 32);
    buf_drop(&c->raw, 36);
    if (!enlngdb_random_bytes(f, 32)) return false;
    x25519_public(F, f);
    x25519(dh1, priv, E);
    x25519(dh2, f, E);
    memset(f, 0, sizeof(f));
    if (all_zero(dh1, 32) || all_zero(dh2, 32)) return false;
    derive_keys(dh1, dh2, E, pub, F, c->link.k_in, c->link.k_out);
    memset(dh1, 0, 32);
    memset(dh2, 0, 32);
    uint8_t out[4 + 64];
    memcpy(out, "EDB1", 4);
    memcpy(out + 4, pub, 32);
    memcpy(out + 36, F, 32);
    if (!send_all(c->link.sock, (const char*)out, sizeof(out))) return false;
    c->link.sealed = true;
    c->hello_done = true;
    return send_msg(&c->link, "ready", "", 0);
}

static int find_served(Served* sv, int nsv, const char* name) {
    for (int i = 0; i < nsv; i++) {
        const char* x = sv[i].name;
        size_t k = 0;
        while (x[k] && name[k] && tolower((unsigned char)x[k]) == tolower((unsigned char)name[k])) k++;
        if (!x[k] && !name[k]) return i;
    }
    return -1;
}

static void end_txn_of(Served* sv, Client* c) {
    if (c->signed_in && sv[c->dbi].txn_owner == c->id && sv[c->dbi].db->txn) { enlngdb_rollback(sv[c->dbi].db); sv[c->dbi].txn_owner = 0; }
}

/* ---- readers: requests that only read, run in other processes at the same time ----
 * The server starts a few reader processes ('enlngdb --reader PORT TOKEN', the same program) that connect back to
 * it on 127.0.0.1. A request whose statements only read (find, select, show, count ...) goes to a reader that is
 * free, when the database has no changes waiting to be saved and no transaction is open: the reader opens the
 * database file (lazily: only the rows it needs are read) and answers from it, while the server goes on with other
 * requests. The file is the database as it is (the server saves each change before a read can go out), and a
 * reader opens it again when the server has saved it since. Changes, transactions and everything else stay with
 * the server, one at a time; the requests of one client are answered in the order they came. */
#define MAX_READERS 16

typedef struct {
    Link link;
    Buf in;
    bool ready;                 /* it said hello with the token */
    int client;                 /* the client (its place) whose request it is running; -1 none */
    int client_id;
    bool alive;
} Reader;

static void make_token(char out[33]) {
    unsigned char r[16] = {0};
    enlngdb_random_bytes(r, sizeof(r));
    for (int i = 0; i < 16; i++) snprintf(out + i * 2, 3, "%02x", r[i]);
}

/* the port a listening socket got */
static int socket_port(qsock s) {
    struct sockaddr_in a;
#ifdef _WIN32
    int n = (int)sizeof(a);
#else
    socklen_t n = sizeof(a);
#endif
    memset(&a, 0, sizeof(a));
    if (p_getsockname(s, (struct sockaddr*)&a, &n) != 0) return -1;
    const unsigned char* b = (const unsigned char*)&a.sin_port;       /* network order (ntohs would need ws2_32 linked) */
    return (b[0] << 8) | b[1];
}

static bool spawn_reader(int port, const char* token) {
#ifdef _WIN32
    char exe[1024];
    DWORD n = GetModuleFileNameA(NULL, exe, sizeof(exe));
    if (n == 0 || n >= sizeof(exe)) return false;
    char cmd[1200];
    snprintf(cmd, sizeof(cmd), "\"%s\" --reader %d %s", exe, port, token);
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    memset(&si, 0, sizeof(si));
    si.cb = sizeof(si);
    memset(&pi, 0, sizeof(pi));
    if (!CreateProcessA(exe, cmd, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) return false;
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    return true;
#else
    char exe[1024];
    ssize_t n = readlink("/proc/self/exe", exe, sizeof(exe) - 1);
    if (n <= 0) return false;
    exe[n] = '\0';
    pid_t pid = fork();
    if (pid < 0) return false;
    if (pid == 0) {
        char ps[16];
        snprintf(ps, sizeof(ps), "%d", port);
        execl(exe, exe, "--reader", ps, token, (char*)NULL);
        _exit(1);
    }
    signal(SIGCHLD, SIG_IGN);       /* nobody waits for it */
    return true;
#endif
}

/* the reader process: one connection to the server, a request at a time */
static int reader_main(const char* port, const char* token) {
    char err[300];
    qsock s = open_socket("127.0.0.1", port, false, err, sizeof(err));
    if (s == QBAD) return 1;
    Link L;
    memset(&L, 0, sizeof(L));
    L.sock = s;
    if (!send_msg(&L, "hello", token, strlen(token))) return 1;
    typedef struct { char name[ENLNGDB_MAX_NAME]; unsigned long gen; EnlngDatabase* db; } Open;
    Open open[MAX_DATABASES];
    int nopen = 0;
    Buf in;
    memset(&in, 0, sizeof(in));
    for (;;) {
        char chunk[65536];
        int k = q_recv(s, chunk, sizeof(chunk));
        if (k <= 0) break;
        if (!buf_add(&in, chunk, (size_t)k)) break;
        for (;;) {
            char word[32];
            const char* body = NULL;
            size_t bn = 0;
            long used = take_msg(in.p, in.len, word, sizeof(word), &body, &bn);
            if (used < 0) goto done;
            if (used == 0) break;
            /* "job": the database, the generation of its file, the user, the client, then the script */
            char* text = (char*)malloc(bn + 1);
            if (!text) goto done;
            memcpy(text, body, bn);
            text[bn] = '\0';
            buf_drop(&in, (size_t)used);
            char* parts[4];
            char* p = text;
            bool good = strcmp(word, "job") == 0;
            for (int i = 0; i < 4 && good; i++) {
                parts[i] = p;
                char* nl = strchr(p, '\n');
                if (!nl) { good = false; break; }
                *nl = '\0';
                p = nl + 1;
            }
            if (!good) { free(text); goto done; }
            const char* name = parts[0];
            unsigned long gen = strtoul(parts[1], NULL, 10);
            const char* user = parts[2];
            int session = atoi(parts[3]);
            int at = -1;
            for (int i = 0; i < nopen; i++) if (strcmp(open[i].name, name) == 0) at = i;
            if (at < 0 && nopen < MAX_DATABASES) { at = nopen++; memset(&open[at], 0, sizeof(open[at])); snprintf(open[at].name, sizeof(open[at].name), "%s", name); }
            char* out = NULL;
            int failed = 0;
            if (at >= 0 && (!open[at].db || open[at].gen != gen)) {
                /* the file changed since: opened again (lazily) */
                if (open[at].db) enlngdb_free(open[at].db);
                open[at].db = enlngdb_create(name);
                char file[300];
                snprintf(file, sizeof(file), "%s.edb", name);
                enlngdb_load(open[at].db, file);
                open[at].gen = gen;
            }
            if (at >= 0) out = enlngdb_run_as(open[at].db, user[0] ? user : NULL, session, p, &failed);
            size_t ol = out ? strlen(out) : 0;
            char* reply_text = (char*)malloc(ol + 16);
            if (!reply_text) { free(out); free(text); goto done; }
            int h = snprintf(reply_text, 16, "%d\n", failed);
            if (ol) memcpy(reply_text + h, out, ol);
            bool sent = send_msg(&L, "done", reply_text, (size_t)h + ol);
            free(reply_text);
            free(out);
            free(text);
            if (!sent) goto done;
        }
    }
done:
    for (int i = 0; i < nopen; i++) if (open[i].db) enlngdb_free(open[i].db);
    free(in.p);
    q_close(s);
    return 0;
}

static int free_reader(Reader* rd) {
    for (int i = 0; i < MAX_READERS; i++) if (rd[i].link.sock != QBAD && rd[i].ready && rd[i].client < 0) return i;
    return -1;
}

static int cpu_count(void) {
#ifdef _WIN32
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    return (int)si.dwNumberOfProcessors;
#else
    long n = sysconf(_SC_NPROCESSORS_ONLN);
    return n > 0 ? (int)n : 1;
#endif
}

static int serve(const char** names, int nnames, const char* host, const char* port, const char* key_path, bool plain, int nreaders) {
    if (plain && strcmp(host, "127.0.0.1") != 0) { fprintf(stderr, "ERROR: --plain (no encryption) is only for this computer: leave out --host, or leave out --plain.\n"); return 1; }
    Served sv[MAX_DATABASES];
    int nsv = 0;
    char err[400];
    int status = 0;
    for (int i = 0; i < nnames && nsv < MAX_DATABASES; i++) {
        Served* s = &sv[nsv];
        memset(s, 0, sizeof(*s));
        snprintf(s->name, sizeof(s->name), "%s", names[i]);
        s->db = enlngdb_create(names[i]);
        if (!enlngdb_lock(s->db, names[i], err, sizeof(err))) { fprintf(stderr, "ERROR: %s.\n", err); enlngdb_free(s->db); status = 1; break; }
        snprintf(s->file, sizeof(s->file), "%s.edb", names[i]);
        {
            /* an encrypted database: the password from ENLNGDB_DB_PASSWORD, or asked for here (three tries) */
            const char* env = getenv("ENLNGDB_DB_PASSWORD");
            enlngdb_open_password = env && *env ? env : NULL;
            bool opened = enlngdb_load(s->db, s->file);
            enlngdb_open_password = NULL;
            for (int tries = 0; !opened && enlngdb_load_error && tries < 3 && is_terminal(); tries++) {
                char pw[256] = "";
                printf("%s for database '%s': ", enlngdb_load_error == 1 ? "Password" : "Wrong password; again", names[i]);
                fflush(stdout);
                read_secret(pw, sizeof(pw));
                enlngdb_open_password = pw;
                opened = enlngdb_load(s->db, s->file);
                enlngdb_open_password = NULL;
                memset(pw, 0, sizeof(pw));
            }
            if (!opened && enlngdb_load_error) {
                fprintf(stderr, "ERROR: the database '%s' is encrypted, and %s. Set ENLNGDB_DB_PASSWORD, or start the server in a terminal to type it.\n",
                        names[i], enlngdb_load_error == 1 ? "no password was given" : "the password is wrong");
                enlngdb_free(s->db);
                status = 1;
                break;
            }
        }
        enlngdb_materialize(s->db);
        s->gen = 1;
        nsv++;
        if (!enlngdb_has_users(s->db)) {
            fprintf(stderr, "ERROR: the database '%s' has no users, so nobody could sign in.\n"
                            "       Make one first, in a script that says 'use database %s':\n"
                            "       create user admin password \"...\" admin\n", names[i], names[i]);
            status = 1;
            break;
        }
    }
    uint8_t priv[32], pub[32];
    bool made = false;
    if (!status && !plain && !server_key(key_path, priv, pub, &made)) { fprintf(stderr, "ERROR: the server's key %s could not be read or made.\n", key_path); status = 1; }
    qsock ls = QBAD;
    if (!status) {
        ls = open_socket(host, port, true, err, sizeof(err));
        if (ls == QBAD) { fprintf(stderr, "ERROR: %s.\n", err); status = 1; }
    }
    if (status) {
        for (int i = 0; i < nsv; i++) enlngdb_free(sv[i].db);
        return 1;
    }
    watch_stop();
    printf("Serving");
    for (int i = 0; i < nsv; i++) printf("%s '%s'", i ? (i == nsv - 1 ? " and" : ",") : "", sv[i].name);
    printf(" at %s:%s. Stop with Ctrl+C.\n", host, port);
    if (plain) printf("NOTE: --plain: nothing is encrypted.\n");
    else {
        char fp[64];
        fingerprint(pub, fp);
        printf("Key fingerprint: %s (clients check it the first time they connect)\n", fp);
        if (made) printf("A new key was made in %s: keep this file private; it is what proves this server is this server.\n", key_path);
    }
    fflush(stdout);
    /* the readers: a private socket on this computer, and a token they must say */
    static Reader rd_[MAX_READERS];
    for (int i = 0; i < MAX_READERS; i++) { memset(&rd_[i], 0, sizeof(rd_[i])); rd_[i].link.sock = QBAD; rd_[i].client = -1; }
    qsock rls = QBAD;
    char token[33] = "";
    int rport = -1;
    if (nreaders > MAX_READERS) nreaders = MAX_READERS;
    for (int i = 0; i < nsv; i++) if (sv[i].db->encrypted) nreaders = 0;     /* an encrypted file: the server alone */
    if (nreaders > 0) {
        char rerr[300];
        rls = open_socket("127.0.0.1", "0", true, rerr, sizeof(rerr));
        rport = rls == QBAD ? -1 : socket_port(rls);
        if (rport <= 0) { if (rls != QBAD) q_close(rls); rls = QBAD; nreaders = 0; }
        make_token(token);
        int started = 0;
        for (int i = 0; i < nreaders; i++) started += spawn_reader(rport, token) ? 1 : 0;
        nreaders = started;
        if (nreaders > 0) printf("%d reader processes answer the requests that only read, at the same time (--readers 0: none).\n", nreaders);
        fflush(stdout);
    }
    static Client cl[MAX_CLIENTS];
    for (int i = 0; i < MAX_CLIENTS; i++) { memset(&cl[i], 0, sizeof(cl[i])); cl[i].link.sock = QBAD; cl[i].at_reader = -1; }
    int next_id = 1;
    bool quit = false;
    bool more = false;
    while (!quit && !stopping()) {
        fd_set rd;
        FD_ZERO(&rd);
        set_add(ls, &rd);
        int top = (int)ls;
        for (int i = 0; i < MAX_CLIENTS; i++) if (cl[i].link.sock != QBAD) { set_add(cl[i].link.sock, &rd); if ((int)cl[i].link.sock > top) top = (int)cl[i].link.sock; }
        if (rls != QBAD) { set_add(rls, &rd); if ((int)rls > top) top = (int)rls; }
        for (int i = 0; i < MAX_READERS; i++) if (rd_[i].link.sock != QBAD) { set_add(rd_[i].link.sock, &rd); if ((int)rd_[i].link.sock > top) top = (int)rd_[i].link.sock; }
        struct timeval tv = {0, more ? 0 : 200000};      /* requests are waiting behind an answer that was held: no wait */
        more = false;
        int r = p_select(top + 1, &rd, NULL, NULL, &tv);
        if (r < 0) continue;
        if (in_set(ls, &rd)) {
            qsock s = p_accept(ls, NULL, NULL);
            if (s != QBAD) {
                int k = -1;
                for (int i = 0; i < MAX_CLIENTS && k < 0; i++) if (cl[i].link.sock == QBAD) k = i;
                if (k < 0) q_close(s);
                else { cl[k].link.sock = s; cl[k].id = next_id++; cl[k].hello_done = plain; cl[k].at_reader = -1; }
            }
        }
        if (rls != QBAD && in_set(rls, &rd)) {
            qsock s = p_accept(rls, NULL, NULL);
            if (s != QBAD) {
                int k = -1;
                for (int i = 0; i < MAX_READERS && k < 0; i++) if (rd_[i].link.sock == QBAD) k = i;
                if (k < 0) q_close(s);
                else { memset(&rd_[k], 0, sizeof(rd_[k])); rd_[k].link.sock = s; rd_[k].client = -1; rd_[k].alive = true; }
            }
        }
        /* what the readers send: hello (with the token), and the answers */
        for (int i = 0; i < MAX_READERS; i++) {
            Reader* R = &rd_[i];
            if (R->link.sock == QBAD || !in_set(R->link.sock, &rd)) continue;
            char chunk[65536];
            int k = q_recv(R->link.sock, chunk, sizeof(chunk));
            bool good = k > 0 && buf_add(&R->in, chunk, (size_t)k);
            while (good) {
                char word[32];
                const char* body = NULL;
                size_t bn = 0;
                long used = take_msg(R->in.p, R->in.len, word, sizeof(word), &body, &bn);
                if (used < 0) { good = false; break; }
                if (used == 0) break;
                if (strcmp(word, "hello") == 0) {
                    good = bn == strlen(token) && memcmp(body, token, bn) == 0;
                    R->ready = good;
                } else if (strcmp(word, "done") == 0 && R->ready && R->client >= 0) {
                    Client* c = &cl[R->client];
                    const char* nl = (const char*)memchr(body, '\n', bn);
                    int failed = atoi(body);
                    const char* out = nl ? nl + 1 : body + bn;
                    if (c->link.sock != QBAD && c->id == R->client_id) {
                        send_msg(&c->link, failed ? "failed" : "ok", out, (size_t)(body + bn - out));
                        c->at_reader = -1;
                    }
                    R->client = -1;
                } else {
                    good = false;
                }
                buf_drop(&R->in, (size_t)used);
            }
            if (!good) {
                /* the reader is gone: a request it had is answered with the reason, and another reader is started */
                if (R->client >= 0) {
                    Client* c = &cl[R->client];
                    if (c->link.sock != QBAD && c->id == R->client_id) { reply(c, "failed", "ERROR: the process that read the database stopped; nothing was changed, try again.\n"); c->at_reader = -1; }
                }
                q_close(R->link.sock);
                free(R->in.p);
                memset(R, 0, sizeof(*R));
                R->link.sock = QBAD;
                R->client = -1;
                spawn_reader(rport, token);
            }
        }
        for (int i = 0; i < MAX_CLIENTS; i++) {
            Client* c = &cl[i];
            if (c->link.sock == QBAD || !in_set(c->link.sock, &rd)) continue;
            char chunk[65536];
            int k = q_recv(c->link.sock, chunk, sizeof(chunk));
            bool good = k > 0;
            if (good) good = buf_add(plain ? &c->in : &c->raw, chunk, (size_t)k);
            if (good && !c->hello_done) good = server_hello(c, priv, pub);
            if (good && c->link.sealed) good = open_frames(&c->link, &c->raw, &c->in);
            if (good && (c->in.len > MAX_MESSAGE + 128 || c->raw.len > MAX_MESSAGE + 512)) good = false;
            if (!good) {
                /* gone, or sent something wrong or changed: a transaction it left open is rolled back */
                end_txn_of(sv, c);
                drop_client(c);
            }
        }
        /* the requests that are all there, one client after another */
        for (int i = 0; i < MAX_CLIENTS && !quit; i++) {
            Client* c = &cl[i];
            while (c->link.sock != QBAD && c->in.len > 0 && !quit && c->at_reader < 0 && !c->held) {
                char word[32];
                const char* body = NULL;
                size_t bn = 0;
                long used = take_msg(c->in.p, c->in.len, word, sizeof(word), &body, &bn);
                if (used < 0) { end_txn_of(sv, c); drop_client(c); break; }
                if (used == 0) break;
                char* text = (char*)malloc(bn + 1);
                if (!text) { end_txn_of(sv, c); drop_client(c); break; }
                memcpy(text, body, bn);
                text[bn] = '\0';
                bool consumed = true, gone = false;
                if (strcmp(word, "login") == 0) {
                    /* "database\nuser\npassword", or "user\npassword" when one database is served */
                    char* l1 = text;
                    char* l2 = strchr(l1, '\n');
                    char* l3 = l2 ? strchr(l2 + 1, '\n') : NULL;
                    const char *dbname = NULL, *user = l1, *pw = "";
                    if (l2) *l2 = '\0';
                    if (l3) { *l3 = '\0'; dbname = l1; user = l2 + 1; pw = l3 + 1; }
                    else if (l2) pw = l2 + 1;
                    int dbi = dbname && dbname[0] ? find_served(sv, nsv, dbname) : (nsv == 1 ? 0 : -1);
                    bool admin = false;
                    if (c->signed_in) {
                        reply(c, "denied", "this connection has signed in already; to use another database, connect again");
                    } else if (dbi < 0) {
                        char m[300];
                        snprintf(m, sizeof(m), "%s: say which one, as in --connect HOST:PORT/%s", dbname && dbname[0] ? "this server does not serve that database" : "this server serves several databases", sv[0].name);
                        reply(c, "denied", m);
                    } else if (enlngdb_login(sv[dbi].db, user, pw, &admin)) {
                        c->signed_in = true;
                        c->admin = admin;
                        c->dbi = dbi;
                        snprintf(c->user, sizeof(c->user), "%s", user);
                        for (int u = 0; u < sv[dbi].db->user_count; u++) {          /* the name as it was made */
                            const char* x = sv[dbi].db->users[u].name;
                            size_t k = 0;
                            while (x[k] && user[k] && tolower((unsigned char)x[k]) == tolower((unsigned char)user[k])) k++;
                            if (!x[k] && !user[k]) snprintf(c->user, sizeof(c->user), "%s", x);
                        }
                        char m[200];
                        snprintf(m, sizeof(m), "Signed in to '%s' as %s%s%s.\n", sv[dbi].name, c->user, admin ? " (an admin)" : "", c->link.sealed ? "; the connection is encrypted" : "");
                        reply(c, "ok", m);
                        printf("signed in: %s to %s (client %d)\n", c->user, sv[dbi].name, c->id);
                        fflush(stdout);
                    } else {
                        c->fails++;
                        reply(c, "denied", "the name or the password is wrong");
                        if (c->fails >= 3) gone = true;
                    }
                    memset(text, 0, bn);
                } else if (strcmp(word, "bye") == 0) {
                    reply(c, "ok", "");
                    gone = true;
                } else if (strcmp(word, "run") == 0) {
                    Served* s = &sv[c->dbi];
                    if (!c->signed_in) {
                        reply(c, "denied", "sign in first");
                    } else if (s->db->txn && s->txn_owner != 0 && s->txn_owner != c->id) {
                        /* another client's transaction is open: this request waits for it */
                        if (!c->waiting_since) c->waiting_since = time(NULL);
                        if (time(NULL) - c->waiting_since > WAIT_FOR_TXN_SECONDS) {
                            reply(c, "busy", "another user has had a transaction open for a while; nothing was run, try again");
                            c->waiting_since = 0;
                        } else {
                            consumed = false;
                        }
                    } else if (c->admin && strcmp(text, "shutdown server") == 0) {
                        reply(c, "ok", "The server stops.\n");
                        quit = true;
                    } else if (rls != QBAD && !s->dirty && !s->stale && !s->db->txn && enlngdb_zone_default() && enlngdb_script_reads_only(text) &&
                               (c->at_reader = free_reader(rd_)) >= 0) {
                        /* only reads, and the file is the database as it is: a reader answers, and the server goes on */
                        Reader* R = &rd_[c->at_reader];
                        size_t tl = strlen(text);
                        char* job = (char*)malloc(tl + 200);
                        int h = job ? snprintf(job, 200, "%s\n%lu\n%s\n%d\n", s->name, s->gen, c->user, c->id) : 0;
                        bool sent = job != NULL;
                        if (sent) { memcpy(job + h, text, tl); sent = send_msg(&R->link, "job", job, (size_t)h + tl); }
                        free(job);
                        if (sent) { R->client = i; R->client_id = c->id; c->waiting_since = 0; }
                        else {
                            c->at_reader = -1;
                            int failed = 0;
                            char* out = enlngdb_run_as(s->db, c->user, c->id, text, &failed);
                            send_msg(&c->link, failed ? "failed" : "ok", out ? out : "", out ? strlen(out) : 0);
                            free(out);
                        }
                    } else {
                        c->waiting_since = 0;
                        unsigned long long before = enlngdb_mutations;
                        int failed = 0;
                        char* out = enlngdb_run_as(s->db, c->user, c->id, text, &failed);
                        bool changed = enlngdb_mutations != before;
                        if (changed) s->dirty = true;
                        s->txn_owner = s->db->txn ? (s->txn_owner ? s->txn_owner : c->id) : 0;
                        if (changed && !s->db->txn && out) {
                            /* it changed the database: the answer waits until the change is on the disk. The requests
                               of the other clients that are here now run first, and one save writes them all. */
                            c->held = out;
                            c->held_failed = failed != 0;
                        } else {
                            send_msg(&c->link, failed ? "failed" : "ok", out ? out : "", out ? strlen(out) : 0);
                            free(out);
                        }
                    }
                } else {
                    reply(c, "denied", "the server knows: login, run, bye");
                }
                free(text);
                if (!consumed) break;
                buf_drop(&c->in, (size_t)used);
                if (gone) {
                    end_txn_of(sv, c);
                    drop_client(c);
                    break;
                }
            }
        }
        for (int i = 0; i < nsv; i++) {
            if (sv[i].dirty && !sv[i].db->txn) {
                bool saved = enlngdb_save(sv[i].db, sv[i].file);
                if (!saved) fprintf(stderr, "ERROR: the database could not be saved to %s.\n", sv[i].file);
                sv[i].dirty = false;
                sv[i].stale = !saved;
                if (saved) sv[i].gen++;
            }
        }
        /* the answers that waited for the save */
        for (int i = 0; i < MAX_CLIENTS; i++) {
            Client* c = &cl[i];
            if (!c->held) continue;
            char* out = c->held;
            c->held = NULL;
            if (c->link.sock != QBAD) {
                if (sv[c->dbi].stale) {
                    const char* note = "\nERROR: the server could not write this change to its disk. It is in the server's memory, and is written with the next change that can be saved.\n";
                    size_t a = strlen(out), b = strlen(note);
                    char* both = (char*)malloc(a + b + 1);
                    if (both) { memcpy(both, out, a); memcpy(both + a, note, b + 1); send_msg(&c->link, "failed", both, a + b); free(both); }
                    else send_msg(&c->link, "failed", out, a);
                } else {
                    send_msg(&c->link, c->held_failed ? "failed" : "ok", out, strlen(out));
                }
                if (c->in.len > 0) more = true;
            }
            free(out);
        }
    }
    for (int i = 0; i < MAX_CLIENTS; i++) if (cl[i].link.sock != QBAD) drop_client(&cl[i]);
    for (int i = 0; i < MAX_READERS; i++) if (rd_[i].link.sock != QBAD) { q_close(rd_[i].link.sock); free(rd_[i].in.p); }     /* they end */
    if (rls != QBAD) q_close(rls);
    for (int i = 0; i < nsv; i++) {
        if (sv[i].db->txn) enlngdb_rollback(sv[i].db);
        if (sv[i].dirty) enlngdb_save(sv[i].db, sv[i].file);
        enlngdb_free(sv[i].db);
    }
    q_close(ls);
    memset(priv, 0, sizeof(priv));
    printf("The server stopped; the databases are saved.\n");
    return 0;
}

/* ---- the client ---- */

/* the file of trusted servers: lines "host:port key-in-hex" */
static void known_path(char* out, size_t n) {
    const char* env = getenv("ENLNGDB_KNOWN_SERVERS");
    if (env && *env) snprintf(out, n, "%s", env);
    else snprintf(out, n, "%s/.enlngdb_known_servers", home_dir());
}

/* 1: the key is the one known for this server; 0: no key is known; -1: a different key is known */
static int known_key(const char* where, const uint8_t pub[32]) {
    char path[600], line[400];
    known_path(path, sizeof(path));
    FILE* f = fopen(path, "r");
    if (!f) return 0;
    int found = 0;
    while (fgets(line, sizeof(line), f)) {
        char w[300], hex[100];
        if (sscanf(line, "%299s %99s", w, hex) != 2 || strcmp(w, where) != 0) continue;
        uint8_t k[32];
        if (strlen(hex) == 64 && from_hex(hex, k, 32)) found = memcmp(k, pub, 32) == 0 ? 1 : -1;
    }
    fclose(f);
    return found;
}

static bool remember_key(const char* where, const uint8_t pub[32]) {
    char path[600], hex[65];
    known_path(path, sizeof(path));
    FILE* f = fopen(path, "a");
    if (!f) return false;
    to_hex(pub, 32, hex);
    fprintf(f, "%s %s\n", where, hex);
    bool ok = fclose(f) == 0;
    keep_private(path);
    return ok;
}

static bool same_fingerprint(const char* a, const char* b) {
    for (;;) {
        while (*a == '-' || *a == ':' || *a == ' ') a++;
        while (*b == '-' || *b == ':' || *b == ' ') b++;
        if (!*a || !*b) return !*a && !*b;
        if (tolower((unsigned char)*a) != tolower((unsigned char)*b)) return false;
        a++;
        b++;
    }
}

/* a whole message from the server: its word and its text (the caller frees it) */
static char* read_answer(Link* L, Buf* raw, Buf* in, char* word, size_t wn) {
    for (;;) {
        const char* body = NULL;
        size_t bn = 0;
        long used = in->len ? take_msg(in->p, in->len, word, wn, &body, &bn) : 0;
        if (used < 0) return NULL;
        if (used > 0) {
            char* out = (char*)malloc(bn + 1);
            if (out) { memcpy(out, body, bn); out[bn] = '\0'; }
            buf_drop(in, (size_t)used);
            return out;
        }
        char chunk[65536];
        int k = q_recv(L->sock, chunk, sizeof(chunk));
        if (k <= 0) return NULL;
        if (!buf_add(L->sealed ? raw : in, chunk, (size_t)k)) return NULL;
        if (L->sealed && !open_frames(L, raw, in)) return NULL;
    }
}

typedef struct { Link link; Buf raw, in; } Conn;

static bool ask(Conn* C, const char* word, const char* text, size_t n, int* status) {
    if (!send_msg(&C->link, word, text, n)) { fprintf(stderr, "ERROR: the server went away.\n"); *status = 1; return false; }
    char w[32] = "";
    char* out = read_answer(&C->link, &C->raw, &C->in, w, sizeof(w));
    if (!out) { fprintf(stderr, "ERROR: the server went away (or its answer was changed on the way).\n"); *status = 1; return false; }
    if (strcmp(w, "ok") != 0 && strcmp(w, "failed") != 0) { printf("%s: %s\n", strcmp(w, "busy") == 0 ? "BUSY" : "DENIED", out); *status = 1; }
    else { fputs(out, stdout); if (strcmp(w, "failed") == 0) *status = 1; }
    fflush(stdout);
    free(out);
    return strcmp(w, "denied") != 0;
}

/* the client's half of the handshake: checks the server's key against the known one (or --trust) */
static bool client_hello(Conn* C, const char* where, const char* trust) {
    uint8_t e[32], E[32], hello[36];
    if (!enlngdb_random_bytes(e, 32)) { fprintf(stderr, "ERROR: this computer gave no random numbers.\n"); return false; }
    x25519_public(E, e);
    memcpy(hello, "EDB1", 4);
    memcpy(hello + 4, E, 32);
    if (!send_all(C->link.sock, (const char*)hello, sizeof(hello))) return false;
    uint8_t got[68];
    size_t have = 0;
    while (have < sizeof(got)) {
        int k = q_recv(C->link.sock, (char*)got + have, sizeof(got) - have);
        if (k <= 0) { fprintf(stderr, "ERROR: the server did not answer the encrypted greeting (a server started with --plain? then use --plain too).\n"); return false; }
        have += (size_t)k;
    }
    if (memcmp(got, "EDB1", 4) != 0) { fprintf(stderr, "ERROR: this is not an enlngdb server.\n"); return false; }
    const uint8_t* S = got + 4;
    const uint8_t* F = got + 36;
    char fp[64];
    fingerprint(S, fp);
    int known = known_key(where, S);
    if (known < 0) {
        char path[600];
        known_path(path, sizeof(path));
        fprintf(stderr, "ERROR: the server at %s shows a DIFFERENT key than before (fingerprint %s).\n"
                        "       Someone may be in between. Nothing was sent. If the server's key was really changed,\n"
                        "       take its line out of %s and connect again.\n", where, fp, path);
        return false;
    }
    if (known == 0) {
        bool ok = false;
        if (trust) {
            ok = same_fingerprint(trust, fp);
            if (!ok) { fprintf(stderr, "ERROR: the server at %s has the fingerprint %s, not %s. Nothing was sent.\n", where, fp, trust); return false; }
        } else if (is_terminal()) {
            printf("The server at %s is new to this computer. Its key fingerprint is\n    %s\n"
                   "Is it the one the server printed when it started? (yes/no) ", where, fp);
            fflush(stdout);
            char ans[32] = "";
            if (fgets(ans, sizeof(ans), stdin)) ok = tolower((unsigned char)ans[0]) == 'y';
            if (!ok) { fprintf(stderr, "Not trusted; nothing was sent.\n"); return false; }
        } else {
            fprintf(stderr, "ERROR: the server at %s is new to this computer; its key fingerprint is %s.\n"
                            "       If that is what the server printed, connect once with --trust %s\n", where, fp, fp);
            return false;
        }
        if (!remember_key(where, S)) fprintf(stderr, "NOTE: the server's key could not be saved for next time.\n");
    }
    uint8_t dh1[32], dh2[32];
    x25519(dh1, e, S);
    x25519(dh2, e, F);
    memset(e, 0, sizeof(e));
    if (all_zero(dh1, 32) || all_zero(dh2, 32)) { fprintf(stderr, "ERROR: the server sent a key that cannot be used.\n"); return false; }
    derive_keys(dh1, dh2, E, S, F, C->link.k_out, C->link.k_in);
    memset(dh1, 0, 32);
    memset(dh2, 0, 32);
    C->link.sealed = true;
    /* the server proves it holds its key: only then can it seal this */
    char w[32] = "";
    char* ready = read_answer(&C->link, &C->raw, &C->in, w, sizeof(w));
    bool ok = ready && strcmp(w, "ready") == 0;
    free(ready);
    if (!ok) fprintf(stderr, "ERROR: the server could not prove it holds its key. Nothing was sent.\n");
    return ok;
}

static int connect_to(const char* where_arg, const char* user, const char* script_path, const char* trust, bool plain) {
    char where[300], host[256] = "127.0.0.1", port[32] = "", dbname[ENLNGDB_MAX_NAME] = "";
    snprintf(where, sizeof(where), "%s", where_arg);
    char* slash = strchr(where, '/');
    if (slash) { snprintf(dbname, sizeof(dbname), "%s", slash + 1); *slash = '\0'; }
    const char* colon = strrchr(where, ':');
    if (colon) { snprintf(host, sizeof(host), "%.*s", (int)(colon - where), where); snprintf(port, sizeof(port), "%s", colon + 1); }
    else snprintf(port, sizeof(port), "%s", where);
    char server_id[300];
    snprintf(server_id, sizeof(server_id), "%s:%s", host, port);
    if (!user) { fprintf(stderr, "ERROR: say who you are: enlngdb --connect %s --user NAME\n", where_arg); return 1; }
    char* script = NULL;
    size_t script_len = 0;
    if (script_path) {
        FILE* f = fopen(script_path, "rb");
        if (!f) { fprintf(stderr, "ERROR: could not open %s\n", script_path); return 1; }
        fseek(f, 0, SEEK_END);
        long sz = ftell(f);
        fseek(f, 0, SEEK_SET);
        script = (char*)malloc((size_t)(sz > 0 ? sz : 0) + 1);
        script_len = script ? fread(script, 1, (size_t)(sz > 0 ? sz : 0), f) : 0;
        if (script) script[script_len] = '\0';
        fclose(f);
        if (!script) return 1;
    }
    char err[400];
    Conn C;
    memset(&C, 0, sizeof(C));
    C.link.sock = open_socket(host, port, false, err, sizeof(err));
    if (C.link.sock == QBAD) { fprintf(stderr, "ERROR: %s.\n", err); free(script); return 1; }
    if (!plain && !client_hello(&C, server_id, trust)) { q_close(C.link.sock); free(script); free(C.raw.p); free(C.in.p); return 1; }
    char password[256] = "";
    const char* env = getenv("ENLNGDB_PASSWORD");
    if (env) snprintf(password, sizeof(password), "%s", env);
    else { printf("Password for %s: ", user); fflush(stdout); read_secret(password, sizeof(password)); }
    int status = 0;
    char login[700];
    int ln = dbname[0] ? snprintf(login, sizeof(login), "%s\n%s\n%s", dbname, user, password) : snprintf(login, sizeof(login), "%s\n%s", user, password);
    memset(password, 0, sizeof(password));
    bool in = ask(&C, "login", login, (size_t)ln, &status);
    memset(login, 0, sizeof(login));
    if (in) {
        if (script) {
            ask(&C, "run", script, script_len, &status);
        } else if (is_terminal()) {
            printf("Type statements; 'exit' leaves.\n");
            char line[65536];
            for (;;) {
                printf("enlngdb> ");
                fflush(stdout);
                if (!fgets(line, sizeof(line), stdin)) break;
                line[strcspn(line, "\r\n")] = '\0';
                const char* t = line;
                while (isspace((unsigned char)*t)) t++;
                if (!*t) continue;
                if (strcmp(t, "exit") == 0 || strcmp(t, "quit") == 0) break;
                int one = 0;
                if (!ask(&C, "run", t, strlen(t), &one)) break;
            }
        } else {
            /* statements piped in: they go as one script */
            Buf all = {0};
            int ch;
            bool good = true;
            while (good && (ch = getchar()) != EOF) { char x = (char)ch; good = buf_add(&all, &x, 1); }
            if (good) ask(&C, "run", all.p ? all.p : "", all.len, &status);
            free(all.p);
        }
        send_msg(&C.link, "bye", "", 0);
    } else {
        status = 1;
    }
    q_close(C.link.sock);
    free(C.raw.p);
    free(C.in.p);
    free(script);
    return status;
}
#endif

int main(int argc, char* argv[]) {
    if (argc < 2 || strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
        printf("EnlngDB Pure C Native Database Engine v%s\n", ENLNGDB_VERSION);
        printf("Usage:\n");
        printf("  enlngdb <script.enlngdb>       Execute natural English database script\n");
        printf("  enlngdb -e \"<query>\"           Execute inline statement\n");
        printf("  enlngdb --serve NAME [NAME...] [--port 7171] [--host 127.0.0.1] [--key FILE]\n");
        printf("                                 Serve databases to signed-in users, encrypted\n");
        printf("  enlngdb --connect [HOST:]PORT[/NAME] --user USER [--trust FINGERPRINT] [script.enlngdb]\n");
        printf("                                 Sign in to a server and run a script or typed lines\n");
        printf("                                 (the password is asked for, or read from ENLNGDB_PASSWORD)\n");
        printf("  enlngdb --selftest             Check the encryption against known answers\n");
        printf("  enlngdb --version              Display version information\n");
        return 0;
    }

    if (strcmp(argv[1], "--version") == 0 || strcmp(argv[1], "-v") == 0) {
        printf("enlngdb version %s (Pure C Microsecond Database Engine)\n", ENLNGDB_VERSION);
        return 0;
    }

#ifndef __EMSCRIPTEN__
    if (strcmp(argv[1], "--selftest") == 0) return self_test();
    if (strcmp(argv[1], "--x25519") == 0 || strcmp(argv[1], "--seal") == 0) return crypto_tool(argc, argv);
    if (strcmp(argv[1], "--reader") == 0 && argc >= 4) return reader_main(argv[2], argv[3]);
    if (strcmp(argv[1], "--serve") == 0) {
        const char* names[MAX_DATABASES];
        int nnames = 0;
        const char* host = "127.0.0.1";
        const char* port = "7171";
        const char* key = "enlngdb_server.key";
        bool plain = false;
        int nreaders = cpu_count() - 1;
        if (nreaders > 4) nreaders = 4;
        if (nreaders < 0) nreaders = 0;
        for (int i = 2; i < argc; i++) {
            if (strcmp(argv[i], "--readers") == 0 && i + 1 < argc) { nreaders = atoi(argv[++i]); if (nreaders < 0) nreaders = 0; }
            else if (strcmp(argv[i], "--port") == 0 && i + 1 < argc) port = argv[++i];
            else if (strcmp(argv[i], "--host") == 0 && i + 1 < argc) host = argv[++i];
            else if (strcmp(argv[i], "--key") == 0 && i + 1 < argc) key = argv[++i];
            else if (strcmp(argv[i], "--plain") == 0) plain = true;
            else if (argv[i][0] == '-') { fprintf(stderr, "ERROR: I don't know the option '%s'\n", argv[i]); return 1; }
            else if (nnames < MAX_DATABASES) names[nnames++] = argv[i];
        }
        if (nnames == 0) { fprintf(stderr, "ERROR: which database? enlngdb --serve NAME [NAME...] [--port 7171] [--host 127.0.0.1]\n"); return 1; }
        return serve(names, nnames, host, port, key, plain, nreaders);
    }
    if (strcmp(argv[1], "--connect") == 0) {
        if (argc < 3) { fprintf(stderr, "ERROR: where? enlngdb --connect [HOST:]PORT[/NAME] --user NAME [script.enlngdb]\n"); return 1; }
        const char* user = NULL;
        const char* script = NULL;
        const char* trust = NULL;
        bool plain = false;
        for (int i = 3; i < argc; i++) {
            if (strcmp(argv[i], "--user") == 0 && i + 1 < argc) user = argv[++i];
            else if (strcmp(argv[i], "--trust") == 0 && i + 1 < argc) trust = argv[++i];
            else if (strcmp(argv[i], "--plain") == 0) plain = true;
            else script = argv[i];
        }
        return connect_to(argv[2], user, script, trust, plain);
    }
#endif

    if (strcmp(argv[1], "-e") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Error: Missing query string for -e.\n");
            return 1;
        }
        EnlngDatabase* db = enlngdb_create("default");
        int count = enlngdb_execute_script(db, argv[2], true);
        enlngdb_free(db);
        return count >= 0 ? 0 : 1;
    }

    const char* filepath = argv[1];
    return enlngdb_run_file(filepath);
}
