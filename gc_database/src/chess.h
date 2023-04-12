#ifndef chess_h
#define chess_h


#include <iostream>
#include <sstream>
#include <cstdarg>
#include <algorithm>
#include <vector>
#include <assert.h>

namespace chess {

    const int CHESS_VERSION = 0x001;

    enum {
        chess_IDX_K_8,
        chess_IDX_K_2,
        chess_IDX_K,
        chess_IDX_KK_8,
        chess_IDX_KK_2,

        chess_IDX_Q = 16,
        chess_IDX_R,
        chess_IDX_B,
        chess_IDX_H,
        chess_IDX_P,

        chess_IDX_QQ,
        chess_IDX_RR,
        chess_IDX_BB,
        chess_IDX_HH,
        chess_IDX_PP,

        chess_IDX_QQQ,
        chess_IDX_RRR,
        chess_IDX_BBB,
        chess_IDX_HHH,
        chess_IDX_PPP,

        chess_IDX_QQQQ,
        chess_IDX_RRRR,
        chess_IDX_BBBB,
        chess_IDX_HHHH,
        chess_IDX_PPPP,


        chess_IDX_LAST = chess_IDX_PPPP,

        chess_IDX_NONE = 254
    };


#define MIN(a, b) (((a) <= (b)) ? (a) : (b))
#define MAX(a, b) (((a) >= (b)) ? (a) : (b))

#define chess_SCORE_DRAW         0

#define chess_SCORE_MATE         1000

#define chess_SCORE_WINNING      1003

#define chess_SCORE_ILLEGAL      1004
#define chess_SCORE_UNKNOWN      1005
#define chess_SCORE_MISSING      1006
#define chess_SCORE_UNSET        1007


    ////////////////////////////////////
#define chess_SIZE_K2            32
#define chess_SIZE_K8            10
#define chess_SIZE_K             64

#define chess_SIZE_KK8           564
#define chess_SIZE_KK2           1806

#define chess_SIZE_X             64
#define chess_SIZE_XX            2016
#define chess_SIZE_XXX           41664
#define chess_SIZE_XXXX          635376

#define chess_SIZE_P             48
#define chess_SIZE_PP            1128
#define chess_SIZE_PPP           17296
#define chess_SIZE_PPPP          194580


#define chess_ID_MAIN_V0                 23456

#define chess_SIZE_COMPRESS_BLOCK        (4 * 1024)
#define chess_PROP_COMPRESSED            (1 << 2)
#define chess_PROP_SPECIAL_SCORE_RANGE   (1 << 3)

#define chess_HEADER_SIZE                128

#define DARK                            8
#define LIGHT                           16

#define B                               0
#define W                               1

#define chess_SMART_MODE_THRESHOLD       10L * 1024 * 1024L

    const int chess_UNCOMPRESS_BIT       = 1 << 31;

    enum class Side {
        black = 0, white = 1, none = 2, offboard = 3
    };

    enum class PieceType {
        king, queen, rook, bishop, knight, pawn, empty, offboard
    };

    enum class GameResultType { // Based on white side
        win,    // white wins
        loss,   // white loses
        draw,
        unknown
    };

    enum Squares {
        A8, B8, C8, D8, E8, F8, G8, H8,
        A7, B7, C7, D7, E7, F7, G7, H7,
        A6, B6, C6, D6, E6, F6, G6, H6,
        A5, B5, C5, D5, E5, F5, G5, H5,
        A4, B4, C4, D4, E4, F4, G4, H4,
        A3, B3, C3, D3, E3, F3, G3, H3,
        A2, B2, C2, D2, E2, F2, G2, H2,
        A1, B1, C1, D1, E1, F1, G1, H1,
        NoSquare
    };


#define i16 int16_t
#define u16 uint16_t
#define i32 int32_t
#define u32 uint32_t
#define u8  uint8_t
#define u64 uint64_t
#define i64 int64_t

    enum class FlipMode {
        none, horizontal, vertical, flipVH, flipHV, rotate90, rotate180, rotate270
    };


#define isPosValid(pos) ((pos)>=0 && (pos)<90)

#define getXSide(side) ((side)==Side::white ? Side::black : Side::white)

#define COL(pos) ((pos)&7)
#define ROW(pos) ((pos)>>3)

#define sider(side) (static_cast<int>(side))

#define CASTLERIGHT_LONG        (1<<0)
#define CASTLERIGHT_SHORT       (1<<1)

#define CASTLERIGHT_MASK        (CASTLERIGHT_LONG|CASTLERIGHT_SHORT)

#define Status_incheck      (1<<4)
#define Status_notincheck   (1<<5)

    enum chessMemMode {
        tiny,          // load minimum to memory
        all,            // load all data into memory, no access hard disk after loading
        smart           // depend on data size, load as small or all mode
    };

    enum chessLoadMode {
        loadnow,
        onrequest
    };

    enum chessLoadStatus {
        none, loaded, error
    };

    void toLower(std::string& str);
    void toLower(char* str);
    std::string posToCoordinateString(int pos);
    std::string getFileName(const std::string& path);
    std::string getVersion();
    std::vector<std::string> listdir2(std::string dirname);

    int decompress(char *dst, int uncompresslen, const char *src, int slen);
    i64 decompressAllBlocks(int blocksize, int blocknum, u32* blocktable, char *dest, i64 uncompressedlen, const char *src, i64 slen);

    // set it to true if you want to print out more messages
    extern bool chessVerbose;

    class Piece;
    class Move;
    class MoveList;
    class chessFile;
    class chessDb;
    class chessBoardCore;
    class chessMailBoard;
    class chessKeyRec;
    class chessKey;

} // namespace chess

#include "chessBoard.h"
#include "chessFile.h"
#include "chessDb.h"
#include "chessKey.h"


#endif

