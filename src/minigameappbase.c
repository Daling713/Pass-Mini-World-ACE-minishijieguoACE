#include <windows.h>

typedef long long (__fastcall *StartFn)(void *, void *, const char *, unsigned int);

typedef struct {
    unsigned char value;
    unsigned char is_wild;
} PatByte;

static int compile_pattern(const char *s, PatByte *out, int max) {
    int n = 0;
    while (*s && n < max) {
        while (*s == ' ') s++;
        if (!*s) break;
        if (*s == '?') {
            out[n].value = 0;
            out[n].is_wild = 1;
            n++;
            s++;
            if (*s == '?') s++;
        } else {
            int v = 0, digits = 0;
            while (*s && *s != ' ' && digits < 2) {
                int c = *s;
                int h;
                if (c >= '0' && c <= '9') h = c - '0';
                else if (c >= 'a' && c <= 'f') h = c - 'a' + 10;
                else if (c >= 'A' && c <= 'F') h = c - 'A' + 10;
                else break;
                v = (v << 4) | h;
                digits++;
                s++;
            }
            while (*s && *s != ' ') s++;
            out[n].value = (unsigned char)v;
            out[n].is_wild = 0;
            n++;
        }
        while (*s == ' ') s++;
    }
    return n;
}

static void *find_pattern(HMODULE mod, const char *pat) {
    PatByte pb[128];
    int n = compile_pattern(pat, pb, 128);
    unsigned char *base;
    IMAGE_DOS_HEADER *dos;
    IMAGE_NT_HEADERS *nt;
    SIZE_T size, i;
    if (n <= 0) return 0;
    base = (unsigned char *)mod;
    dos = (IMAGE_DOS_HEADER *)base;
    if (dos->e_magic != 0x5A4D) return 0;
    nt = (IMAGE_NT_HEADERS *)(base + dos->e_lfanew);
    if (nt->Signature != 0x00004550) return 0;
    size = nt->OptionalHeader.SizeOfImage;
    for (i = 0; i + (SIZE_T)n <= size; i++) {
        unsigned char *p = base + i;
        int j;
        for (j = 0; j < n; j++) {
            if (!pb[j].is_wild && p[j] != pb[j].value) break;
        }
        if (j == n) return p;
    }
    return 0;
}

static void patch_to_ret0(void *p) {
    DWORD old;
    unsigned char *b;
    if (!p) return;
    if (!VirtualProtect(p, 3, PAGE_EXECUTE_READWRITE, &old)) return;
    b = (unsigned char *)p;
    b[0] = 0x33;
    b[1] = 0xC0;
    b[2] = 0xC3;
    VirtualProtect(p, 3, old, &old);
    FlushInstructionCache(GetCurrentProcess(), p, 3);
}

BOOL WINAPI DllMain(HINSTANCE inst, DWORD reason, LPVOID reserved) {
    if (reason == DLL_PROCESS_ATTACH) DisableThreadLibraryCalls(inst);
    return TRUE;
}

void __stdcall Ordinal1(void) {
    HMODULE h = LoadLibraryA("libiworld.dll");
    void *ace;
    void *start;
    if (!h) return;

    ace = find_pattern(h, "40 53 48 81 EC 30 08 00 00");
    if (!ace) {
        unsigned char *f = (unsigned char *)h + 0x143AC50;
        if (f[0] == 0x40 && f[1] == 0x53 && f[2] == 0x48 && f[3] == 0x81 && f[4] == 0xEC)
            ace = f;
    }
    patch_to_ret0(ace);

    start = find_pattern(h,
        "48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 48 89 7C 24 ? 41 56 48 83 EC 30 4C 8B F1");
    if (!start) {
        void *hit = find_pattern(h, "49 8B F0 48 8B EA E8 ?? ?? ?? ?? 48 8B D8 48 85 C0");
        if (hit) start = (unsigned char *)hit - 0x25;
    }
    if (!start) return;

    ((StartFn)start)(0, 0, "apiid=110", 1);
}
