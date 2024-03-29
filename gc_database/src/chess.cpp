#include "chess.h"
#include <string>
#include <vector>
#include <algorithm>
#include <sstream>
#include <iostream>
#include <filesystem>

// for compression
#include "lzma/7zTypes.h"
#include "lzma/LzmaDec.h"

namespace chess {

bool chessVerbose = false;

void toLower(std::string& str) {
    std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c) { return std::tolower(c); });
}

void toLower(const char* str) {
    std::transform(str, str + std::strlen(str), const_cast<char*>(str), [](unsigned char c) { return std::tolower(c); });
}

std::string posToCoordinateString(int pos) {
    int row = pos / 9, col = pos % 9;
    std::ostringstream stringStream;
    stringStream << char('a' + col) << 9 - row;
    return stringStream.str();
}

std::string getFileName(const std::string& path) {
    const auto pos = path.find_last_of('/');
    std::string str = (pos != std::string::npos) ? path.substr(pos + 1) : path;
    const auto dotPos = str.find_last_of(".");
    if (dotPos != std::string::npos) {
        str = str.substr(0, dotPos);
    }
    return str;
}

std::string getVersion() {
    char buf[10];
    std::snprintf(buf, sizeof(buf), "%d.%02d", CHESS_VERSION >> 8, CHESS_VERSION & 0xff);
    return buf;
}

#ifdef _WIN32
static void findFiles(std::vector<std::string>& names, const std::string& dirname) {
    const std::string search_path = dirname + "/*.*";
    WIN32_FIND_DATAA file;
    const HANDLE search_handle = FindFirstFileA(search_path.c_str(), &file);
    if (search_handle != INVALID_HANDLE_VALUE) {
        do {
            const std::string fullpath = dirname + "/" + file.cFileName;
            if ((file.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && (file.cFileName[0] != '.')) {
                findFiles(names, fullpath);
            } else {
                names.push_back(fullpath);
            }
        } while (FindNextFileA(search_handle, &file));
        FindClose(search_handle);
    }
}

std::vector<std::string> listdir(const std::string& dirname) {
    std::vector<std::string> names;
    findFiles(names, dirname);
    return names;
}

#else

std::vector<std::string> listdir(const std::string& dirname) {
    std::vector<std::string> vec;
    for (const auto& entry : std::filesystem::directory_iterator(dirname)) {
        if (entry.is_directory()) {
            const auto vec2 = listdir(entry.path().string());
            vec.insert(vec.end(), vec2.begin(), vec2.end());
        } else {
            vec.push_back(entry.path().string());
        }
    }
    return vec;
}


#endif

static void* _allocForLzma(ISzAllocPtr, size_t size) {
    return malloc(size);
}

static void _freeForLzma(ISzAllocPtr, void* addr) {
    free(addr);
}

static ISzAlloc _szAllocForLzma = { _allocForLzma, _freeForLzma };

static const Byte lzmaPropData[5] = { 93, 0, 0, 0, 1 };

    int decompress(char *dst, int uncompresslen, const char *src, int slen) {
        SizeT srcLen = slen, dstLen = uncompresslen;
        ELzmaStatus lzmaStatus;

        SRes res = LzmaDecode(
                              (Byte *)dst, &dstLen,
                              (const Byte *)src, &srcLen,
                              lzmaPropData, LZMA_PROPS_SIZE,
                              LZMA_FINISH_ANY,
                              &lzmaStatus, &_szAllocForLzma
                              );
        return res == SZ_OK ? (int)dstLen : -1;
    }

    i64 decompressAllBlocks(int blocksize, int blocknum, u32* blocktable, char *dest, i64 uncompressedlen, const char *src, i64 slen) {
        auto *s = src;
        auto p = dest;

        for(int i = 0; i < blocknum; i++) {
            int blocksz = (blocktable[i] & ~chess_UNCOMPRESS_BIT) - (i == 0 ? 0 : (blocktable[i - 1] & ~chess_UNCOMPRESS_BIT));
            int uncompressed = blocktable[i] & chess_UNCOMPRESS_BIT;

            if (uncompressed) {
                memcpy(p, s, blocksz);
                p += blocksz;
            } else {
                auto left = uncompressedlen - (i64)(p - dest);
                auto curBlockSize = (int)MIN(left, (i64)blocksize);

                auto originSz = decompress((char*)p, curBlockSize, s, blocksz);
                p += originSz;
            }
            s += blocksz;
        }

        return (i64)(p - dest);
    }
}

