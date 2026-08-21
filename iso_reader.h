// iso_reader.h
#pragma once
#include <cstdint>
#include <cstring>
#include <vector>
#include <string>
#include <fstream>
#include <unordered_set>
#include <QDebug>

// ===================== XDVDFS (Xbox 360 ISO) =====================

static const char* XISO_MAGIC = "MICROSOFT*XBOX*MEDIA";
static const size_t SECTOR_SIZE = 2048;

#pragma pack(push, 1)
struct XisoVolumeDescriptor {
    char     Magic1[20];
    uint32_t RootDirSector;
    uint32_t RootDirSize;
    uint64_t ImageCreationTime;
    uint8_t  Unused[1992];
    char     Magic2[20];
};

struct XisoDirEntry {
    uint16_t LeftOffset;
    uint16_t RightOffset;
    uint32_t StartSector;
    uint32_t FileSize;
    uint8_t  Attributes;
    uint8_t  NameLength;
};
#pragma pack(pop)

struct IsoFileEntry {
    std::string path;
    uint32_t startSector;
    uint32_t fileSize;
    bool isDirectory;
};

class IsoReader {
public:
    bool Open(const std::string& path) {
        file_.open(path, std::ios::binary);
        if (!file_) return false;
        file_.seekg(0, std::ios::end);
        fileSize_ = (uint64_t)file_.tellg();
        return LocateVolumeDescriptor();
    }

    bool ExtractFile(const std::string& targetName, std::vector<uint8_t>& outData) {
        return FindAndRead(baseOffset_ + (uint64_t)rootDirSector_ * SECTOR_SIZE,
                           rootDirSize_, targetName, outData);
    }

    // In-memory extraction. For large files, prefer ExtractBySectorToDisk below
    // to avoid buffering the whole file in RAM.
    bool ExtractBySector(uint32_t startSector, uint32_t fileSize,
                         std::vector<uint8_t>& outData) {
        uint64_t offset = baseOffset_ + (uint64_t)startSector * SECTOR_SIZE;
        outData.resize(fileSize);
        ReadAt(offset, outData.data(), fileSize);
        return true;
    }

    // Streaming version: writes directly to disk in small chunks instead of
    // buffering the whole file in RAM. Use this for extraction loops handling
    // arbitrarily large files to keep peak memory bounded and constant.
    bool ExtractBySectorToDisk(uint32_t startSector, uint32_t fileSize, const std::string& outPath) {
        std::ofstream out(outPath, std::ios::binary);
        if (!out) return false;

        uint64_t offset = baseOffset_ + (uint64_t)startSector * SECTOR_SIZE;
        uint32_t remaining = fileSize;
        const size_t chunkSize = 1024 * 1024; // 1MB chunks
        std::vector<uint8_t> buf(chunkSize);

        while (remaining > 0) {
            size_t toRead = remaining < chunkSize ? remaining : chunkSize;
            ReadAt(offset, buf.data(), toRead);
            out.write((const char*)buf.data(), (std::streamsize)toRead);
            offset += toRead;
            remaining -= (uint32_t)toRead;
        }
        out.close();
        return true;
    }

    std::vector<IsoFileEntry> ListAllFiles() {
        std::vector<IsoFileEntry> results;
        visitedDirOffsets_.clear();
        WalkDirectory(baseOffset_ + (uint64_t)rootDirSector_ * SECTOR_SIZE,
                      rootDirSize_, "", results, 0);
        return results;
    }

private:
    std::ifstream file_;
    uint64_t fileSize_ = 0;
    uint64_t baseOffset_ = 0;
    uint32_t rootDirSector_ = 0;
    uint32_t rootDirSize_ = 0;
    std::vector<uint64_t> visitedDirOffsets_;

    void ReadAt(uint64_t offset, void* dst, size_t len) {
        file_.clear();
        file_.seekg((std::streamoff)offset, std::ios::beg);
        file_.read((char*)dst, (std::streamsize)len);
    }

    bool LocateVolumeDescriptor() {
        static const uint64_t candidates[] = {
            0x00000000ULL, 0x0FD90000ULL, 0x02080000ULL,
        };
        for (uint64_t base : candidates) {
            if (TryReadVolumeAt(base)) return true;
        }
        uint64_t foundOffset = 0;
        if (ScanForMagic(foundOffset)) {
            return TryReadVolumeAt(foundOffset);
        }
        return false;
    }

    bool TryReadVolumeAt(uint64_t base) {
        uint64_t vdOffset = base + 32ULL * SECTOR_SIZE;
        if (vdOffset + sizeof(XisoVolumeDescriptor) > fileSize_) return false;

        XisoVolumeDescriptor vd{};
        ReadAt(vdOffset, &vd, sizeof(vd));

        if (memcmp(vd.Magic1, XISO_MAGIC, 20) != 0) return false;

        uint64_t rootByteOffset = base + (uint64_t)vd.RootDirSector * SECTOR_SIZE;
        if (rootByteOffset >= fileSize_) {
            qDebug() << "Rejected candidate at" << base << "- root dir sector out of range";
            return false;
        }
        if (vd.RootDirSize < 32 || vd.RootDirSize > 64 * 1024 * 1024) {
            qDebug() << "Rejected candidate at" << base << "- implausible root dir size:" << vd.RootDirSize;
            return false;
        }

        baseOffset_ = base;
        rootDirSector_ = vd.RootDirSector;
        rootDirSize_ = vd.RootDirSize;
        qDebug() << "XDVDFS found at base offset" << base
                 << "root sector" << rootDirSector_ << "root size" << rootDirSize_;
        return true;
    }

    bool ScanForMagic(uint64_t& foundBaseOffset) {
        const size_t chunkSize = 1024 * 1024;
        std::vector<char> buf(chunkSize + 32);
        for (uint64_t pos = 0; pos < fileSize_; pos += chunkSize) {
            file_.seekg((std::streamoff)pos, std::ios::beg);
            file_.read(buf.data(),
                       (std::streamsize)std::min<uint64_t>(chunkSize + 32, fileSize_ - pos));
            std::streamsize got = file_.gcount();
            for (std::streamsize i = 0; i + 20 <= got; i++) {
                uint64_t magicOffset = pos + (uint64_t)i;
                if (magicOffset % SECTOR_SIZE != 0) continue;
                if (memcmp(buf.data() + i, XISO_MAGIC, 20) == 0) {
                    if (magicOffset >= 32ULL * SECTOR_SIZE) {
                        foundBaseOffset = magicOffset - 32ULL * SECTOR_SIZE;
                        return true;
                    }
                }
            }
        }
        return false;
    }

    bool FindAndRead(uint64_t dirByteOffset, uint32_t dirSize,
                     const std::string& targetName, std::vector<uint8_t>& outData) {
        std::vector<uint8_t> table(dirSize);
        ReadAt(dirByteOffset, table.data(), dirSize);
        return SearchNode(table, 0, targetName, outData);
    }

    bool SearchNode(std::vector<uint8_t>& table, size_t entryOffset,
                    const std::string& targetName, std::vector<uint8_t>& outData) {
        if (entryOffset + sizeof(XisoDirEntry) > table.size()) return false;
        XisoDirEntry entry;
        memcpy(&entry, table.data() + entryOffset, sizeof(entry));
        if (entry.LeftOffset == 0xFFFF && entry.RightOffset == 0xFFFF && entry.NameLength == 0)
            return false;

        size_t nameStart = entryOffset + sizeof(XisoDirEntry);
        if (nameStart + entry.NameLength > table.size()) return false;

        std::string name((char*)table.data() + nameStart, entry.NameLength);
        std::string a = name, b = targetName;
        std::transform(a.begin(), a.end(), a.begin(), ::tolower);
        std::transform(b.begin(), b.end(), b.begin(), ::tolower);
        int cmp = b.compare(a);

        if (cmp == 0) {
            uint64_t fileOffset = baseOffset_ + (uint64_t)entry.StartSector * SECTOR_SIZE;
            outData.resize(entry.FileSize);
            ReadAt(fileOffset, outData.data(), entry.FileSize);
            return true;
        }

        bool isDirectory = (entry.Attributes & 0x10) != 0;
        if (isDirectory) {
            uint64_t subDirOffset = baseOffset_ + (uint64_t)entry.StartSector * SECTOR_SIZE;
            if (FindAndRead(subDirOffset, entry.FileSize, targetName, outData)) return true;
        }
        if (cmp < 0 && entry.LeftOffset != 0xFFFF)
            return SearchNode(table, (size_t)entry.LeftOffset * 4, targetName, outData);
        if (cmp > 0 && entry.RightOffset != 0xFFFF)
            return SearchNode(table, (size_t)entry.RightOffset * 4, targetName, outData);
        return false;
    }

    void WalkDirectory(uint64_t dirByteOffset, uint32_t dirSize,
                       const std::string& pathPrefix,
                       std::vector<IsoFileEntry>& results, int depth = 0) {
        if (dirSize == 0) return;

        if (depth > 32) {
            qDebug() << "WalkDirectory: max depth exceeded, aborting at"
                     << QString::fromStdString(pathPrefix);
            return;
        }
        if (dirSize > 64 * 1024 * 1024 || dirByteOffset + dirSize > fileSize_) {
            qDebug() << "Skipping bad directory: offset" << dirByteOffset << "size" << dirSize;
            return;
        }

        for (uint64_t seen : visitedDirOffsets_) {
            if (seen == dirByteOffset) {
                return; // cycle guard, silent
            }
        }
        visitedDirOffsets_.push_back(dirByteOffset);

        std::vector<uint8_t> table(dirSize);
        ReadAt(dirByteOffset, table.data(), dirSize);
        WalkNode(table, 0, pathPrefix, results, depth);
    }

    void WalkNode(std::vector<uint8_t>& table, size_t startEntryOffset,
                  const std::string& pathPrefix, std::vector<IsoFileEntry>& results,
                  int depth = 0) {
        std::vector<size_t> stack;
        stack.push_back(startEntryOffset);

        // Track entries already processed IN THIS TABLE so a Left/Right cycle
        // (entries pointing back at each other) can't cause the same entry to
        // be re-added and re-processed indefinitely.
        std::unordered_set<size_t> visitedEntries;

        size_t visitGuard = 0;
        const size_t maxVisits = table.size() / sizeof(XisoDirEntry) + 16; // can't exceed real entry count

        while (!stack.empty()) {
            if (++visitGuard > maxVisits) {
                qDebug() << "WalkNode: visit guard exceeded (should not happen with entry dedup), aborting at"
                         << QString::fromStdString(pathPrefix);
                break;
            }

            size_t entryOffset = stack.back();
            stack.pop_back();

            if (visitedEntries.count(entryOffset)) continue; // already processed, skip
            visitedEntries.insert(entryOffset);

            if (entryOffset + sizeof(XisoDirEntry) > table.size()) continue;

            XisoDirEntry entry;
            memcpy(&entry, table.data() + entryOffset, sizeof(entry));

            if (entry.LeftOffset == 0xFFFF && entry.RightOffset == 0xFFFF && entry.NameLength == 0)
                continue;

            size_t nameStart = entryOffset + sizeof(XisoDirEntry);
            if (nameStart + entry.NameLength > table.size()) continue;

            std::string name((char*)table.data() + nameStart, entry.NameLength);

            bool isDirectory = (entry.Attributes & 0x10) != 0;
            std::string fullPath = pathPrefix.empty() ? name : pathPrefix + "\\" + name;

            IsoFileEntry fileEntry;
            fileEntry.path = fullPath;
            fileEntry.startSector = entry.StartSector;
            fileEntry.fileSize = entry.FileSize;
            fileEntry.isDirectory = isDirectory;
            results.push_back(fileEntry);

            if (isDirectory && entry.FileSize > 0 && entry.FileSize < 64 * 1024 * 1024) {
                uint64_t subDirOffset = baseOffset_ + (uint64_t)entry.StartSector * SECTOR_SIZE;
                if (subDirOffset + entry.FileSize <= fileSize_) {
                    WalkDirectory(subDirOffset, entry.FileSize, fullPath, results, depth + 1);
                }
            }

            if (entry.RightOffset != 0xFFFF) {
                size_t rightOff = (size_t)entry.RightOffset * 4;
                if (rightOff + sizeof(XisoDirEntry) <= table.size() && !visitedEntries.count(rightOff)) {
                    stack.push_back(rightOff);
                }
            }
            if (entry.LeftOffset != 0xFFFF) {
                size_t leftOff = (size_t)entry.LeftOffset * 4;
                if (leftOff + sizeof(XisoDirEntry) <= table.size() && !visitedEntries.count(leftOff)) {
                    stack.push_back(leftOff);
                }
            }
        }
    }
};
