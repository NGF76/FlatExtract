# FlatExtract

FlatExtract is a simple and powerful desktop application written in C++ using the Qt framework. It is designed to easily and quickly extract files from compressed archives and ISO images. The application is aimed at Linux users who are looking for a lightweight and user-friendly graphical tool for managing compressed files.

---

## Features

- Supports extraction from the following formats:
  - ZIP archives
  - ISO images (native support without external libraries)
- Simple and organized graphical user interface (GUI)
- Progress bar to show extraction status
- Status bar to display process information
- Built-in Debug Console to show system messages and errors
- Night Mode for a comfortable experience in low-light conditions
- Manual destination folder selection
- Display file list before and after extraction
- Multithreading to prevent interface freezing during extraction

---

Developer By NGF76

Developed by NGF76, this application is completely free and open source. Check the license file to learn more. 


---
## Installation and Usage

### Running the Application Directly (AppImage)

1. Download the `FlatExtract-x86_64.AppImage` file from the [Releases page](https://github.com/NGF76/FlatExtract/releases).

2. Make it executable:
   ```bash
   chmod +x FlatExtract-x86_64.AppImage

---

IMPORTANT NOTE !!!!!!!

## Libraries Used

This project uses the following open-source libraries:

- **Qt** - Cross-platform application framework (LGPL v2.1 / v3)  
  [https://www.qt.io](https://www.qt.io)

- **QuaZIP** - Qt/C++ library for ZIP archives (LGPL v2.1+)  
  [https://quazip.sourceforge.net](https://quazip.sourceforge.net)

- **zlib** - Compression library (zlib/libpng license)  
  [https://zlib.net](https://zlib.net)

- **libcdio / libiso9660** - CD-ROM and ISO file system access (GPL / LGPL)  
  [https://www.gnu.org/software/libcdio](https://www.gnu.org/software/libcdio)

---

### License Compliance

FlatExtract is licensed under the **MIT License**.  
However, it uses libraries under other open-source licenses (LGPL, GPL).  
These libraries are dynamically linked and can be replaced by the user, in compliance with their respective licenses.

For more details, see the `LICENSE` file and the individual library licenses.
