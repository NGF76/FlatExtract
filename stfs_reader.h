// stfs_reader.h
// Reads Xbox 360 STFS packages (CON / LIVE / PIRS) - the container format
// used for game content, title updates, system updates, DLC, etc.
// Based on the documented format at https://free60.org/System-Software/Formats/STFS/
#pragma once
#include <cstdint>
#include <cstring>
#include <vector>
#include <string>
#include <fstream>
#include <algorithm>
#include <QDebug>

struct StfsFileEntry {
    std::string path;       // full path with '\' separators, matching IsoFileEntry style
    uint32_t startBlock;
    uint32_t fileSize;
    bool isDirectory;
    bool blocksConsecutive; // bit 6 of flags byte
};

class StfsReader {
public:
    enum class Magic { CON, LIVE, PIRS, Unknown };

    bool Open(const std::string& path) {
        file_.open(path, std::ios::binary);
        if (!file_) return false;
        file_.seekg(0, std::ios::end);
        fileSize_ = (uint64_t)file_.tellg();
        return ParseHeader();
    }

    std::vector<StfsFileEntry> ListAllFiles() {
        std::vector<StfsFileEntry> results;
        if (!headerValid_) return results;

        std::vector<RawEntry> rawEntries = ReadFileListing();

        // Build paths: pathIndicator refers to the index of the parent entry
        // in this same listing, -1 (0xFFFF) = root.
        for (size_t i = 0; i < rawEntries.size(); i++) {
            std::string path = BuildPath(rawEntries, i);
            StfsFileEntry e;
            e.path = path;
            e.startBlock = rawEntries[i].startBlock;
            e.fileSize = rawEntries[i].fileSize;
            e.isDirectory = rawEntries[i].isDirectory;
            e.blocksConsecutive = rawEntries[i].consecutive;
            results.push_back(e);
        }
        return results;
    }

    // Extracts file data by following the block chain (handles non-consecutive files).
    // Extracts file data by following the block chain, writing directly to disk
    // in small chunks instead of buffering the whole file in RAM. Use this for
    // large files (audio, movies, textures) to avoid memory spikes.
    bool ExtractFileToDisk(const StfsFileEntry& entry, const std::string& outPath) {
        if (!headerValid_) return false;

        std::ofstream out(outPath, std::ios::binary);
        if (!out) return false;

        uint32_t block = entry.startBlock;
        uint32_t remaining = entry.fileSize;
        int guard = 0;

        while (remaining > 0 && block != 0xFFFFFF && guard < 200000) {
            guard++;
            uint64_t dataBlockNum = ComputeDataBlockNumber(block);
            uint64_t byteOffset = BlockToOffset(dataBlockNum);
            if (byteOffset + 0x1000 > fileSize_) return false;

            uint32_t chunk = remaining < 0x1000 ? remaining : 0x1000;
            uint8_t buf[0x1000]; // stack buffer, not heap - no accumulation
            ReadAt(byteOffset, buf, 0x1000);
            out.write((const char*)buf, chunk);
            remaining -= chunk;

            if (remaining == 0) break;

            if (entry.blocksConsecutive) {
                block = block + 1;
            } else {
                uint32_t nextBlock = ReadNextBlockFromHashTable(block);
                if (nextBlock == 0xFFFFFF || nextBlock == block) break;
                block = nextBlock;
            }
        }
        out.close();
        return true;
    }

    bool ExtractFile(const StfsFileEntry& entry, std::vector<uint8_t>& outData) {
        if (!headerValid_) return false;
        outData.clear();
        outData.reserve(entry.fileSize);

        uint32_t block = entry.startBlock;
        uint32_t remaining = entry.fileSize;
        int guard = 0;

        while (remaining > 0 && block != 0xFFFFFF && guard < 200000) {
            guard++;
            uint64_t dataBlockNum = ComputeDataBlockNumber(block);
            uint64_t byteOffset = BlockToOffset(dataBlockNum);
            if (byteOffset + 0x1000 > fileSize_) {
                qDebug() << "STFS: block offset out of range, aborting extraction";
                return false;
            }

            uint32_t chunk = remaining < 0x1000 ? remaining : 0x1000;
            std::vector<uint8_t> buf(0x1000);
            file_.clear();
            file_.seekg((std::streamoff)byteOffset, std::ios::beg);
            file_.read((char*)buf.data(), 0x1000);
            outData.insert(outData.end(), buf.begin(), buf.begin() + chunk);
            remaining -= chunk;

            if (remaining == 0) break;

            // Reverted: consecutive flag confirmed correct, hash lookup formula has separate bug
            bool forceHashLookup = false;
            if (entry.blocksConsecutive && !forceHashLookup) {
                qDebug() << "STFS chain: block" << block << "-> (consecutive) next" << (block + 1);
                block = block + 1;
            } else {
                uint32_t nextBlock = ReadNextBlockFromHashTable(block);
                qDebug() << "STFS chain: block" << block << "-> hash table says next" << nextBlock;
                if (nextBlock == 0xFFFFFF || nextBlock == block) {
                    qDebug() << "STFS: block chain ended early or looped, got"
                             << (int)outData.size() << "of" << entry.fileSize << "bytes";
                    break;
                }
                block = nextBlock;
            }
        }
        return true;
    }

    // Streams the file directly to an already-open output stream, block by block,
    // instead of buffering the whole file in memory first. Keeps peak RAM usage
    // bounded to ~4KB per file regardless of file size.
    bool ExtractFileToStream(const StfsFileEntry& entry, std::ofstream& out) {
        if (!headerValid_) return false;

        uint32_t block = entry.startBlock;
        uint32_t remaining = entry.fileSize;
        int guard = 0;
        std::vector<uint8_t> buf(0x1000);

        while (remaining > 0 && block != 0xFFFFFF && guard < 200000) {
            guard++;
            uint64_t dataBlockNum = ComputeDataBlockNumber(block);
            uint64_t byteOffset = BlockToOffset(dataBlockNum);
            if (byteOffset + 0x1000 > fileSize_) return false;

            uint32_t chunk = remaining < 0x1000 ? remaining : 0x1000;
            ReadAt(byteOffset, buf.data(), 0x1000);
            out.write((const char*)buf.data(), chunk); // write immediately, don't accumulate
            remaining -= chunk;

            if (remaining == 0) break;

            if (entry.blocksConsecutive) {
                block = block + 1;
            } else {
                uint32_t nextBlock = ReadNextBlockFromHashTable(block);
                if (nextBlock == 0xFFFFFF || nextBlock == block) break;
                block = nextBlock;
            }
        }
        return true;
    }

    Magic GetMagic() const { return magic_; }
    uint32_t GetTitleId() const { return titleId_; }
    std::string GetDisplayName() const { return displayName_; }

private:
    struct RawEntry {
        std::string name;
        uint32_t blocksAllocated;
        uint32_t startBlock;
        int16_t pathIndicator;
        uint32_t fileSize;
        bool isDirectory;
        bool consecutive;
    };

    std::ifstream file_;
    uint64_t fileSize_ = 0;
    bool headerValid_ = false;
    Magic magic_ = Magic::Unknown;
    uint32_t headerSize_ = 0;
    uint8_t blockSeparation_ = 0;
    uint32_t fileTableBlockNumber_ = 0;
    uint16_t fileTableBlockCount_ = 0;
    uint32_t titleId_ = 0;
    std::string displayName_;

    static uint32_t ReadBE32(const uint8_t* p) {
        return (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3];
    }
    static uint16_t ReadBE16(const uint8_t* p) {
        return (p[0] << 8) | p[1];
    }
    static uint32_t ReadBE24(const uint8_t* p) {
        return (p[0] << 16) | (p[1] << 8) | p[2];
    }
    static uint32_t ReadLE24(const uint8_t* p) {
        return p[0] | (p[1] << 8) | (p[2] << 16);
    }

    void ReadAt(uint64_t offset, void* dst, size_t len) {
        file_.clear();
        file_.seekg((std::streamoff)offset, std::ios::beg);
        file_.read((char*)dst, (std::streamsize)len);
    }

    bool ParseHeader() {
        if (fileSize_ < 0x4000) return false;

        uint8_t magicBytes[4];
        ReadAt(0, magicBytes, 4);
        if (memcmp(magicBytes, "CON ", 4) == 0) magic_ = Magic::CON;
        else if (memcmp(magicBytes, "LIVE", 4) == 0) magic_ = Magic::LIVE;
        else if (memcmp(magicBytes, "PIRS", 4) == 0) magic_ = Magic::PIRS;
        else {
            qDebug() << "STFS: not a valid STFS package (bad magic)";
            return false;
        }

        uint8_t buf4[4];
        ReadAt(0x340, buf4, 4);
        headerSize_ = ReadBE32(buf4);

        // Volume descriptor starts at 0x379
        ReadAt(0x379 + 0x02, buf4, 1);
        blockSeparation_ = buf4[0];

        uint8_t buf2[2];
        ReadAt(0x379 + 0x03, buf2, 2);
        fileTableBlockCount_ = ReadBE16(buf2);

        uint8_t buf3[3];
        ReadAt(0x379 + 0x05, buf3, 3);
        // File Table Block Number - stored big-endian per convention with rest of metadata
        fileTableBlockNumber_ = ReadBE24(buf3);

        ReadAt(0x360, buf4, 4);
        titleId_ = ReadBE32(buf4);

        // Display name (English locale, first 0x80 bytes of the 0x900 block at 0x411), UTF-16BE
        std::vector<uint8_t> nameBuf(0x80);
        ReadAt(0x411, nameBuf.data(), 0x80);
        displayName_.clear();
        for (size_t i = 0; i + 1 < nameBuf.size(); i += 2) {
            uint16_t ch = (nameBuf[i] << 8) | nameBuf[i + 1];
            if (ch == 0) break;
            if (ch < 128) displayName_ += (char)ch;
        }

        headerValid_ = true;
        QString magicStr = (magic_ == Magic::CON) ? "CON" : (magic_ == Magic::LIVE) ? "LIVE" : (magic_ == Magic::PIRS) ? "PIRS" : "UNKNOWN";
        qDebug() << "STFS opened - magic=" << magicStr << "HeaderSize=" << headerSize_
                 << "FileTableBlock=" << fileTableBlockNumber_
                 << "TitleId=" << QString::number(titleId_, 16)
                 << "Name=" << QString::fromStdString(displayName_)
                 << "HeaderAdjust=0x" << QString::number(HeaderAdjust(), 16)
                 << "BlockShift=" << BlockShift();
        return true;
    }

    uint32_t HeaderAdjust() const {
        return (headerSize_ + 0xFFF) & 0xF000;
    }

    int BlockShift() const {
        if (HeaderAdjust() == 0xB000) return 1;
        return ((blockSeparation_ & 1) == 1) ? 0 : 1;
    }

    uint64_t BlockToOffset(uint64_t block) const {
        return (uint64_t)HeaderAdjust() + (block << 12);
    }

    // Converts a "file data" block number into the true block number
    // (accounting for interspersed hash table blocks).
    uint64_t ComputeDataBlockNumber(uint64_t xBlock) const {
        int shift = BlockShift();
        bool applyShift = (magic_ == Magic::CON); // reverted: original condition was correct

        uint64_t xBase = (xBlock + 0xAA) / 0xAA;
        if (applyShift) xBase = xBase << shift;
        uint64_t xReturn = xBase + xBlock;

        if (xBlock > 0xAA) {
            xBase = (xBlock + 0x70E4) / 0x70E4;
            if (applyShift) xBase = xBase << shift;
            xReturn += xBase;

            if (xBlock > 0x70E4) {
                xBase = (xBlock + 0x4AF768) / 0x4AF768;
                if (applyShift) xBase = xBase << 1;
                xReturn += xBase;
            }
        }
        return xReturn;
    }

    // Location (true block number) of the level-0 hash table block covering xBlock.
    uint64_t ComputeLevel0HashBlockNumber(uint64_t xBlock) const {
        int shift = BlockShift();
        bool applyShift = (magic_ == Magic::CON);
        uint32_t blockStep0 = (HeaderAdjust() == 0xB000) ? 0xAC
                                                         : (((blockSeparation_ & 1) == 1) ? 0xAB : 0xAC);

        uint64_t xReturn = xBlock / 0xAA;
        uint64_t xStep = xReturn * blockStep0;

        if (xReturn == 0) return xStep;

        uint64_t xLevel1 = xBlock / 0x70E4;
        uint64_t xBase = xLevel1 + 1;
        if (applyShift) xStep += (xBase << shift);
        else xStep += xBase;

        if (xLevel1 == 0) return xStep;

        if (applyShift) return xStep + (1ULL << shift);
        return xStep + 1;
    }

    uint32_t ReadNextBlockFromHashTable(uint32_t xBlock) {
        uint64_t hashBlockNum = ComputeLevel0HashBlockNumber(xBlock);
        uint64_t hashByteOffset = BlockToOffset(hashBlockNum);
        if (hashByteOffset + 0x1000 > fileSize_) {
            qDebug() << "  hash lookup: block" << xBlock << "-> hashBlockNum" << hashBlockNum
                     << "offset" << hashByteOffset << "(OUT OF RANGE)";
            return 0xFFFFFF;
        }

        uint32_t recordIndex = xBlock % 0xAA;
        uint64_t recordOffset = hashByteOffset + (uint64_t)recordIndex * 0x18;

        uint8_t rec[0x18];
        ReadAt(recordOffset, rec, 0x18);

        uint32_t nextLE = ReadLE24(&rec[0x15]);
        uint32_t nextBE = ReadBE24(&rec[0x15]);
        qDebug() << "  hash lookup: block" << xBlock << "-> hashBlockNum" << hashBlockNum
                 << "recordIndex" << recordIndex << "recordOffset" << recordOffset
                 << "status=0x" << QString::number(rec[0x14], 16)
                 << "nextLE=" << nextLE << "nextBE=" << nextBE;

        return nextLE; // next block field
    }

    std::vector<RawEntry> ReadFileListing() {
        std::vector<RawEntry> entries;
        uint32_t block = fileTableBlockNumber_;
        uint16_t blocksToRead = fileTableBlockCount_ > 0 ? fileTableBlockCount_ : 1;

        bool stop = false;
        for (uint16_t b = 0; b < blocksToRead && !stop; b++) {
            uint64_t dataBlockNum = ComputeDataBlockNumber(block + b);
            uint64_t byteOffset = BlockToOffset(dataBlockNum);
            if (byteOffset + 0x1000 > fileSize_) break;

            std::vector<uint8_t> buf(0x1000);
            ReadAt(byteOffset, buf.data(), 0x1000);

            // Each entry record is 0x40 bytes; 0x1000 / 0x40 = 64 per block
            for (int i = 0; i < 64; i++) {
                const uint8_t* rec = buf.data() + i * 0x40;

                // All-zero record marks the end of the listing
                bool allZero = true;
                for (int j = 0; j < 0x40; j++) if (rec[j] != 0) { allZero = false; break; }
                if (allZero) { stop = true; break; }

                uint8_t flagsByte = rec[0x28];
                uint8_t nameLen = flagsByte & 0x3F;
                bool isDir = (flagsByte & 0x80) != 0;
                bool consecutive = (flagsByte & 0x40) != 0;
                if (nameLen == 0 || nameLen > 0x28) continue;

                RawEntry e;
                e.name.assign((const char*)rec, nameLen);
                e.blocksAllocated = ReadLE24(&rec[0x29]);
                e.startBlock = ReadLE24(&rec[0x2F]);
                e.pathIndicator = (int16_t)ReadBE16(&rec[0x32]);
                e.fileSize = ReadBE32(&rec[0x34]);
                e.isDirectory = isDir;
                e.consecutive = consecutive;
                entries.push_back(e);
            }
        }
        return entries;
    }

    std::string BuildPath(const std::vector<RawEntry>& entries, size_t index) {
        std::vector<std::string> parts;
        int cur = (int)index;
        int guard = 0;
        while (cur >= 0 && guard < 64) {
            guard++;
            parts.push_back(entries[cur].name);
            int16_t parentIdx = entries[cur].pathIndicator;
            if (parentIdx == -1) break;
            cur = parentIdx;
        }
        std::string path;
        for (auto it = parts.rbegin(); it != parts.rend(); ++it) {
            if (!path.empty()) path += "\\";
            path += *it;
        }
        return path;
    }
};
