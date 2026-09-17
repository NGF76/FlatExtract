# FlatExtract v1.1.0

FlatExtract is a simple and powerful desktop application written in C++ using the Qt framework. It is designed to easily and quickly extract files from compressed archives and ISO images. The application is aimed at Linux users who are looking for a lightweight and user-friendly graphical tool for managing compressed files.

---

## What's New in This Release

- Added support for extracting 7z, TAR, GZ, BZ2, XZ, and RAR archives using the bundled 7z tool.
- Bundled the 7z executable within the application, so users no longer need to install it manually.
- Display the original file size in the file list.
- Display the number of files inside the archive before extraction.
- Display the total size of extracted files after the operation.
- Improved display of file names (without full paths) for easier reading.
- Added a Support button that opens the GitHub page.
- Fixed issues with archive content parsing.
- Improved application stability during extraction.


---

Developer By NGF76

Developed by NGF76, this application is completely free and open source. Check the license file to learn more. 


---
## Installation and Usage (AppImage)

### Running the Application Directly (AppImage)

1. Download the `FlatExtract-x86_64.AppImage` file from the [Releases page](https://github.com/NGF76/FlatExtract/releases).

2. Make it executable:
   ```bash
   chmod +x FlatExtract-x86_64.AppImage

## Installation and Usage (.DEB) 

### Or: DEB Package (Recommended for Debian/Ubuntu/Linux Mint)

1. Download the `.deb Package` file from the [Releases page](https://github.com/NGF76/FlatExtract/releases).

2. Download the .deb file and install it using:
    ```bash

      sudo dpkg -i FlatExtract-1.1.0-Linux.deb
      sudo apt install -f

----

## Disclaimer

This software is provided "AS IS", without warranty of any kind, express or implied, including but not limited to the warranties of merchantability, fitness for a particular purpose, and noninfringement.

In no event shall the developer be liable for any claim, damages, or other liability, whether in an action of contract, tort, or otherwise, arising from, out of, or in connection with the software or the use or other dealings in the software.

This application uses the 7-Zip tool developed by Igor Pavlov, licensed under GNU LGPL. For more information, please visit:
https://www.7-zip.org

This application uses the QuaZIP library, licensed under GNU LGPL.
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
---

### License Compliance

FlatExtract is licensed under the **MIT License**.  
However, it uses libraries under other open-source licenses (LGPL, GPL).  
These libraries are dynamically linked and can be replaced by the user, in compliance with their respective licenses.

For more details, see the `LICENSE` file and the individual library licenses.
