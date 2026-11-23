#include "shortcuts.h"
#include <objbase.h>
#include <shlobj.h>
#include <shobjidl.h>
#include <shlwapi.h>
#include <wchar.h>
#include <wctype.h>

namespace {

bool
IsLetter(const wchar_t c)
{
    return ((c >= L'A') && (c <= L'Z')) || ((c >= L'a') && (c <= L'z'));
}

/* A name that Windows keeps for a device, whatever follows its first dot: CON, PRN, AUX, NUL, COM0 to COM9 and
   LPT0 to LPT9. */
bool
IsDeviceName(const std::wstring &name)
{
    std::wstring s = name.substr(0, name.find(L'.'));

    while (!s.empty() && (s.back() == L' '))
        s.pop_back();
    for (auto &c : s)
        c = static_cast<wchar_t>(towupper(c));
    if ((s == L"CON") || (s == L"PRN") || (s == L"AUX") || (s == L"NUL"))
        return true;
    return (s.size() == 4) && ((s.compare(0, 3, L"COM") == 0) || (s.compare(0, 3, L"LPT") == 0)) && (s[3] >= L'0') &&
           (s[3] <= L'9');
}

/* The shortcut, read from its file, with the file to save it through, or nothing if the file is no shortcut. */
IShellLinkW *
OpenShortcut(const std::wstring &path, const DWORD mode, IPersistFile **file)
{
    IShellLinkW *link = nullptr;

    *file = nullptr;
    if (FAILED(CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&link))))
        return nullptr;
    if (SUCCEEDED(link->QueryInterface(IID_PPV_ARGS(file))) && SUCCEEDED((*file)->Load(path.c_str(), mode)))
        return link;
    if (*file != nullptr) {
        (*file)->Release();
        *file = nullptr;
    }
    link->Release();
    return nullptr;
}

} /* namespace */

std::wstring
JoinPath(const std::wstring &folder, const std::wstring &file)
{
    return (!folder.empty() && (folder.back() == L'\\')) ? (folder + file) : (folder + L"\\" + file);
}

wchar_t
ShortcutDrive(const std::wstring &path)
{
    IShellLinkW  *link             = nullptr;
    IPersistFile *file             = nullptr;
    wchar_t       target[MAX_PATH] = {};
    HRESULT       hr               = E_FAIL;

    if (FAILED(CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&link))))
        return 0;
    if (SUCCEEDED(link->QueryInterface(IID_PPV_ARGS(&file)))) {
        if (SUCCEEDED(file->Load(path.c_str(), STGM_READ)))
            hr = link->GetPath(target, MAX_PATH, nullptr, 0);
        file->Release();
    }
    link->Release();
    /* "A:\", or "A:". */
    if ((hr != S_OK) || !IsLetter(target[0]) || (target[1] != L':') ||
        !((target[2] == L'\0') || ((target[2] == L'\\') && (target[3] == L'\0'))))
        return 0;
    return static_cast<wchar_t>(towupper(target[0]));
}

std::vector<std::wstring>
ShortcutFiles(const std::wstring &folder)
{
    std::vector<std::wstring> files;
    WIN32_FIND_DATAW          f;

    /* A pattern of "*.lnk" also finds a longer extension through its short name, so the name is checked. */
    const HANDLE h = FindFirstFileExW(JoinPath(folder, L"*.lnk").c_str(), FindExInfoBasic, &f, FindExSearchNameMatch,
                                      nullptr, 0);
    if (h == INVALID_HANDLE_VALUE)
        return files;
    do {
        if (!(f.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && (_wcsicmp(PathFindExtensionW(f.cFileName), L".lnk") == 0))
            files.push_back(f.cFileName);
    } while (FindNextFileW(h, &f));
    FindClose(h);
    return files;
}

std::wstring
ShortcutName(const std::wstring &name, const wchar_t letter, const bool letters)
{
    std::wstring s = name;
    std::wstring out;

    if (!letters) {
        const std::wstring tag = std::wstring(L"(") + letter + L":)";
        const size_t       at  = s.find(tag);

        /* The letter goes with one of the spaces around it. */
        if (at != std::wstring::npos) {
            size_t from = at;
            size_t to   = at + tag.size();

            if ((from > 0) && (s[from - 1] == L' '))
                from--;
            else if ((to < s.size()) && (s[to] == L' '))
                to++;
            s.erase(from, to - from);
        }
    }
    for (const wchar_t c : s)
        if (c == L':')
            out += kColon;
        else if ((c < 0x20) || (wcschr(L"\\/*?\"<>|", c) != nullptr))
            out += L'_';
        else
            out += c;
    /* Windows drops the spaces and dots at the end of a file's name. */
    while (!out.empty() && ((out.back() == L' ') || (out.back() == L'.')))
        out.pop_back();
    const size_t first = out.find_first_not_of(L' ');
    out                = (first == std::wstring::npos) ? std::wstring() : out.substr(first);
    if (out.empty())
        return letters ? std::wstring(1, letter) : ShortcutName(name, letter, true);
    if (IsDeviceName(out)) {
        const size_t dot = out.find(L'.');

        out.insert((dot == std::wstring::npos) ? out.size() : dot, 1, L'_');
    }
    return out;
}

std::wstring
FreeFileName(const std::wstring &folder, const std::wstring &file, const std::wstring &current)
{
    const wchar_t     *dot  = PathFindExtensionW(file.c_str());
    const std::wstring stem = file.substr(0, static_cast<size_t>(dot - file.c_str()));
    const std::wstring ext  = dot;

    for (int n = 1; n < 1000; n++) {
        const std::wstring name = (n == 1) ? file : (stem + L" (" + std::to_wstring(n) + L")" + ext);

        if ((_wcsicmp(name.c_str(), current.c_str()) == 0) ||
            (GetFileAttributesW(JoinPath(folder, name).c_str()) == INVALID_FILE_ATTRIBUTES))
            return name;
    }
    return current;
}

std::wstring
ShortcutIcon(const std::wstring &path)
{
    IPersistFile *file             = nullptr;
    IShellLinkW  *link             = OpenShortcut(path, STGM_READ, &file);
    wchar_t       where[MAX_PATH] = {};
    int           index            = 0;
    std::wstring  icon;

    if (link == nullptr)
        return icon;
    if (SUCCEEDED(link->GetIconLocation(where, MAX_PATH, &index)) && (where[0] != L'\0'))
        icon = std::wstring(where) + L"," + std::to_wstring(index);
    file->Release();
    link->Release();
    return icon;
}

bool
SetShortcutIcon(const std::wstring &path, const std::wstring &icon)
{
    IPersistFile *file  = nullptr;
    IShellLinkW  *link  = OpenShortcut(path, STGM_READWRITE, &file);
    const size_t  comma = icon.rfind(L',');
    std::wstring  where = icon;
    int           index = 0;

    if (link == nullptr)
        return false;
    if (comma != std::wstring::npos) {
        where = icon.substr(0, comma);
        index = static_cast<int>(wcstol(icon.c_str() + comma + 1, nullptr, 10));
    }
    /* No file clears the icon, which the file then has no more of. */
    const bool ok = SUCCEEDED(link->SetIconLocation(where.c_str(), index)) && SUCCEEDED(file->Save(nullptr, TRUE));
    file->Release();
    link->Release();
    return ok;
}

std::wstring
FileIdentity(const std::wstring &path)
{
    static const wchar_t       digits[] = L"0123456789abcdef";
    BY_HANDLE_FILE_INFORMATION info;
    std::wstring               id;

    const HANDLE h = CreateFileW(path.c_str(), 0, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
                                 OPEN_EXISTING, 0, nullptr);
    if (h == INVALID_HANDLE_VALUE)
        return id;
    const bool ok = GetFileInformationByHandle(h, &info) != 0;
    CloseHandle(h);
    if (!ok)
        return id;
    for (const DWORD v : { info.dwVolumeSerialNumber, info.nFileIndexHigh, info.nFileIndexLow }) {
        for (int shift = 28; shift >= 0; shift -= 4)
            id += digits[(v >> shift) & 15];
        id += L'-';
    }
    id.pop_back();
    return id;
}
