/*
 * Tests of how RetroRun finds shortcuts to drives and names them: the drive a shortcut links to, whether a program
 * made the shortcut by the drive's path or Explorer by its item, as when the drive is dragged out of This PC; the
 * shortcuts of a folder; the names shortcuts get after drives' names, with and without the drives' letters; names
 * free in a folder; a shortcut's own icon, given and taken away; and the identity a file keeps as it is renamed and
 * saved again. A temporary folder holds the shortcuts; nothing they link to is touched.
 */
#include "../src/shortcuts.h"
#include <objbase.h>
#include <shlobj.h>
#include <shobjidl.h>
#include <stdio.h>
#include <wchar.h>
#include <wctype.h>
#include <algorithm>

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

/* A shortcut to target: by its path, as a program makes one, or by its item, as Explorer makes one of an item
   dragged out of a folder. */
bool
MakeShortcut(const std::wstring &path, const std::wstring &target, const bool byItem)
{
    IShellLinkW  *link = nullptr;
    IPersistFile *file = nullptr;
    bool          ok   = false;

    if (FAILED(CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&link))))
        return false;
    if (byItem) {
        PIDLIST_ABSOLUTE item = nullptr;

        if (SUCCEEDED(SHParseDisplayName(target.c_str(), nullptr, &item, 0, nullptr))) {
            ok = SUCCEEDED(link->SetIDList(item));
            CoTaskMemFree(item);
        }
    } else
        ok = SUCCEEDED(link->SetPath(target.c_str()));
    if (ok && SUCCEEDED(link->QueryInterface(IID_PPV_ARGS(&file)))) {
        ok = SUCCEEDED(file->Save(path.c_str(), TRUE));
        file->Release();
    } else
        ok = false;
    link->Release();
    return ok;
}

bool
WriteBytes(const std::wstring &path, const void *data, const DWORD size)
{
    const HANDLE h = CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    DWORD        wrote = 0;

    if (h == INVALID_HANDLE_VALUE)
        return false;
    const bool ok = (size == 0) || (WriteFile(h, data, size, &wrote, nullptr) && (wrote == size));
    CloseHandle(h);
    return ok;
}

} /* namespace */

int
wmain()
{
    wchar_t tmp[MAX_PATH];
    wchar_t windows[MAX_PATH];

    (void) CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    (void) GetTempPathW(MAX_PATH, tmp);
    (void) GetSystemWindowsDirectoryW(windows, MAX_PATH);
    const wchar_t      system = static_cast<wchar_t>(towupper(windows[0]));
    const std::wstring root   = std::wstring(1, system) + L":\\";
    const std::wstring dir    = std::wstring(tmp) + L"RetroRun-shortcuts-test-" + std::to_wstring(GetCurrentProcessId());
    const std::wstring shell  = JoinPath(windows, L"System32\\shell32.dll") + L",12";
    const std::wstring res    = JoinPath(windows, L"System32\\imageres.dll") + L",-28";
    const std::wstring colon(1, kColon);
    (void) CreateDirectoryW(dir.c_str(), nullptr);

    /* The drive a shortcut links to. */
    Check(MakeShortcut(JoinPath(dir, L"Drive.lnk"), root, false), L"a shortcut made to a drive");
    Check(ShortcutDrive(JoinPath(dir, L"Drive.lnk")) == system, L"a shortcut to a drive's root");
    Check(MakeShortcut(JoinPath(dir, L"Dragged.LNK"), root, true), L"a shortcut made to a drive's item");
    Check(ShortcutDrive(JoinPath(dir, L"Dragged.LNK")) == system, L"a shortcut to a drive's item, as from This PC");
    Check(MakeShortcut(JoinPath(dir, L"Windows.lnk"), windows, false), L"a shortcut made to a folder");
    Check(ShortcutDrive(JoinPath(dir, L"Windows.lnk")) == 0, L"a shortcut to a folder of a drive");
    Check(WriteBytes(JoinPath(dir, L"Not one.lnk"), "not a shortcut", 14), L"a file that is no shortcut");
    Check(ShortcutDrive(JoinPath(dir, L"Not one.lnk")) == 0, L"a file named as a shortcut that is none");
    Check(ShortcutDrive(JoinPath(dir, L"None.lnk")) == 0, L"a shortcut that is not there");

    /* The shortcuts of a folder. */
    Check(WriteBytes(JoinPath(dir, L"notes.txt"), "", 0), L"a file of another kind");
    Check(WriteBytes(JoinPath(dir, L"Longer.lnkx"), "", 0), L"a file of a longer extension");
    Check(CreateDirectoryW(JoinPath(dir, L"Folder.lnk").c_str(), nullptr) != 0, L"a folder named as a shortcut");
    {
        std::vector<std::wstring> files = ShortcutFiles(dir);
        std::wstring              list;

        std::sort(files.begin(), files.end(),
                  [](const std::wstring &a, const std::wstring &b) { return _wcsicmp(a.c_str(), b.c_str()) < 0; });
        for (const auto &f : files)
            list += f + L"|";
        Check(list == L"Dragged.LNK|Drive.lnk|Not one.lnk|Windows.lnk|", L"the shortcuts of a folder", list);
    }

    /* Shortcuts' names for the names Explorer gives drives. */
    Check(ShortcutName(L"TESTDISK (A:)", L'A', true) == L"TESTDISK (A" + colon + L")", L"a name with its drive's letter",
          ShortcutName(L"TESTDISK (A:)", L'A', true));
    Check(ShortcutName(L"TESTDISK (A:)", L'A', false) == L"TESTDISK", L"a name without its drive's letter",
          ShortcutName(L"TESTDISK (A:)", L'A', false));
    Check(ShortcutName(L"DVD Drive (G:) DISC", L'G', true) == L"DVD Drive (G" + colon + L") DISC",
          L"a disc drive's name, with its letter", ShortcutName(L"DVD Drive (G:) DISC", L'G', true));
    Check(ShortcutName(L"DVD Drive (G:) DISC", L'G', false) == L"DVD Drive DISC", L"a disc drive's name, without it",
          ShortcutName(L"DVD Drive (G:) DISC", L'G', false));
    Check(ShortcutName(L"(A:) Floppy Disk Drive", L'A', false) == L"Floppy Disk Drive",
          L"a name whose letter comes first", ShortcutName(L"(A:) Floppy Disk Drive", L'A', false));
    Check(ShortcutName(L"Floppy Disk Drive", L'A', false) == L"Floppy Disk Drive", L"a name Explorer shows no letter in",
          ShortcutName(L"Floppy Disk Drive", L'A', false));
    Check(ShortcutName(L"A/B:C? (A:)", L'A', true) == L"A_B" + colon + L"C_ (A" + colon + L")",
          L"characters no file's name holds", ShortcutName(L"A/B:C? (A:)", L'A', true));
    Check(ShortcutName(L"Disk.. (A:)", L'A', false) == L"Disk", L"dots at the end", ShortcutName(L"Disk.. (A:)", L'A', false));
    Check(ShortcutName(L"CON (A:)", L'A', false) == L"CON_", L"a device's name", ShortcutName(L"CON (A:)", L'A', false));
    Check(ShortcutName(L"nul.txt (A:)", L'A', false) == L"nul_.txt", L"a device's name with an extension",
          ShortcutName(L"nul.txt (A:)", L'A', false));
    Check(ShortcutName(L"(A:)", L'A', false) == L"(A" + colon + L")", L"a name of the letter alone",
          ShortcutName(L"(A:)", L'A', false));

    /* Names free in a folder. */
    Check(WriteBytes(JoinPath(dir, L"Taken.lnk"), "", 0) && WriteBytes(JoinPath(dir, L"Taken (2).lnk"), "", 0),
          L"files with names");
    Check(FreeFileName(dir, L"Free.lnk", L"Drive.lnk") == L"Free.lnk", L"a free name",
          FreeFileName(dir, L"Free.lnk", L"Drive.lnk"));
    Check(FreeFileName(dir, L"Taken.lnk", L"Drive.lnk") == L"Taken (3).lnk", L"a name that files have",
          FreeFileName(dir, L"Taken.lnk", L"Drive.lnk"));
    Check(FreeFileName(dir, L"TAKEN.lnk", L"Taken.lnk") == L"TAKEN.lnk", L"the file's own name in another case",
          FreeFileName(dir, L"TAKEN.lnk", L"Taken.lnk"));
    Check(FreeFileName(dir, L"Taken (2).lnk", L"Taken (2).lnk") == L"Taken (2).lnk", L"the file's own name",
          FreeFileName(dir, L"Taken (2).lnk", L"Taken (2).lnk"));

    /* A shortcut's own icon, and the file's identity. */
    {
        const std::wstring link  = JoinPath(dir, L"Drive.lnk");
        const std::wstring moved = JoinPath(dir, L"Moved \xa789.lnk");
        const std::wstring id    = FileIdentity(link);

        Check(ShortcutIcon(link).empty(), L"a shortcut without an icon of its own", ShortcutIcon(link));
        Check(SetShortcutIcon(link, shell) && (ShortcutIcon(link) == shell), L"an icon given", ShortcutIcon(link));
        Check(SetShortcutIcon(link, res) && (ShortcutIcon(link) == res), L"an icon by its resource's number",
              ShortcutIcon(link));
        Check(SetShortcutIcon(link, L"") && ShortcutIcon(link).empty(), L"the icon taken away", ShortcutIcon(link));
        Check(ShortcutDrive(link) == system, L"the shortcut links to its drive still");
        Check(!id.empty() && (FileIdentity(link) == id), L"a shortcut saved again is the same file", FileIdentity(link));
        Check(MoveFileW(link.c_str(), moved.c_str()) && (FileIdentity(moved) == id), L"a shortcut renamed is the same file",
              FileIdentity(moved));
        Check(FileIdentity(JoinPath(dir, L"Dragged.LNK")) != id, L"another file is another");
        Check(FileIdentity(JoinPath(dir, L"None.lnk")).empty(), L"no file has no identity");
        (void) MoveFileW(moved.c_str(), link.c_str());
    }

    for (const wchar_t *f : { L"Drive.lnk", L"Dragged.LNK", L"Windows.lnk", L"Not one.lnk", L"notes.txt", L"Longer.lnkx",
                              L"Taken.lnk", L"Taken (2).lnk" })
        (void) DeleteFileW(JoinPath(dir, f).c_str());
    (void) RemoveDirectoryW(JoinPath(dir, L"Folder.lnk").c_str());
    (void) RemoveDirectoryW(dir.c_str());
    CoUninitialize();

    wprintf(L"%d of %d checks passed\n", g_checks - g_failures, g_checks);
    return (g_failures == 0) ? 0 : 1;
}
