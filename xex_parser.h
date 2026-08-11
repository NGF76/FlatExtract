#ifndef XEX_PARSER_H
#define XEX_PARSER_H

// xex_parser.h
#pragma once
#include <cstdint>
#include <cstring>
#include <vector>
#include <string>

static inline uint32_t swap32(uint32_t v) {
    return ((v & 0xFF) << 24) | ((v & 0xFF00) << 8) |
           ((v & 0xFF0000) >> 8) | ((v & 0xFF000000) >> 24);
}
static inline uint16_t swap16(uint16_t v) {
    return (uint16_t)((v >> 8) | (v << 8));
}

#pragma pack(push, 1)
struct XexHeader {
    uint32_t Magic;
    uint32_t ModuleFlags;
    uint32_t SizeOfHeaders;
    uint32_t SizeOfDiscardableHeaders;
    uint32_t SecurityInfoOffset;
    uint32_t HeaderCount;
};

struct XexOptHeaderEntry {
    uint32_t Key;
    uint32_t Value;
};

struct XexExecutionInfo {
    uint32_t MediaId;
    uint32_t Version;
    uint32_t BaseVersion;
    uint32_t TitleId;
    uint8_t  Platform;
    uint8_t  ExecutableType;
    uint8_t  DiscNum;
    uint8_t  DiscCount;
    uint32_t SaveGameId;
};

struct XexFileFormatInfo {
    uint32_t InfoSize;
    uint16_t EncryptionType;
    uint16_t CompressionType;
};
#pragma pack(pop)

enum XexHeaderKeys : uint32_t {
    XEX_HEADER_RESOURCE_INFO      = 0x000002FF,
    XEX_HEADER_FILE_FORMAT_INFO   = 0x000003FF,
    XEX_HEADER_ORIGINAL_BASE_ADDR = 0x00010001,
    XEX_HEADER_ENTRY_POINT        = 0x00010100,
    XEX_HEADER_IMAGE_BASE_ADDRESS = 0x00010201,
    XEX_HEADER_IMPORT_LIBRARIES   = 0x000103FF,
    XEX_HEADER_ORIGINAL_PE_NAME   = 0x000183FF,
    XEX_HEADER_EXECUTION_INFO     = 0x00040006,
    CxGetVersion = 0x00000001,
    NbtNetbios = 0x00000002,
    SmbCloseHandle = 0x00000003,
};

struct ParsedXex {
    XexHeader header{};
    uint32_t securityInfoOffset = 0;
    uint32_t entryPoint = 0;
    uint32_t imageBaseAddress = 0;
    uint16_t compressionType = 0xFFFF;
    uint16_t encryptionType = 0xFFFF;
    uint32_t titleId = 0;
    std::string originalPeName;
};

inline bool ParseXex(const std::vector<uint8_t>& buf, ParsedXex& out) {
    if (buf.size() < sizeof(XexHeader)) return false;

    memcpy(&out.header, buf.data(), sizeof(XexHeader));
    out.header.Magic = swap32(out.header.Magic);
    out.header.ModuleFlags = swap32(out.header.ModuleFlags);
    out.header.SizeOfHeaders = swap32(out.header.SizeOfHeaders);
    out.header.SizeOfDiscardableHeaders = swap32(out.header.SizeOfDiscardableHeaders);
    out.header.SecurityInfoOffset = swap32(out.header.SecurityInfoOffset);
    out.header.HeaderCount = swap32(out.header.HeaderCount);

    if (out.header.Magic != 0x58455832) return false;
    out.securityInfoOffset = out.header.SecurityInfoOffset;

    size_t tableOffset = sizeof(XexHeader);
    for (uint32_t i = 0; i < out.header.HeaderCount; i++) {
        size_t entryOff = tableOffset + i * sizeof(XexOptHeaderEntry);
        if (entryOff + sizeof(XexOptHeaderEntry) > buf.size()) break;

        XexOptHeaderEntry entry;
        memcpy(&entry, buf.data() + entryOff, sizeof(entry));
        entry.Key = swap32(entry.Key);
        entry.Value = swap32(entry.Value);

        switch (entry.Key) {
        case XEX_HEADER_ENTRY_POINT:
            out.entryPoint = entry.Value;
            break;
        case XEX_HEADER_IMAGE_BASE_ADDRESS:
            out.imageBaseAddress = entry.Value;
            break;
        case XEX_HEADER_EXECUTION_INFO: {
            uint32_t off = entry.Value;
            if (off + sizeof(XexExecutionInfo) <= buf.size()) {
                XexExecutionInfo exec;
                memcpy(&exec, buf.data() + off, sizeof(exec));
                out.titleId = swap32(exec.TitleId);
            }
            break;
        }
        case XEX_HEADER_FILE_FORMAT_INFO: {
            uint32_t off = entry.Value;
            if (off + sizeof(XexFileFormatInfo) <= buf.size()) {
                XexFileFormatInfo fmt;
                memcpy(&fmt, buf.data() + off, sizeof(fmt));
                out.encryptionType = swap16(fmt.EncryptionType);
                out.compressionType = swap16(fmt.CompressionType);
            }
            break;
        }
        case XEX_HEADER_ORIGINAL_PE_NAME: {
            uint32_t off = entry.Value;
            if (off + 4 <= buf.size()) {
                uint32_t len = swap32(*(uint32_t*)(buf.data() + off));
                if (off + 4 + len <= buf.size()) {
                    out.originalPeName.assign((const char*)(buf.data() + off + 4), len);
                }
            }
            break;
        }
        default:
            break;
        }
    }
    return true;
}

#endif // XEX_PARSER_H
