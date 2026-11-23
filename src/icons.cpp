#include "icons.h"
#include <string.h>
#include <utility>

namespace {

#pragma pack(push, 2)
/* An image's entry in an icon group resource, RT_GROUP_ICON, after its 6-byte header. */
struct GroupEntry {
    BYTE  width;
    BYTE  height;
    BYTE  colors;
    BYTE  reserved;
    WORD  planes;
    WORD  bits;
    DWORD bytes;
    WORD  id; /* of its RT_ICON resource */
};
#pragma pack(pop)

/* An image's entry in an .ico file, after its 6-byte header. */
struct FileEntry {
    BYTE  width;
    BYTE  height;
    BYTE  colors;
    BYTE  reserved;
    WORD  planes;
    WORD  bits;
    DWORD bytes;
    DWORD offset; /* of its image in the file */
};

static_assert(sizeof(GroupEntry) == 14, "an icon group's entry is 14 bytes");
static_assert(sizeof(FileEntry) == 16, "an .ico file's entry is 16 bytes");

/* The icon group an enumeration looks for, by how many groups before it are left to pass, and the one found. */
struct Find {
    int          left;
    bool         found;
    WORD         id;
    std::wstring name;
};

BOOL CALLBACK
FindGroup(HMODULE, LPCWSTR, LPWSTR name, LONG_PTR param)
{
    Find *const f = reinterpret_cast<Find *>(param);

    if (f->left-- > 0)
        return TRUE;
    f->found = true;
    if (IS_INTRESOURCE(name))
        f->id = static_cast<WORD>(reinterpret_cast<ULONG_PTR>(name));
    else
        f->name = name;
    return FALSE;
}

/* A resource's bytes and size, or nullptr if the module has no such resource. */
const BYTE *
Resource(const HMODULE module, LPCWSTR name, LPCWSTR type, DWORD *size)
{
    const HRSRC r = FindResourceW(module, name, type);

    if (r == nullptr)
        return nullptr;
    const HGLOBAL g = LoadResource(module, r);
    if (g == nullptr)
        return nullptr;
    *size = SizeofResource(module, r);
    return static_cast<const BYTE *>(LockResource(g));
}

void
Put(std::vector<char> &out, const void *data, const size_t size)
{
    const char *const c = static_cast<const char *>(data);

    out.insert(out.end(), c, c + size);
}

} /* namespace */

bool
ProgramIcon(const std::wstring &path, const int index, std::vector<char> *ico)
{
    const HMODULE module = LoadLibraryExW(path.c_str(), nullptr, LOAD_LIBRARY_AS_DATAFILE | LOAD_LIBRARY_AS_IMAGE_RESOURCE);
    Find          find   = { index, false, 0, std::wstring() };
    DWORD         size   = 0;
    bool          ok     = false;

    if (module == nullptr)
        return false;
    /* ExtractIconEx counts the groups as they are enumerated: those with names first, then those with IDs. */
    if (index < 0) {
        find.found = index >= -0xffff;
        find.id    = static_cast<WORD>(-index);
    } else
        (void) EnumResourceNamesW(module, RT_GROUP_ICON, FindGroup, reinterpret_cast<LONG_PTR>(&find));
    const BYTE *const group =
        find.found ? Resource(module, find.name.empty() ? MAKEINTRESOURCEW(find.id) : find.name.c_str(), RT_GROUP_ICON,
                              &size) :
                     nullptr;
    WORD count = 0;

    if ((group != nullptr) && (size >= 6))
        memcpy(&count, group + 4, sizeof(count));
    if ((count > 0) && (size >= 6 + static_cast<DWORD>(count) * sizeof(GroupEntry))) {
        std::vector<FileEntry>                        entries;
        std::vector<std::pair<const BYTE *, DWORD>> images;

        for (WORD i = 0; i < count; i++) {
            GroupEntry g;
            DWORD      bytes = 0;

            memcpy(&g, group + 6 + i * sizeof(GroupEntry), sizeof(g));
            const BYTE *const image = Resource(module, MAKEINTRESOURCEW(g.id), RT_ICON, &bytes);
            if ((image == nullptr) || (bytes == 0))
                continue;
            entries.push_back({ g.width, g.height, g.colors, g.reserved, g.planes, g.bits, bytes, 0 });
            images.push_back({ image, bytes });
        }
        if (!entries.empty()) {
            const WORD header[3] = { 0, 1, static_cast<WORD>(entries.size()) };
            DWORD      offset    = static_cast<DWORD>(sizeof(header) + entries.size() * sizeof(FileEntry));

            for (auto &e : entries) {
                e.offset = offset;
                offset += e.bytes;
            }
            ico->clear();
            Put(*ico, header, sizeof(header));
            for (const auto &e : entries)
                Put(*ico, &e, sizeof(e));
            for (const auto &image : images)
                Put(*ico, image.first, image.second);
            ok = true;
        }
    }
    FreeLibrary(module);
    return ok;
}
