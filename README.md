# FlatExtract v1.2.0

FlatExtract is a simple and powerful desktop application written in C++ using the Qt framework. It is designed to easily and quickly extract files from compressed archives and ISO images. The application is aimed at Linux users who are looking for a lightweight and user-friendly graphical tool for managing compressed files.

---

## What's New in v1.2.0

FlatExtract v1.2.0 adds native TAR extraction, improves Xbox ISO handling, and provides clearer extraction feedback.

### Native TAR Support

- Extract `.tar`, `.tgz`, `.tar.gz`, `.tbz`, `.tar.bz`, `.tbz2`, `.tar.bz2`, and `.tar.xz` archives directly with libarchive.
- No external `7z`, `tar`, or decompression binary is required.
- Preserve file permissions, timestamps, symlinks, and hardlinks.
- Reject unsafe paths and symlink escapes during extraction.

### ISO Improvements

- Detect and extract original Xbox (OG Xbox) ISO images, including XBE metadata.
- Xbox 360 ISO and XEX support remains available.
- Show per-file extraction progress for ZIP, TAR, and ISO operations.

### Reliability and UI

- Display archive file counts, original sizes, and total extracted size.
- Show cleaner file names in the file list.
- Improved status messages and extraction progress handling.
- Refreshed application icon.

### Packaging

- The AppImage release bundles Qt and required runtime libraries.
- The `.deb` package installs the application, desktop entry, and 512×512 icon.

> **Compatibility note:** The bundled 7-Zip tool has been removed. 7z and RAR archives are no longer supported. Supported archive formats are ZIP and the TAR family listed above.

## Supported Formats

| Type | Formats |
| --- | --- |
| Archives | ZIP, TAR, TGZ, TAR.GZ, TBZ, TAR.BZ, TBZ2, TAR.BZ2, TAR.XZ |
| Disc images | Xbox 360 ISO, OG Xbox ISO |


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

1. Download the latest `FlatExtract-1.2.0-Linux.deb` from the [Releases page](https://github.com/NGF76/FlatExtract/releases).

2. Install it using:
   ```bash
   sudo dpkg -i FlatExtract-1.2.0-Linux.deb
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
---

### License Compliance

FlatExtract is licensed under the **MIT License**.  
However, it uses libraries under other open-source licenses (LGPL, GPL).  
These libraries are dynamically linked and can be replaced by the user, in compliance with their respective licenses.

For more details, see the `LICENSE` file and the individual library licenses.
