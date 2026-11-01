# Security Policy

## About This Project

FlatExtract is a personal educational project developed by a student. It is maintained on a best-effort basis, and I may not be able to respond to security reports immediately.

## Supported Versions

Only the latest release receives updates.

| Version | Supported          |
| ------- | ------------------ |
| 1.3.x   | :white_check_mark: |
| < 1.3   | :x:                |

## Reporting a Vulnerability

If you discover a security issue, please open a private security advisory on GitHub:

1. Go to the **Security** tab of this repository.
2. Click **"Report a vulnerability"**.
3. Describe the issue clearly.

I will review reports when time permits. Please be patient, as this is a side project maintained alongside my studies.

## Scope

This project is a file extraction tool. It does not handle sensitive user data, network communication, or authentication. The main security considerations are:
- Safe handling of archive files (avoiding path traversal)
- Proper handling of large or malformed files

## Disclaimer

This software is provided "AS IS", without warranty of any kind. See the LICENSE file for details.
