#pragma once
#include <windows.h>
#include <string>
#include <vector>

/* Shortcuts to drives, and the names and icons that the program gives them. Explorer shows a shortcut under its
   file's name, and takes in a rename at once; it reads a folder's desktop.ini, whose [LocalizedFileNames] could name
   a file otherwise, only once while it runs. */

/* The folder's path with the file's name. */
std::wstring JoinPath(const std::wstring &folder, const std::wstring &file);

/* The letter of the drive whose root the shortcut links to, as "A:\" names it, or 0 for a shortcut to anything
   else and for a file that is no shortcut. The shortcut is read, not resolved: nothing it links to is touched.
   COM must be initialized on the thread. */
wchar_t ShortcutDrive(const std::wstring &path);

/* The shortcuts in the folder itself, not in its subfolders, by their files' names. */
std::vector<std::wstring> ShortcutFiles(const std::wstring &folder);

/* The character that stands for a colon in a file's name, which cannot hold one: U+A789, MODIFIER LETTER COLON. */
constexpr wchar_t kColon = L'\xa789';

/* A shortcut's name, without its extension, for a drive that Explorer names name, as "TESTDISK (A:)" or
   "DVD Drive (G:) DISC": with the drive's letter, whose colon becomes kColon, or without it. Characters that no
   file's name can hold become '_', and a name that Windows keeps for a device, as CON, gets one after it. */
std::wstring ShortcutName(const std::wstring &name, wchar_t letter, bool letters);

/* The name for a file in the folder: file, or file with " (2)", " (3)" and so on before its extension if another
   file has that name. current is the file's own name, which it may keep. */
std::wstring FreeFileName(const std::wstring &folder, const std::wstring &file, const std::wstring &current);

/* The shortcut's own icon as "file,index", or nothing if it shows its target's. COM must be initialized. */
std::wstring ShortcutIcon(const std::wstring &path);

/* Gives the shortcut the icon, "file,index", or none, so that it shows its target's again, and saves it where it is,
   which keeps the file the same. Returns whether it was saved. COM must be initialized. */
bool SetShortcutIcon(const std::wstring &path, const std::wstring &icon);

/* What tells the file from any other however it is renamed or saved again: its volume's serial number and its
   index there, or nothing if it cannot be opened. */
std::wstring FileIdentity(const std::wstring &path);
