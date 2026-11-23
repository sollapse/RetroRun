#pragma once
#include <windows.h>
#include <string>
#include <vector>

/* Icons in programs and icon libraries. */

/* A 32- or 64-bit program's or library's icon, as the bytes of an .ico file in ico: the index'th of its icons, as
   ExtractIconEx counts them, or, for a negative index, the one whose resource ID is -index. The file is loaded as
   data, so none of its code runs. Loaded so from a floppy, a program is read from the disk again rather than from
   the file cache, which Windows' malware scan fills as the program is opened, so a local copy reads faster. Returns
   false for a file that cannot be loaded so, as a 16-bit program, and for one without that icon. */
bool ProgramIcon(const std::wstring &path, int index, std::vector<char> *ico);
