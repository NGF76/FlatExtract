# FlatExtract v1.3.1

FlatExtract is a simple and powerful desktop application written in C++ using the Qt framework. It is designed to easily and quickly extract files from compressed archives and game disc images. The application is aimed at Linux users who are looking for a lightweight and user-friendly graphical tool for managing compressed files.

---

## What's New in v1.3.1

FlatExtract v1.3.1 is a UI fix release for the file picker.

### Fixed

- File and folder names are now readable in the open-file dialog when the dark theme is active.
- File dialogs consistently follow the selected theme, including list, icon, and detail views, column headers, path and file-type combo boxes, and navigation buttons.
- The light theme now styles file dialogs consistently as well.

### Packaging

- Updated AppImage and `.deb` packages to version 1.3.1.

## What's New in v1.3.0

FlatExtract v1.3.0 adds native extraction support for PlayStation 2, PlayStation 3, and GameCube disc images, alongside the existing ZIP, TAR, and Xbox support.

### PlayStation 2 (ISO 9660)

- Extract PS2 `.iso` images directly, including nested folders, empty files, and large game assets.
- Files are streamed to disk instead of being loaded into memory all at once.
- Unsafe paths are rejected during listing and extraction.

### PlayStation 3 (UDF 2.50)

- Extract PS3 `.iso` images using the `libudfread` UDF reader.
- Automatic fallback to ISO 9660 for hybrid UDF bridge discs when the UDF layout cannot be read.
- Per-file progress reporting while large game files are extracted.

### GameCube (ISO / GCM)

- Extract GameCube `.iso` and `.gcm` images using a built-in FST reader.
- Supports nested folders and Shift-JIS file names.
- Validates file bounds and rejects unsafe paths.

### Reliability and UI

- Refreshed interface with dark and light themes.
- Display archive file counts, original sizes, and total extracted size.
- Show cleaner file names in the file list.
- Improved status messages and per-file extraction progress.

### Packaging

- The AppImage bundles Qt, libarchive, and `libudfread` so PS3 images work out of the box.
- The `.deb` package installs the application, desktop entry, and 512×512 icon, and declares `libudfread0` as a dependency.

### Previous release highlights (v1.2.0)

- Native TAR extraction with libarchive, including permissions, timestamps, symlinks, and hardlinks.
- Original Xbox (OG Xbox) ISO extraction with XBE metadata support.
- Xbox 360 ISO and XEX support.

> **Compatibility note:** The bundled 7-Zip tool has been removed. 7z and RAR archives are no longer supported. Supported archive formats are ZIP and the TAR family listed below.

## Supported Formats

| Type | Formats |
| --- | --- |
| Archives | ZIP, TAR, TGZ, TAR.GZ, TBZ, TAR.BZ, TBZ2, TAR.BZ2, TAR.XZ |
| Disc images | PlayStation 2 ISO (ISO 9660), PlayStation 3 ISO (UDF 2.50), GameCube ISO/GCM, Xbox 360 ISO, OG Xbox ISO |

> **PlayStation 3 note:** UDF extraction uses `libudfread`. The AppImage bundles it, and the `.deb` package installs `libudfread0` automatically. When building from source, install it with `sudo apt install libudfread0`.


---

Developer By NGF76

Developed by **NGF76**. This application is completely free and open source. See the `LICENSE` file for more details.

GitHub: https://github.com/NGF76


---
## Installation and Usage (AppImage)

### Running the Application Directly (AppImage)

The AppImage is portable and includes the required Qt and runtime libraries.

1. Download the latest `FlatExtract-x86_64.AppImage` from the [Releases page](https://github.com/NGF76/FlatExtract/releases).

2. Make it executable and run it:
   ```bash
   chmod +x FlatExtract-x86_64.AppImage
   ./FlatExtract-x86_64.AppImage
   ```

## Installation and Usage (.DEB)

### Or: DEB Package (Recommended for Debian/Ubuntu/Linux Mint)

1. Download the latest `FlatExtract-1.3.1-Linux.deb` from the [Releases page](https://github.com/NGF76/FlatExtract/releases).

2. Install it using:
   ```bash
   sudo dpkg -i FlatExtract-1.3.1-Linux.deb
   sudo apt install -f
   ```

----

## Disclaimer

This software is provided "AS IS", without warranty of any kind, express or implied, including but not limited to the warranties of merchantability, fitness for a particular purpose, and noninfringement.

In no event shall the developer be liable for any claim, damages, or other liability, whether in an action of contract, tort, or otherwise, arising from, out of, or in connection with the software or the use or other dealings in the software.

The user is responsible for ensuring that their use of archives and files complies with all applicable local and international laws.



This application uses the QuaZIP library, licensed under GNU LGPL.
This application uses the libarchive library, licensed under the BSD 2-Clause License.
This application uses the libcdio / libiso9660 library, licensed under GNU GPL / LGPL.
This application uses the Qt framework, licensed under GNU LGPL.

The user is responsible for ensuring that their use of archives and files complies with all applicable local and international laws.

---

IMPORTANT NOTE !!!!!!!

## Libraries Used

This product includes software developed by the following open-source projects:

1. Qt Framework
   - License: GNU Lesser General Public License (LGPL) v2.1 or v3
   - Source: https://www.qt.io

2. QuaZIP / QuaZIP5
   - License: GNU Lesser General Public License (LGPL) v2.1+
   - Source: https://quazip.sourceforge.net

3. zlib (ZIP compression library)
   - License: zlib/libpng License
   - Source: https://zlib.net

4. libcdio
   - License: GNU General Public License (GPL) v2+ or LGPL v2.1+
   - Source: https://www.gnu.org/software/libcdio

5. libcdio++ (CDIOPP)
   - License: GNU General Public License (GPL) v2+ or LGPL v2.1+
   - Source: https://www.gnu.org/software/libcdio

6. libiso9660
   - License: GNU General Public License (GPL) v2+ or LGPL v2.1+
   - Source: https://www.gnu.org/software/libcdio (part of libcdio project)

7. Radare2 (R2)
   - License: GNU Lesser General Public License (LGPL) v3
   - Source: https://www.radare.org

8. libarchive
   - License: BSD 2-Clause License
   - Source: https://www.libarchive.org

9. libudfread
   - License: GNU Lesser General Public License (LGPL) v2.1+
   - Source: https://www.videolan.org/developers/libudfread.html
---

### License Compliance

FlatExtract is licensed under the **MIT License**.  
However, it uses libraries under other open-source licenses (LGPL, GPL).  
These libraries are dynamically linked and can be replaced by the user, in compliance with their respective licenses.

For more details, see the `LICENSE` file and the individual library licenses.
