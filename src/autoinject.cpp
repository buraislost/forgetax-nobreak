// Background launcher + overlay: waits for ForgeTax.exe, injects autobreak.dll once per game process,
// and draws a click-through "made by bura" overlay on the game window while it is in front.
#include <windows.h>
#include <tlhelp32.h>
#include <cstdio>
#include <cstring>
#include <string>

static char g_dll[MAX_PATH], g_status[MAX_PATH];
static DWORD g_done = 0, g_pid = 0;
static HWND g_game = 0, g_ov = 0;

static DWORD FindPid() { DWORD pid = 0; PROCESSENTRY32 e{ sizeof e }; HANDLE s = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
  for (BOOL ok = Process32First(s, &e); ok; ok = Process32Next(s, &e)) if (!_stricmp(e.szExeFile, "ForgeTax.exe")) pid = e.th32ProcessID; CloseHandle(s); return pid; }
static bool HasModule(DWORD pid, const char* name) { HANDLE s = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid); if (s == INVALID_HANDLE_VALUE) return false;
  MODULEENTRY32 m{ sizeof m }; bool f = false; for (BOOL ok = Module32First(s, &m); ok; ok = Module32Next(s, &m)) if (!_stricmp(m.szModule, name)) f = true; CloseHandle(s); return f; }

static BOOL CALLBACK FindWin(HWND h, LPARAM lp) { DWORD p; GetWindowThreadProcessId(h, &p);
  if (p == g_pid && IsWindowVisible(h) && !GetWindow(h, GW_OWNER)) { *(HWND*)lp = h; return FALSE; } return TRUE; }

static bool NoBreakOn() { FILE* f = fopen(g_status, "r"); if (!f) return false; char b[256] = {}; fread(b, 1, 255, f); fclose(f); return strstr(b, "enabled=1") != 0; }

static void TryInject() {
  DWORD pid = FindPid(); if (!pid) { g_done = 0; g_pid = 0; return; } g_pid = pid; if (pid == g_done) return;
  if (!HasModule(pid, "mono-2.0-bdwgc.dll")) return; if (HasModule(pid, "autobreak.dll")) { g_done = pid; return; }
  HANDLE h = OpenProcess(PROCESS_ALL_ACCESS, 0, pid); if (!h) { printf("OpenProcess failed %lu\n", GetLastError()); return; }
  void* m = VirtualAllocEx(h, 0, MAX_PATH, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE); WriteProcessMemory(h, m, g_dll, strlen(g_dll) + 1, 0);
  HANDLE t = CreateRemoteThread(h, 0, 0, (LPTHREAD_START_ROUTINE)GetProcAddress(GetModuleHandleA("kernel32"), "LoadLibraryA"), m, 0, 0);
  if (t) { WaitForSingleObject(t, 10000); CloseHandle(t); printf("injected autobreak.dll into pid %lu\n", pid); g_done = pid; }
  else printf("CreateRemoteThread failed %lu\n", GetLastError());
  CloseHandle(h);
}

static LRESULT CALLBACK OvProc(HWND h, UINT m, WPARAM w, LPARAM l) {
  if (m == WM_PAINT) { PAINTSTRUCT ps; HDC dc = BeginPaint(h, &ps); RECT r; GetClientRect(h, &r);
    FillRect(dc, &r, (HBRUSH)GetStockObject(BLACK_BRUSH)); // black = transparent (color key)
    SetBkMode(dc, TRANSPARENT); HFONT f = CreateFontA(22, 0, 0, 0, FW_BOLD, 0, 0, 0, DEFAULT_CHARSET, 0, 0, ANTIALIASED_QUALITY, 0, "Segoe UI");
    HGDIOBJ o = SelectObject(dc, f);
    SetTextColor(dc, RGB(255, 190, 60)); TextOutA(dc, 6, 2, "made by bura", 12);
    bool on = NoBreakOn(); SetTextColor(dc, on ? RGB(120, 255, 120) : RGB(255, 110, 110));
    const char* s = on ? "No-Break: ON  (F8)" : "No-Break: OFF (F8)"; TextOutA(dc, 6, 28, s, (int)strlen(s));
    SelectObject(dc, o); DeleteObject(f); EndPaint(h, &ps); return 0; }
  return DefWindowProcA(h, m, w, l);
}

static void CALLBACK Tick(HWND, UINT, UINT_PTR, DWORD) {
  static int n = 0; if ((n++ % 4) == 0) TryInject();
  if (g_pid && (!g_game || !IsWindow(g_game))) { g_game = 0; EnumWindows(FindWin, (LPARAM)&g_game); }
  if (!g_pid) g_game = 0;
  bool show = g_game && GetForegroundWindow() == g_game && !IsIconic(g_game);
  if (!show) { ShowWindow(g_ov, SW_HIDE); return; }
  RECT c; GetClientRect(g_game, &c); POINT p{ 0, 0 }; ClientToScreen(g_game, &p);
  SetWindowPos(g_ov, HWND_TOPMOST, p.x + 16, p.y + 12, 220, 56, SWP_NOACTIVATE | SWP_SHOWWINDOW);
  InvalidateRect(g_ov, 0, TRUE);
}

int main() {
  GetModuleFileNameA(0, g_dll, MAX_PATH); *strrchr(g_dll, '\\') = 0; strcpy(g_status, g_dll); strcat(g_status, "\\status.txt"); strcat(g_dll, "\\autobreak.dll");
  SetConsoleTitleA("ForgeTax No-Break - made by bura"); puts("=== ForgeTax No-Break | made by bura ===");
  puts("autoinject: waiting for ForgeTax.exe (Ctrl+C to quit). In game: F8 toggles no-break.");
  WNDCLASSA wc{}; wc.lpfnWndProc = OvProc; wc.hInstance = GetModuleHandleA(0); wc.lpszClassName = "BuraOverlay"; RegisterClassA(&wc);
  g_ov = CreateWindowExA(WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE, "BuraOverlay", "", WS_POPUP, 0, 0, 220, 56, 0, 0, wc.hInstance, 0);
  SetLayeredWindowAttributes(g_ov, RGB(0, 0, 0), 0, LWA_COLORKEY);
  SetTimer(0, 1, 500, Tick);
  MSG m; while (GetMessageA(&m, 0, 0, 0) > 0) { TranslateMessage(&m); DispatchMessageA(&m); }
}
