// Standalone "unlock all cosmetics" mod for Geometry Dash (Windows build,
// works under Proton/Wine on Linux). Builds as xinput1_4.dll.
// Controller calls are passed on to the real system XInput DLL.

#include <windows.h>
#include <stdint.h>
#include <string.h>

// ---- Function offsets for YOUR game version -----------------------------
// Offsets from the start of GeometryDash.exe. Get the "win" values for
// 2.2091 from the geode-sdk/bindings repo on GitHub (GeometryDash.bro):
//   GameManager::isIconUnlocked(int, IconType)
//   GameManager::isColorUnlocked(int, UnlockType)
//   GameStatsManager::isItemUnlocked(UnlockType, int)
// Leave a value at 0 to skip that patch.
static const uintptr_t OFFSET_isIconUnlocked  = 0x0; // TODO
static const uintptr_t OFFSET_isColorUnlocked = 0x0; // TODO
static const uintptr_t OFFSET_isItemUnlocked  = 0x0; // TODO

// mov al, 1 ; ret  -> function immediately returns true
static const uint8_t RETURN_TRUE[] = { 0xB0, 0x01, 0xC3 };

static void patchReturnTrue(uintptr_t base, uintptr_t offset) {
    if (offset == 0) return;
    void* target = (void*)(base + offset);
    DWORD oldProtect;
    if (VirtualProtect(target, sizeof(RETURN_TRUE), PAGE_EXECUTE_READWRITE, &oldProtect)) {
        memcpy(target, RETURN_TRUE, sizeof(RETURN_TRUE));
        VirtualProtect(target, sizeof(RETURN_TRUE), oldProtect, &oldProtect);
        FlushInstructionCache(GetCurrentProcess(), target, sizeof(RETURN_TRUE));
    }
}

// ---- Pass-through to the real XInput -------------------------------------
static HMODULE g_self = NULL;
static HMODULE g_real = NULL;

static HMODULE realXInput() {
    if (g_real) return g_real;
    const char* names[] = { "xinput1_4.dll", "xinput1_3.dll" };
    char path[MAX_PATH];
    for (int i = 0; i < 2; i++) {
        UINT len = GetSystemDirectoryA(path, MAX_PATH);
        if (len == 0 || len + 20 > MAX_PATH) break;
        strcat(path, "\\");
        strcat(path, names[i]);
        HMODULE h = LoadLibraryA(path);
        if (h && h != g_self) { g_real = h; break; }
    }
    return g_real;
}

static FARPROC realProc(LPCSTR name) {
    HMODULE h = realXInput();
    return h ? GetProcAddress(h, name) : NULL;
}

#define NOT_CONNECTED 1167 /* ERROR_DEVICE_NOT_CONNECTED */

extern "C" {

DWORD WINAPI XInputGetState(DWORD i, void* s) {
    typedef DWORD (WINAPI *F)(DWORD, void*);
    F f = (F)realProc("XInputGetState");
    return f ? f(i, s) : NOT_CONNECTED;
}

DWORD WINAPI XInputGetStateEx(DWORD i, void* s) {
    typedef DWORD (WINAPI *F)(DWORD, void*);
    F f = (F)realProc((LPCSTR)100);
    if (!f) f = (F)realProc("XInputGetState");
    return f ? f(i, s) : NOT_CONNECTED;
}

DWORD WINAPI XInputSetState(DWORD i, void* v) {
    typedef DWORD (WINAPI *F)(DWORD, void*);
    F f = (F)realProc("XInputSetState");
    return f ? f(i, v) : NOT_CONNECTED;
}

DWORD WINAPI XInputGetCapabilities(DWORD i, DWORD fl, void* c) {
    typedef DWORD (WINAPI *F)(DWORD, DWORD, void*);
    F f = (F)realProc("XInputGetCapabilities");
    return f ? f(i, fl, c) : NOT_CONNECTED;
}

void WINAPI XInputEnable(BOOL e) {
    typedef void (WINAPI *F)(BOOL);
    F f = (F)realProc("XInputEnable");
    if (f) f(e);
}

DWORD WINAPI XInputGetBatteryInformation(DWORD i, BYTE t, void* b) {
    typedef DWORD (WINAPI *F)(DWORD, BYTE, void*);
    F f = (F)realProc("XInputGetBatteryInformation");
    return f ? f(i, t, b) : NOT_CONNECTED;
}

DWORD WINAPI XInputGetKeystroke(DWORD i, DWORD r, void* k) {
    typedef DWORD (WINAPI *F)(DWORD, DWORD, void*);
    F f = (F)realProc("XInputGetKeystroke");
    return f ? f(i, r, k) : NOT_CONNECTED;
}

DWORD WINAPI XInputGetAudioDeviceIds(DWORD i, LPWSTR r, UINT* rc, LPWSTR c, UINT* cc) {
    typedef DWORD (WINAPI *F)(DWORD, LPWSTR, UINT*, LPWSTR, UINT*);
    F f = (F)realProc("XInputGetAudioDeviceIds");
    return f ? f(i, r, rc, c, cc) : NOT_CONNECTED;
}

BOOL WINAPI DllMain(HINSTANCE inst, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        g_self = inst;
        DisableThreadLibraryCalls(inst);
        HMODULE gd = GetModuleHandleA("GeometryDash.exe");
        if (gd) {
            uintptr_t base = (uintptr_t)gd;
            patchReturnTrue(base, OFFSET_isIconUnlocked);
            patchReturnTrue(base, OFFSET_isColorUnlocked);
            patchReturnTrue(base, OFFSET_isItemUnlocked);
        }
    }
    return TRUE;
}

} // extern "C"
