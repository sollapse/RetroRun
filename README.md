# RetroRun

RetroRun is a small Windows tray program for removable disks: floppy and SuperDisk disks, USB drives, memory
cards, CDs and DVDs, and external hard drives. For the kinds of drives you choose, it:

- **shows disks' own names and icons in File Explorer.** Explorer shows every floppy drive as "Floppy Disk Drive"
  with the stock icon, whatever disk is in it. RetroRun reads the disk and shows its autorun.inf label or volume
  label, and its autorun.inf icon or the first icon file in its root.
- **starts autorun.inf programs** as a disk goes in, as AutoRun did before Windows limited it to CDs and DVDs.
- **turns off Windows' AutoPlay** while it runs, so that AutoPlay and RetroRun don't both act on a disc.
- **renames shortcuts to drives after the drives**, if you ask it to: a shortcut on the desktop to a drive takes
  the drive's name as File Explorer shows it, with the disk's name in it, and the drive's icon, in place of its own
  name, such as "Floppy Disk Drive (A) - Shortcut".

It runs as the current user, needs no administrator rights, and needs Windows 10 or 11.

## The tray menu

| Item | What it does |
| --- | --- |
| One line per drive | The drives RetroRun follows, and what it found in each |
| Read the disks now | Reads every followed drive again |
| Display names and icons (submenu) | The kinds of drives whose disks' names and icons are shown; floppy and SuperDisk drives at first |
| Display autorun.inf icon | Shows the icon that the disk's autorun.inf names |
| Display autorun.inf label | Shows the label that the disk's autorun.inf names |
| Display first icon on disk (no autorun.inf) | Shows the first `.ico` file in the disk's root when no autorun.inf icon is shown |
| Display volume label (no autorun.inf label) | Shows the volume label when no autorun.inf label is shown |
| Cache icons per volume | Keeps each disk's icons between reads and runs, so that a disk that comes back unchanged shows its icon at once; off at first |
| Clear the icon cache | Deletes the icons kept, but those the drives show now |
| Update shortcuts to drives | Renames shortcuts to drives on the desktop after their drives, as Explorer names the drives, and gives them the drives' icons; off at first |
| Drive letters in shortcut names | Names the shortcuts with their drives' letters, as "TESTDISK (A꞉)", or, unticked, without them, as "TESTDISK" |
| Launch autorun.inf applications (submenu) | The kinds of drives whose autorun.inf programs are started |
| Disable native AutoPlay handling (submenu) | The kinds of drives that Windows' AutoPlay is kept from acting on |
| Look for disks (submenu) | The kinds of drives that are asked every 2 seconds whether a disk is in; all at first |
| Start with Windows | Starts RetroRun when you sign in |
| Exit | Ends RetroRun, which gives the drives their usual names back |

Each submenu lists the kinds of drives, with the letters of those present: floppy disk drives, SuperDisk drives,
USB drives, memory cards, other removable disks, CD and DVD drives, and external hard drives. Its "All removable
disks" item ticks them all, or unticks them all when all are ticked.

## Names and icons

RetroRun gives Explorer what it found through the drive letter's per-user key,
`HKCU\Software\Classes\Applications\Explorer.exe\Drives\<letter>` (`DefaultLabel` and `DefaultIcon`). It marks
the keys it writes with a `RetroRun` value and never changes a key without that mark. Icons are copied from the
disks to `%LOCALAPPDATA%\RetroRun\Icons`. An icon in a program or library (`.exe`, `.dll`, `.icl`, `.cpl`, `.ocx`)
is kept as an `.ico` file taken out of the program's copy, which is then deleted, so that no program stays there; a
16-bit program, whose icon cannot be taken out so, stays whole.

The icons are deleted as RetroRun starts, unless **Cache icons per volume** is ticked. Then RetroRun keeps each
disk's icons, for drives of every kind, and remembers under `HKCU\Software\RetroRun\IconCache` what each came from:
the volume's serial number and label, its autorun.inf, and the icon file's size and date, which the disk's
directory tells without the file being read. A disk that comes back with all of these unchanged gets its icon again
at once, without its icon file being read. That matters most for an icon in a program on a floppy: Windows' malware
scan reads the whole program as it is opened, which takes seconds. **Clear the icon cache** deletes what is kept,
but the icons the drives show now.

Explorer reads the disks in drives other than floppy drives itself, and what it finds comes first: the volume
label before `DefaultLabel`, and a disc's autorun.inf icon and label before both. On those drives RetroRun fills
only what Explorer leaves out, such as the first icon file of a disc whose autorun.inf names no icon.

## Launching

When a disk goes into a drive of a kind ticked under **Launch autorun.inf applications**, RetroRun reads its
autorun.inf as Windows does: the `[autorun.amd64]` section on 64-bit Windows if the file has one, and `[autorun]`
otherwise. It starts the `shellexecute=` entry, which may name a document, or else the `open=` entry, from the
disk's root. `open=start file`, the form that Windows 95's START program ran, opens the file.

Nothing is started:

- when the file sets `UseAutoPlay=1`;
- when Shift is held as the disk goes in;
- when the window in front, or a program registered with AutoPlay, cancels AutoRun;
- when Windows' AutoPlay is set to start a disc's program itself, unless native AutoPlay handling is disabled
  for the drive;
- for a disk that is already in when RetroRun starts or when its kind is ticked. A floppy controller's drive tells
  of no disk until Windows mounts it, so a disk there is started when it is first read.

## Disabling native AutoPlay handling

While RetroRun runs, it registers with AutoPlay to be asked first: an `IQueryCancelAutoPlay` object in the running
object table, whose class it lists under
`HKCU\Software\Microsoft\Windows\CurrentVersion\Explorer\AutoplayHandlers\CancelAutoplay\CLSID`. For drives of
the kinds ticked under **Disable native AutoPlay handling**, it tells AutoPlay not to act, so that AutoPlay
neither asks what to do nor does anything, and doesn't start a disc's program either. The listing is removed when
RetroRun exits. Explorer still reads discs itself, for their autorun.inf icon and label, and no per-user setting
changes that.

## Shortcuts to drives

With **Update shortcuts to drives** ticked, each shortcut on the desktop to the root of a drive whose disks' names
and icons are shown is renamed after its drive, as File Explorer names the drive, and given the drive's icon:
"Floppy Disk Drive (A꞉)", say, and "TESTDISK (A꞉)" while a disk labelled TESTDISK is in. Its name and icon follow
the disks as they go in and out.

- A file's name cannot hold a colon, so the drive's letter is followed by `꞉` (U+A789), which looks like one. Untick
  **Drive letters in shortcut names** for names without the letter, such as "TESTDISK". A program that reads file
  names only in the ANSI code page, as Windows Script Host does, cannot open a shortcut whose name holds `꞉`.
- The drive's icon becomes the shortcut's own, so that Explorer takes it from the shortcut rather than from the
  drive.
- Each shortcut gets its own name and icon back when the item is unticked, when its drive goes, when RetroRun
  exits, and as RetroRun starts after it ended without that. If another file has taken the shortcut's name
  meanwhile, it gets "(2)" after it, as Windows does.
- A shortcut that someone renames while the item is ticked is renamed after its drive again, and still gets its
  own name from before back.
- Shortcuts on the desktop of all users, in the Public Desktop folder, are left alone: renaming them would need
  administrator rights.

RetroRun keeps each shortcut's own name and icon under `HKCU\Software\RetroRun\ShortcutNames`, by the identity of
the shortcut's file, which holds however the file is renamed. It renames the shortcuts because Explorer shows a
rename at once, while the names that a folder's `desktop.ini` can give files it reads only once as long as it runs.

## How disks are noticed

Drives are recognized by their devices, whatever their letters; SuperDisk drives by their model (LS-120 or
LS-240). How a disk is noticed depends on the drive:

- A drive that describes itself, as SuperDisk, CD and other ATAPI and USB drives do, can tell from its own state
  whether a disk is in. While **Look for disks** is ticked for its kind, it is asked every 2 seconds, until it
  reports its disks by itself.
- A floppy controller's drive can tell only by running its motor, so its disk is read when Windows mounts it, as
  when you open the drive, or when you choose **Read the disks now**.
- An external hard drive's disk never changes, so there is nothing to look for.
- Drives other than floppy drives also tell of their disks through Windows' device broadcasts.

A drive whose volume another program has locked is left alone until that program lets it go. That includes a
program that had the drive before RetroRun did, as an emulator does that takes a drive as soon as it appears:
RetroRun is not told of that lock, but the volume refuses to be read, and its tray menu shows the drive as in use
by another program until it is let go.

## Settings and files

The settings are DWORD values under `HKCU\Software\RetroRun`, which the menu writes. `Display`, `Launch`,
`Native` and `Look` hold the ticked kinds of drives as bits: 1 floppy disk drives, 2 SuperDisk drives, 4 USB
drives, 8 memory cards, 16 other removable disks, 32 CD and DVD drives, 64 external hard drives. `Shortcuts` is 1
while shortcuts to drives are updated, and `ShortcutLetters` 0 while their names leave the drives' letters out.
`CacheIcons` is 1 while icons are kept per volume. Three settings have no menu item: `CheckMs`, how often drives are asked
whether a disk is in (2000 milliseconds, at least 500); `RefreshAll` (0 or 1), which has Explorer redraw every icon
instead of only the drive's; and `ShortcutFolders`, a list of folders (`REG_MULTI_SZ`) whose shortcuts to drives are
updated instead of the desktop's. RetroRun reads them as it starts.

RetroRun's log is `%LOCALAPPDATA%\RetroRun\RetroRun.log`, started afresh when it grows past 512 KiB.

## Building

RetroRun builds with CMake 3.21 or later. `CMakePresets.json` has a preset for each toolchain, which takes its
compiler from `PATH`:

| Preset | Toolchain |
| --- | --- |
| `vs2022` | Visual Studio 2022 or its Build Tools: MSVC, x64. CMake finds Visual Studio itself. |
| `mingw` | MinGW-w64 GCC (`g++` and `windres`), such as MSYS2's `mingw64`, `ucrt64` or 32-bit `mingw32` |
| `clang` | `clang++`: LLVM for Windows, which uses Visual Studio's libraries, or a MinGW-w64 Clang |
| `clang-cl` | `clang-cl`, from a Visual Studio developer prompt |

The `mingw`, `clang` and `clang-cl` presets use Ninja. To build and test with one:

```
cmake --preset vs2022
cmake --build --preset vs2022
ctest --preset vs2022
```

The program is `build/<preset>/Release/RetroRun.exe`. Add `--config Debug` to the build command for a debug
build, and configure with `cmake --preset vs2022 -A Win32` for 32-bit x86. Visual Studio can also open the folder
and build from the presets.

The tests check how autorun.inf files are read, how their commands are split into a program and its parameters,
how drives are told apart by model, and how shortcuts to drives are found, named and given icons, in a temporary
folder. They start nothing.

## Installing

There is no installer. Copy `RetroRun.exe` to a folder of your choice, such as `%LOCALAPPDATA%\Programs\RetroRun`,
run it, and tick **Start with Windows** in its tray menu. `RetroRun.exe /exit` asks a running copy to exit.

To remove it, untick **Start with Windows** and choose **Exit**; RetroRun takes its drive keys and its AutoPlay
listing back as it exits, and gives shortcuts their own names and icons back. Then delete the program, the key `HKCU\Software\RetroRun` and the folder
`%LOCALAPPDATA%\RetroRun`.
