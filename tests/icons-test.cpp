/*
 * Tests of how RetroRun takes an icon out of a program: Windows' own libraries' icons, by their index and by their
 * resource ID, made into .ico files that draw as the icons ExtractIconEx gives of the libraries themselves; and
 * the icons that are not there. A temporary folder holds the .ico files.
 */
#include "../src/icons.h"
#include <shellapi.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>

namespace {

int g_checks;
int g_failures;

void
Check(const bool ok, const wchar_t *what, const std::wstring &got = L"")
{
    g_checks++;
    if (!ok) {
        g_failures++;
        wprintf(L"FAIL: %ls: got '%ls'\n", what, got.c_str());
    }
}

bool
WriteBytes(const std::wstring &path, const std::vector<char> &data)
{
    const HANDLE h = CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    DWORD        wrote = 0;

    if (h == INVALID_HANDLE_VALUE)
        return false;
    const bool ok = data.empty() || (WriteFile(h, data.data(), static_cast<DWORD>(data.size()), &wrote, nullptr) &&
                                     (wrote == data.size()));
    CloseHandle(h);
    return ok;
}

/* The icon's pixels as it draws at size by size. */
std::vector<DWORD>
Pixels(const HICON icon, const int size)
{
    BITMAPINFO         bi   = {};
    void              *bits = nullptr;
    std::vector<DWORD> pixels;
    const HDC          dc   = CreateCompatibleDC(nullptr);

    bi.bmiHeader.biSize     = sizeof(bi.bmiHeader);
    bi.bmiHeader.biWidth    = size;
    bi.bmiHeader.biHeight   = -size;
    bi.bmiHeader.biPlanes   = 1;
    bi.bmiHeader.biBitCount = 32;
    const HBITMAP bitmap    = CreateDIBSection(dc, &bi, DIB_RGB_COLORS, &bits, nullptr, 0);
    if (bitmap != nullptr) {
        const HGDIOBJ old = SelectObject(dc, bitmap);

        (void) DrawIconEx(dc, 0, 0, icon, size, size, 0, nullptr, DI_NORMAL);
        GdiFlush();
        pixels.assign(static_cast<const DWORD *>(bits), static_cast<const DWORD *>(bits) + size * size);
        (void) SelectObject(dc, old);
        DeleteObject(bitmap);
    }
    DeleteDC(dc);
    return pixels;
}

/* The large icon that ExtractIconEx gives of the file at index, drawn. */
std::vector<DWORD>
Drawn(const std::wstring &file, const int index)
{
    HICON              icon = nullptr;
    std::vector<DWORD> pixels;

    if ((ExtractIconExW(file.c_str(), index, &icon, nullptr, 1) != 0) && (icon != nullptr)) {
        pixels = Pixels(icon, 32);
        DestroyIcon(icon);
    }
    return pixels;
}

/* An .ico file's header and entries fit the file: one image or more, each within it. */
bool
WellFormed(const std::vector<char> &ico)
{
    WORD header[3] = {};

    if (ico.size() < sizeof(header))
        return false;
    memcpy(header, ico.data(), sizeof(header));
    if ((header[0] != 0) || (header[1] != 1) || (header[2] == 0) || (ico.size() < 6 + header[2] * 16u))
        return false;
    for (WORD i = 0; i < header[2]; i++) {
        DWORD bytes  = 0;
        DWORD offset = 0;

        memcpy(&bytes, ico.data() + 6 + i * 16 + 8, sizeof(bytes));
        memcpy(&offset, ico.data() + 6 + i * 16 + 12, sizeof(offset));
        if ((bytes == 0) || (offset < 6 + header[2] * 16u) || (offset + bytes > ico.size()))
            return false;
    }
    return true;
}

/* The program's icon, taken out and drawn from an .ico file of its own, against the icon ExtractIconEx draws of
   the program itself. */
void
Same(const std::wstring &dir, const std::wstring &program, const int index, const wchar_t *what)
{
    std::vector<char>  ico;
    const std::wstring file = dir + L"\\icon.ico";

    Check(ProgramIcon(program, index, &ico) && WellFormed(ico), what, std::to_wstring(ico.size()) + L" bytes");
    Check(WriteBytes(file, ico) && (ExtractIconExW(file.c_str(), -1, nullptr, nullptr, 0) == 1), what,
          L"an .ico file Windows does not read");
    const std::vector<DWORD> taken = Drawn(file, 0);
    Check(!taken.empty() && (taken == Drawn(program, index)), what, L"another icon");
    (void) DeleteFileW(file.c_str());
}

} /* namespace */

int
wmain()
{
    wchar_t           system[MAX_PATH];
    wchar_t           tmp[MAX_PATH];
    std::vector<char> ico;

    (void) GetSystemDirectoryW(system, MAX_PATH);
    (void) GetTempPathW(MAX_PATH, tmp);
    const std::wstring shell32  = std::wstring(system) + L"\\shell32.dll";
    const std::wstring imageres = std::wstring(system) + L"\\imageres.dll";
    const std::wstring dir      = std::wstring(tmp) + L"RetroRun-icons-test-" + std::to_wstring(GetCurrentProcessId());
    (void) CreateDirectoryW(dir.c_str(), nullptr);

    /* Icons there are, by their index and by their resource ID. */
    Same(dir, shell32, 0, L"a library's first icon");
    Same(dir, shell32, 3, L"a library's fourth icon");
    Same(dir, imageres, -28, L"an icon by its resource ID");

    /* Icons that are not there. */
    Check(!ProgramIcon(shell32, 100000, &ico), L"an index past the library's icons");
    Check(!ProgramIcon(shell32, -65000, &ico), L"a resource ID the library has no icon of");
    Check(!ProgramIcon(dir + L"\\none.exe", 0, &ico), L"a program that is not there");
    {
        const std::wstring text = dir + L"\\text.exe";

        Check(WriteBytes(text, std::vector<char>(100, 'x')) && !ProgramIcon(text, 0, &ico), L"a file that is no program");
        (void) DeleteFileW(text.c_str());
    }
    (void) RemoveDirectoryW(dir.c_str());

    wprintf(L"%d of %d checks passed\n", g_checks - g_failures, g_checks);
    return (g_failures == 0) ? 0 : 1;
}
