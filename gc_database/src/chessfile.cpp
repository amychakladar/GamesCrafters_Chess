#include <fstream>
#include <iomanip>
#include <ctime>

#include "chess.h"
#include "chessFile.h"
#include "chessKey.h"

using namespace chess;

extern int subppp_sizes[7];
extern int *kk_2, *kk_8;

//////////////////////////////////////////////////////////////////////

#define TB_ILLEGAL              0
#define TB_UNSET                1
#define TB_MISSING              2
#define TB_WINING               3
#define TB_UNKNOWN              4
#define TB_DRAW                 5

#define TB_START_MATING         (TB_DRAW + 1)
#define TB_START_LOSING         130

#define TB_SPECIAL_DRAW         0
#define TB_SPECIAL_START_MATING (TB_SPECIAL_DRAW + 1)
#define TB_SPECIAL_START_LOSING 128

//////////////////////////////////////////////////////////////////////

const char* chessFileExtensions[] = {
    ".mtb", ".zmt", nullptr
};

bool chessFile::knownExtension(const std::string& path) {

    for (int i = 0; chessFileExtensions[i]; i++) {
        if (path.find(chessFileExtensions[i]) != std::string::npos) {
            return true;
        }
    }
    return false;
}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
chessFile::chessFile() {
    pBuf[0] = pBuf[1] = pCompressBuf = nullptr;
    compressBlockTables[0] = compressBlockTables[1] = nullptr;
    header = nullptr;
    memMode = chessMemMode::tiny;
    loadStatus = chessLoadStatus::none;
    reset();
}

chessFile::~chessFile() {
    removeBuffers();
    if (header) {
        delete header;
        header = nullptr;
    }
}

void chessFile::reset() {
    if (header != nullptr) {
        header->reset();
    }

    path[0] = path[1]= "";
    startpos[0] = 0; endpos[0] = 0; startpos[1] = 0; endpos[1] = 0;
};

void chessFile::removeBuffers() {
    if (pCompressBuf) free(pCompressBuf);
    pCompressBuf = nullptr;

    for (int i = 0; i < 2; i++) {
        if (pBuf[i]) {
            free(pBuf[i]);
            pBuf[i] = nullptr;
        }

        if (compressBlockTables[i]) {
            free(compressBlockTables[i]);
            compressBlockTables[i] = nullptr;
        }

        startpos[i] = endpos[i] = 0;
    }
    loadStatus = chessLoadStatus::none;
}

//////////////////////////////////////////////////////////////////////
void chessFile::merge(chessFile& otherchessFile)
{
    for(int sd = 0; sd < 2; sd++) {
        Side side = static_cast<Side>(sd);
        if (header == nullptr) {
            auto path = otherchessFile.getPath(sd);
            if (!path.empty()) {
                setPath(path, sd);
            }
            continue;
        }

        if (otherchessFile.header->isSide(side)) {
            header->addSide(side);
            setPath(otherchessFile.getPath(sd), sd);

            if (compressBlockTables[sd]) {
                free(compressBlockTables[sd]);
            }
            compressBlockTables[sd] = otherchessFile.compressBlockTables[sd];

            if (pBuf[sd] == nullptr && otherchessFile.pBuf[sd] != nullptr) {
                pBuf[sd] = otherchessFile.pBuf[sd];
                startpos[sd] = otherchessFile.startpos[sd];
                endpos[sd] = otherchessFile.endpos[sd];

                otherchessFile.pBuf[sd] = nullptr;
                otherchessFile.startpos[sd] = 0;
                otherchessFile.endpos[sd] = 0;
                otherchessFile.compressBlockTables[sd] = nullptr;
            }
        }
    }
}

void chessFile::setPath(const std::string& s, int sd) {
    auto ss = s;
    toLower(ss);
    if (sd != 0 && sd != 1) {
        sd = (ss.find("w.") != std::string::npos) ? W : B;
    }
    path[sd] = s;
}

bool chessFile::createBuf(i64 len, int sd) {
    pBuf[sd] = (char *)malloc(len + 16);
    startpos[sd] = 0; endpos[sd] = 0;
    return pBuf[sd];
}


//////////////////////////////////////////////////////////////////////
// Preload files
//////////////////////////////////////////////////////////////////////
bool chessFile::preload(const std::string& path, chessMemMode _memMode, chessLoadMode _loadMode) {
    if (_memMode == chessMemMode::smart) {
        _memMode = getSize() < chess_SMART_MODE_THRESHOLD ? chessMemMode::all : chessMemMode::tiny;
    }

    memMode = _memMode;
    loadMode = _loadMode;

    loadStatus = chessLoadStatus::none;
    if (loadMode == chessLoadMode::onrequest) {
        auto theName = getFileName(path);
        if (theName.length() < 4) {
            return false;
        }
        toLower(theName);
        int loadingSd = theName.find("w") != std::string::npos ? W : B;
        setPath(path, loadingSd);

        theName = theName.substr(0, theName.length() - 1); // remove W / B
        chessName = theName;

        setupIdxComputing(getName(), 0, 3);
        return true;
    }

    bool r = loadHeaderAndTable(path);
    loadStatus = r ? chessLoadStatus::loaded : chessLoadStatus::error;
    return r;
}

// Load all data too if requested
bool chessFile::loadHeaderAndTable(const std::string& path) {
    assert(path.size() > 8);
    std::ifstream file(path, std::ios::binary);

    // if there are files for both sides, header has been created already
    auto oldSide = Side::none;
    if (header == nullptr) {
        header = new chessFileHeader();
    } else {
        oldSide = header->isSide(Side::black) ? Side::black : Side::white;
    }
    auto loadingSide = Side::none;

    bool r = false;
    if (file && header->readFile(file) && header->isValid()) {
        loadingSide = header->isSide(Side::white) ? Side::white : Side::black;
        assert(loadingSide == (path.find("w.") != std::string::npos ? Side::white : Side::black));
        chessName = header->name;

        setPath(path, static_cast<int>(loadingSide));
        header->setOnlySide(loadingSide);
        assert(loadingSide == (path.find("w.") != std::string::npos ? Side::white : Side::black));
        r = loadingSide != Side::none;

        if (r) {
            setupIdxComputing(getName(), header->order, header->getVersion());

            if (oldSide != Side::none) {
                header->addSide(oldSide);
            }
        }
    }

    auto sd = static_cast<int>(loadingSide);
    startpos[sd] = endpos[sd] = 0;

    if (r && isCompressed()) {
        // Create & read compress block table
        auto blockCnt = getCompresseBlockCount();
        int blockTableSz = blockCnt * sizeof(u32);

        compressBlockTables[sd] = (u32*) malloc(blockTableSz + 64);

        if (!file.read((char *)compressBlockTables[sd], blockTableSz)) {
            if (chessVerbose) {
                std::cerr << "Error: cannot read " << path << std::endl;
            }
            file.close();
            free(compressBlockTables[sd]);
            compressBlockTables[sd] = nullptr;
            return false;
        }

    }

    if (r && memMode == chessMemMode::all) {
        r = loadAllData(file, loadingSide);
    }
    file.close();


    if (!r && chessVerbose) {
        std::cerr << "Error: cannot read " << path << std::endl;
    }

    return r;
}

bool chessFile::loadAllData(std::ifstream& file, Side side) {

    auto sd = static_cast<int>(side);
    startpos[sd] = endpos[sd] = 0;

    if (isCompressed()) {
        auto blockCnt = getCompresseBlockCount();
        int blockTableSz = blockCnt * sizeof(u32);

        i64 seekpos = chess_HEADER_SIZE + blockTableSz;
        file.seekg(seekpos, std::ios::beg);

        createBuf(getSize(), sd); assert(pBuf[sd]);

        auto compDataSz = compressBlockTables[sd][blockCnt - 1] & ~chess_UNCOMPRESS_BIT;

        char* tempBuf = (char*) malloc(compDataSz + 64);
        if (file.read(tempBuf, compDataSz)) {
            auto originSz = decompressAllBlocks(chess_SIZE_COMPRESS_BLOCK, blockCnt, compressBlockTables[sd], (char*)pBuf[sd], getSize(), tempBuf, compDataSz);
            assert(originSz == getSize());

            endpos[sd] = originSz;
        }

        free(tempBuf);
        free(compressBlockTables[sd]);
        compressBlockTables[sd] = nullptr;
    } else {
        auto sz = getSize();
        createBuf(sz, sd);
        i64 seekpos = chess_HEADER_SIZE;
        file.seekg(seekpos, std::ios::beg);

        if (file.read(pBuf[sd], sz)) {
            endpos[sd] = sz;
        }
    }

    return startpos[sd] < endpos[sd];
}

void chessFile::checkToLoadHeaderAndTable() {
    if (header != nullptr && loadStatus != chessLoadStatus::none) {
        return;
    }

    std::lock_guard<std::mutex> thelock(mtx);
    if (header != nullptr && loadStatus != chessLoadStatus::none) {
        return;
    }

    bool r = false;
    if (!path[0].empty() && !path[1].empty()) {
        r = loadHeaderAndTable(path[0]) && loadHeaderAndTable(path[1]);
    } else {
        auto thepath = path[0].empty() ? path[1] : path[0];
        r = loadHeaderAndTable(thepath);
    }

    loadStatus = r ? chessLoadStatus::loaded : chessLoadStatus::error;
}

bool chessFile::readBuf(i64 idx, int sd)
{
    if (!pBuf[sd]) {
        createBuf(getBufSize(), sd);
    }

    auto bufCnt = MIN(getBufItemCnt(), getSize() - idx);
    auto bufsz = bufCnt;

    bool r = false;
    std::ifstream file(getPath(sd), std::ios::binary);
    if (file) {
        if (memMode == chessMemMode::all) {
            Side side = static_cast<Side>(sd);
            r = loadAllData(file, side);
        } else if (isCompressed() && compressBlockTables[sd]) {
            r = readCompressedBlock(file, idx, sd, (char*)pBuf[sd]);
        } else {
            auto beginIdx = (idx + bufCnt <= getSize()) ? idx : 0;
            auto x = beginIdx;
            i64 seekpos = chess_HEADER_SIZE + x;
            file.seekg(seekpos, std::ios::beg);

            if (file.read(pBuf[sd], bufsz)) {
                startpos[sd] = beginIdx;
                endpos[sd] = beginIdx + bufCnt;
                r = true;
            }
        }
    }

    file.close();

    if (!r && chessVerbose) {
        std::cerr << "Error: cannot read " << getPath(sd) << std::endl;
    }

    return r;
}

bool chessFile::readCompressedBlock(std::ifstream& file, i64 idx, int sd, char* pDest)
{
    auto blockCnt = getCompresseBlockCount();
    int blockTableSz = blockCnt * sizeof(u32);

    const int blockSize = chess_SIZE_COMPRESS_BLOCK;
    auto blockIdx = idx / blockSize;
    startpos[sd] = endpos[sd] = blockIdx * blockSize;

    auto iscompressed = !(compressBlockTables[sd][blockIdx] & chess_UNCOMPRESS_BIT);
    auto blockOffset = blockIdx == 0 ? 0 : (compressBlockTables[sd][blockIdx - 1] & ~chess_UNCOMPRESS_BIT);

    auto compDataSz = (compressBlockTables[sd][blockIdx] & ~chess_UNCOMPRESS_BIT) - blockOffset;

    i64 seekpos = chess_HEADER_SIZE + blockTableSz + blockOffset;
    file.seekg(seekpos, std::ios::beg);

    if (iscompressed) {
        if (pCompressBuf == nullptr) {
            pCompressBuf = (char*) malloc(chess_SIZE_COMPRESS_BLOCK * 3 / 2);
        }
        if (file.read(pCompressBuf, compDataSz)) {
            auto curBlockSize = (int)MIN(getSize() - startpos[sd], (i64)blockSize);
            auto originSz = decompress(pDest, curBlockSize, pCompressBuf, compDataSz);
            endpos[sd] += originSz;
            return true;
        }
    } else if (file.read(pDest, compDataSz)) {
        endpos[sd] += compDataSz;
        return true;
    }

    if (chessVerbose) {
        std::cerr << "Error: cannot read " << getPath(sd) << std::endl;
    }
    return false;
}

//////////////////////////////////////////////////////////////////////
// Get scores
//////////////////////////////////////////////////////////////////////
int chessFile::cellToScore(char cell) {
    if (header->property & chess_PROP_SPECIAL_SCORE_RANGE) {
        u8 s = (u8)cell;
        if (s >= TB_SPECIAL_DRAW) {
            if (s == TB_SPECIAL_DRAW) return chess_SCORE_DRAW;
            if (s < TB_SPECIAL_START_LOSING) {
                int mi = (s - TB_SPECIAL_START_MATING) * 2 + 1;
                return chess_SCORE_MATE - mi;
            }

            int mi = (s - TB_SPECIAL_START_LOSING) * 2;
            return -chess_SCORE_MATE + mi;
        }
        return chess_SCORE_DRAW;
    }

    u8 s = (u8)cell;
    if (s >= TB_DRAW) {
        if (s == TB_DRAW) return chess_SCORE_DRAW;
        if (s < TB_START_LOSING) {
            int mi = (s - TB_START_MATING) * 2 + 1;
            return chess_SCORE_MATE - mi;
        }

        int mi = (s - TB_START_LOSING) * 2;
        return -chess_SCORE_MATE + mi;
    }

    switch (s) {
        case TB_MISSING:
            return chess_SCORE_MISSING;

        case TB_WINING:
            return chess_SCORE_WINNING;

        case TB_UNKNOWN:
            return chess_SCORE_UNKNOWN;

        case TB_ILLEGAL:
            return chess_SCORE_ILLEGAL;

        case TB_UNSET:
        default:
            return chess_SCORE_UNSET;
    }
}

char chessFile::getCell(i64 idx, Side side)
{
    if (idx >= getSize()) {
        return TB_MISSING;
    }

    int sd = static_cast<int>(side);

    if (!isDataReady(idx, sd)) {
        if (!readBuf(idx, sd)) {
            return TB_MISSING;
        }
    }

    char ch = pBuf[sd][idx - startpos[sd]];
    return ch;
}

char chessFile::getCell(const chessBoardCore& board, Side side) {
    i64 key = getKey(board).key;
    return getCell(key, side);
}

int chessFile::getScoreNoLock(const chessBoardCore& board, Side side) {
    chessKeyRec r = getKey(board);
    if (r.flipSide) {
        side = getXSide(side);
    }
    return getScoreNoLock(r.key, side);
}

int chessFile::getScoreNoLock(i64 idx, Side side)
{
    char cell = getCell(idx, side);
    return cellToScore(cell);
}

int chessFile::getScore(const chessBoardCore& board, Side side, bool useLock) {
    chessKeyRec r = getKey(board);
    if (r.flipSide) {
        side = getXSide(side);
    }

    return getScore(r.key, side, useLock);
}

int chessFile::getScore(i64 idx, Side side, bool useLock)
{
    checkToLoadHeaderAndTable();

    if (useLock && (memMode != chessMemMode::all || !isDataReady(idx, static_cast<int>(side)))) {
        std::lock_guard<std::mutex> thelock(sdmtx[static_cast<int>(side)]);
        return getScoreNoLock(idx, side);
    }
    return getScoreNoLock(idx, side);
}

//////////////////////////////////////////////////////////////////////
// Parse name
//////////////////////////////////////////////////////////////////////


i64 chessFile::parseAttr(const std::string& name, int* idxArr, i64* idxMult, int* pieceCount, u16 order, int version)
{
    auto havingPawns = name.find("p") != std::string::npos;

    int k = 0;
    int pawnCnt[] = { 0, 0};

    for (int i = 0, sd = W; i < (int)name.size(); i++) {
        char ch = name[i];
        if (ch == 'k') { // king
            if (i == 0) {
                idxArr[k++] = (havingPawns ? chess_IDX_KK_2 : chess_IDX_KK_8) | (W << 8);
                assert(idxArr[k - 1] >> 8 == sd);
            } else {
                sd = B;
            }
            continue;
        }
        if (ch == 'p') {
            pawnCnt[sd]++;
        }

        const char* p = strchr(pieceTypeName, ch);
        int t = (int)(p - pieceTypeName);

        t += chess_IDX_Q - 1;

        for (int j = 0;; j++) {
            if (ch != name[i + 1]) {
                break;
            }
            i++;
            t += 5;
        }
        idxArr[k++] = t | (sd << 8);
    }

    bool enpassantable = pawnCnt[0] > 0 && pawnCnt[1] > 0;
    idxArr[k] = chess_IDX_NONE;

    // permutation
    if (order != 0) {
        const int orderArray[] = { order & 0x7, (order >> 3) & 0x7, (order >> 6) & 0x7, (order >> 9) & 0x7 , (order >> 12) & 0x7, (order >> 15) & 0x7 };
        int tmpArr[10];
        memcpy(tmpArr, idxArr, k * sizeof(int));
        for(int i = 0; i < k; i++) {
            int x = orderArray[i];
            idxArr[x] = tmpArr[i];
        }
        return parseAttr(idxArr, idxMult, pieceCount, (int*)orderArray, enpassantable);
    }

    return parseAttr(idxArr, idxMult, pieceCount, nullptr, enpassantable);
}

i64 chessFile::parseAttr(const int* idxArr, i64* idxMult, int* pieceCount, const int* orderArray, bool enpassantable) {
    memset(pieceCount, 0, 2 * 7 * sizeof(int));

    pieceCount[static_cast<int>(PieceType::king)] = 1;
    pieceCount[6 + static_cast<int>(PieceType::king)] = 1;

    i64 sz = 1;
    int n = 0;
    for (int i = 0; ; i++, n++) {
        auto a = idxArr[i];
        if (a == chess_IDX_NONE) {
            break;
        }

        i64 h = 0;
        int sd = a >> 8;
        int d = sd == W ? 6 : 0;

        a &= 0xff;
        switch (a) {
            case chess_IDX_K_2:
                h = chess_SIZE_K2;
                break;
            case chess_IDX_K_8:
                h = chess_SIZE_K8;
                break;
            case chess_IDX_K:
                h = chess_SIZE_K;
                break;

            case chess_IDX_KK_2:
                h = chess_SIZE_KK2;
                break;
            case chess_IDX_KK_8:
                h = chess_SIZE_KK8;
                break;

            case chess_IDX_Q:
            case chess_IDX_R:
            case chess_IDX_B:
            case chess_IDX_H:
            case chess_IDX_P:
            {
                h = a != chess_IDX_P ? chess_SIZE_X : chess_SIZE_P;
                int type = 1 + a - chess_IDX_Q;
                pieceCount[d + type] = 1;
                break;
            }

            case chess_IDX_QQ:
            case chess_IDX_RR:
            case chess_IDX_BB:
            case chess_IDX_HH:
            case chess_IDX_PP:
            {
                h = a != chess_IDX_PP ? chess_SIZE_XX : chess_SIZE_PP;
                int type = 1 + a - chess_IDX_QQ;
                pieceCount[d + type] = 2;
                break;
            }

            case chess_IDX_QQQ:
            case chess_IDX_RRR:
            case chess_IDX_BBB:
            case chess_IDX_HHH:
            case chess_IDX_PPP:
            {
                h = a != chess_IDX_PPP ? chess_SIZE_XXX : chess_SIZE_PPP;
                int type = 1 + a - chess_IDX_QQQ;
                pieceCount[d + type] = 3;
                break;
            }

            case chess_IDX_QQQQ:
            case chess_IDX_RRRR:
            case chess_IDX_BBBB:
            case chess_IDX_HHHH:
            case chess_IDX_PPPP:
            {
                h = a != chess_IDX_PPPP ? chess_SIZE_XXXX : chess_SIZE_PPPP;
                int type = 1 + a - chess_IDX_QQQQ;
                pieceCount[d + type] = 4;
                break;
            }

            default:
                assert(false);
                break;
        }

        idxMult[i] = h;
        sz *= h;
    }

    for(int i = 0; i < n; i++) {
        idxMult[i] = 1;
        for (int j = i + 1; j < n; j++) {
            idxMult[i] *= idxMult[j];
        }
    }

    return sz;
}

i64 chessFile::setupIdxComputing(const std::string& name, int order, int version)
{
    size = chessFile::parseAttr(name.c_str(), idxArr, idxMult, (int*)pieceCount, order, version);
    enpassantable = pieceCount[0][static_cast<int>(PieceType::pawn)] > 0 && pieceCount[1][static_cast<int>(PieceType::pawn)] > 0;
    return size;
}

i64 chessFile::computeSize(const std::string &name)
{
    int idxArr[32];
    i64 idxMult[32];
    int pieceCount[2][7];
    return parseAttr(name, idxArr, idxMult, (int*)pieceCount, 0, 3);
}

std::string chessFile::pieceListToName(const Piece* pieceList) {
    int pieceCnt[2][6];
    memset(pieceCnt, 0, sizeof(pieceCnt));

    for(int sd = 0, d = 0; sd < 2; sd++, d = 16) {
        for (int i = 0; i < 16; i++) {
            if (!pieceList[d + i].isEmpty()) {
                pieceCnt[sd][static_cast<int>(pieceList[d + i].type)]++;
            }
        }
    }

    std::string s;
    for(int sd = 1; sd >= 0; sd--) {
        for (int type = 0; type < 6; type++) {
            auto n = pieceCnt[sd][type];
            if (n) {
                char ch = pieceTypeName[type];
                for(int j = 0; j < n; j++) {
                    s += ch;
                }
            }
        }
    }

    return s;
}

chessKeyRec chessFile::getKey(const chessBoardCore& board) const {
    chessKeyRec rec;
    chessKey::getKey(rec, board, idxArr, idxMult, header ? header->order : 0);
    return rec;
}

extern const int tb_kIdxToPos[10];

bool chessFile::setupBoard(chessBoardCore& board, i64 idx, FlipMode flip, Side firstsider) const
{
    board.enpassant = -1;
    board._status = 0;
    board.castleRights[0] = board.castleRights[1] = 0;

    int order = header ? header->order : 0;
    if (!order) {
        order = 0 | 1 << 3 | 2 << 6 | 3 << 9 | 4 << 12 | 5 << 15;
    }
    const int orderArray[] = { order & 0x7, (order >> 3) & 0x7, (order >> 6) & 0x7, (order >> 9) & 0x7 , (order >> 12) & 0x7, (order >> 15) & 0x7 };

    chessBoardCore::pieceList_reset((Piece*)board.pieceList);

    i64 rest = idx;

    int sds[20] = { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 };

    for(int i = 0, sd = static_cast<int>(firstsider), stdSd = W; idxArr[i] != chess_IDX_NONE; i++) {
        int j = orderArray[i];
        if (idxArr[j] >> 8 != stdSd) {
            sd = 1 - sd;
            stdSd = 1 - stdSd;
        }
        sds[j] = sd;
    }

    for(int i = 0; idxArr[i] != chess_IDX_NONE; i++) {
        auto arr = idxArr[i] & 0xff;
        auto mul = idxMult[i];

        auto key = (int)(rest / mul);
        rest = rest % mul;

        int sd = sds[i];
        auto side = static_cast<Side>(sd);

        switch (arr) {
            case chess_IDX_K_2:
            {
                assert(key >= 0 && key < 32);
                int r = key >> 2, f = key & 0x3;
                auto pos = (r << 3) + f;
                board.pieceList[sd][0].type = PieceType::king;
                board.pieceList[sd][0].side = side;
                board.pieceList[sd][0].idx = pos;
                break;
            }

            case chess_IDX_K_8:
            {
                auto pos = tb_kIdxToPos[key];
                board.pieceList[sd][0].type = PieceType::king;
                board.pieceList[sd][0].side = side;
                board.pieceList[sd][0].idx = pos;
                break;
            }
            case chess_IDX_K:
            {
                board.pieceList[sd][0].type = PieceType::king;
                board.pieceList[sd][0].side = side;
                board.pieceList[sd][0].idx = key;
                break;
            }

            case chess_IDX_KK_2:
            {
                int kk = kk_2[key];
                int k0 = kk >> 8, k1 = kk & 0xff;
                board.pieceList[sd][0].type = PieceType::king;
                board.pieceList[sd][0].side = side;
                board.pieceList[sd][0].idx = k0;

                board.pieceList[1 - sd][0].type = PieceType::king;
                board.pieceList[1 - sd][0].side = getXSide(side);
                board.pieceList[1 - sd][0].idx = k1;
                break;
            }

            case chess_IDX_KK_8:
            {
                int kk = kk_8[key];
                int k0 = kk >> 8, k1 = kk & 0xff;
                board.pieceList[sd][0].type = PieceType::king;
                board.pieceList[sd][0].side = side;
                board.pieceList[sd][0].idx = k0;

                board.pieceList[1 - sd][0].type = PieceType::king;
                board.pieceList[1 - sd][0].side = getXSide(side);
                board.pieceList[1 - sd][0].idx = k1;
                break;
            }

            case chess_IDX_Q:
            case chess_IDX_R:
            case chess_IDX_B:
            case chess_IDX_H:
            case chess_IDX_P:
            {
                PieceType type = static_cast<PieceType>(arr - chess_IDX_Q + 1);
                if (!chessKey.setupBoard_x(board, key, type, side)) {
                    return false;
                }
                break;
            }

            case chess_IDX_QQ:
            case chess_IDX_RR:
            case chess_IDX_BB:
            case chess_IDX_HH:
            case chess_IDX_PP:
            {
                PieceType type = static_cast<PieceType>(arr - chess_IDX_QQ + 1);
                if (!chessKey.setupBoard_xx(board, key, type, side)) {
                    return false;
                }
                break;
            }

            case chess_IDX_QQQ:
            case chess_IDX_RRR:
            case chess_IDX_BBB:
            case chess_IDX_HHH:
            case chess_IDX_PPP:
            {
                PieceType type = static_cast<PieceType>(arr - chess_IDX_QQQ + 1);
                if (!chessKey.setupBoard_xxx(board, key, type, side)) {
                    return false;
                }
                break;
            }

            case chess_IDX_QQQQ:
            case chess_IDX_RRRR:
            case chess_IDX_BBBB:
            case chess_IDX_HHHH:
            case chess_IDX_PPPP:
            {
                PieceType type = static_cast<PieceType>(arr - chess_IDX_QQQQ + 1);
                if (!chessKey.setupBoard_xxxx(board, key, type, side)) {
                    return false;
                }
                break;
            }

            default:
                assert(false);
                break;
        }
    }

    return board.pieceList_setupBoard();
}

