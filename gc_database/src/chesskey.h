#ifndef chessKey_h
#define chessKey_h

#include <map>

#include "chess.h"

namespace chess {

    class chessKeyRec {
    public:
        i64 key;
        bool flipSide;
    };

    class chessKey {
    public:
        chessKey();

        static void getKey(chessKeyRec& rec, const chessBoardCore& board, const int* idxArr, const i64* idxMult, u32 order);

        bool setupBoard_x(chessBoardCore& board, int key, PieceType type, Side side) const;
        bool setupBoard_xx(chessBoardCore& board, int key, PieceType type, Side side) const;
        bool setupBoard_xxx(chessBoardCore& board, int key, PieceType type, Side side) const;
        bool setupBoard_xxxx(chessBoardCore& board, int key, PieceType type, Side side) const;

    private:
        static int getKey_x(int pos0);
        static int getKey_xx(int p0, int p1);
        static int getKey_xxx(int p0, int p1, int p2);
        static int getKey_xxxx(int p0, int p1, int p2, int p3);

        static int getKey_p(int p0);
        static int getKey_pp(int p0, int p1);
        static int getKey_ppp(int p0, int p1, int p2);
        static int getKey_pppp(int p0, int p1, int p2, int p3);

        void initOnce();

        void createXXKeys();
        void createKingKeys();

    private:

    };

    extern chessKey chessKey;

} // namespace chess

#endif /* TbKey_hpp */

