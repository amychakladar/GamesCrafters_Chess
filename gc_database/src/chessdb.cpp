#include "chess.h"
#include "chessDb.h"
#include "chessKey.h"

using namespace chess;

chessDb::chessDb() {
}

chessDb::~chessDb() {
    closeAll();
}

void chessDb::closeAll() {
    for (auto && chessFile : chessFileVec) {
        delete chessFile;
    }
    folders.clear();
    chessFileVec.clear();
    nameMap.clear();
}

void chessDb::removeAllBuffers() {
    for (auto && chessFile : chessFileVec) {
        chessFile->removeBuffers();
    }
}

void chessDb::setFolders(const std::vector<std::string>& folders_) {
    folders.clear();
    folders.insert(folders_.end(), folders_.begin(), folders_.end());
}

void chessDb::addFolders(const std::string& folderName) {
    folders.push_back(folderName);
}

chessFile* chessDb::getchessFile(const std::string& name) {
    return nameMap[name];
}

void chessDb::preload(const std::string& folder, chessMemMode chessMemMode, chessLoadMode loadMode) {
    addFolders(folder);
    preload(chessMemMode, loadMode);
}

void chessDb::preload(chessMemMode chessMemMode, chessLoadMode loadMode) {
    for (auto && folderName : folders) {
        auto vec = listdir(folderName);

        for (auto && path : vec) {
            if (chessFile::knownExtension(path)) {
                chessFile *chessFile = new chessFile();
                if (chessFile->preload(path, chessMemMode, loadMode)) {
                    auto pos = nameMap.find(chessFile->getName());
                    if (pos == nameMap.end()) {
                        addchessFile(chessFile);
                        continue;
                    }
                    pos->second->merge(*chessFile);
                } else {
                    std::cout << "Error: not loaded: " << path << std::endl;
                }
                free(chessFile);
            }
        }
    }
}

void chessDb::addchessFile(chessFile *chessFile) {
    chessFileVec.push_back(chessFile);

    auto s = chessFile->getName();
    nameMap[s] = chessFile;
    auto p = s.find_last_of("k");
    auto s0 = s.substr(0, p);
    auto s1 = s.substr(p);
    s = s1 + s0;
    nameMap[s] = chessFile;
}

////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////

int chessDb::getScore(const std::vector<Piece> pieceVec, Side side) {
    chessBoard board;
    board.setup(pieceVec, side);
    return getScore(board, board.side);
}

int chessDb::getScore(chessBoardCore& board) {
    return getScore(board, board.side);
}

int chessDb::getScore(chessBoardCore& board, Side side) {
    assert(side == Side::white || side == Side::black);

    chessFile* pchessFile = getchessFile(board);
    if (pchessFile == nullptr || pchessFile->loadStatus == chessLoadStatus::error) {
        return chess_SCORE_MISSING;
    }

    pchessFile->checkToLoadHeaderAndTable();
    auto r = pchessFile->getKey(board);
    auto querySide = r.flipSide ? getXSide(side) : side;

    if (pchessFile->header->isSide(querySide) && board.enpassant <= 0) {
        int score = pchessFile->getScore(r.key, querySide);
        return score;
    }

    return getScoreOnePly(board, side);
}

int chessDb::getScoreOnePly(chessBoardCore& board, Side side) {

    auto xside = getXSide(side);

    MoveList moveList;
    Hist hist;
    board.gen(moveList, side, false);
    int bestscore = -chess_SCORE_MATE, legalCnt = 0;

    for(int i = 0; i < moveList.end; i++) {
        auto move = moveList.list[i];
        board.make(move, hist);

        if (!board.isIncheck(side)) {
            legalCnt++;
            auto score = getScore(board, xside);

            if (score == chess_SCORE_MISSING && !hist.cap.isEmpty() && board.pieceList_isDraw()) {
                score = chess_SCORE_DRAW;
            }

            if (abs(score) <= chess_SCORE_MATE) {
                bestscore = MAX(bestscore, -score);
            }
        }
        board.takeBack(hist);
    }

    if (legalCnt) {
        if (abs(bestscore) <= chess_SCORE_MATE && bestscore != chess_SCORE_DRAW) {
            bestscore += bestscore > 0 ? -1 : +1;
        }
        return bestscore;
    }

    return board.isIncheck(side) ? -chess_SCORE_MATE : chess_SCORE_DRAW;
}

chessFile* chessDb::getchessFile(const chessBoardCore& board) const {
    auto name = chessFile::pieceListToName((const Piece* )board.pieceList);
    return nameMap.find(name) != nameMap.end() ? nameMap.at(name) : nullptr;
}

int chessDb::probe(const std::vector<Piece> pieceVec, Side side, MoveList& moveList) {
    chessBoard board;
    board.setup(pieceVec, side);
    return probe(board, moveList);
}

int chessDb::probe(const char* fenString, MoveList& moveList) {
    chessBoard board;
    board.setFen(fenString);
    return probe(board, moveList);
}

int chessDb::probe(chessBoardCore& board, MoveList& moveList) {
    auto side = board.side;
    auto xside = getXSide(board.side);
    int bestScore = -chess_SCORE_MATE, legalMoveCnt = 0;
    bool cont = true;
    Move bestMove(Side::none, -1, -1);

    MoveList mList;
    board.gen(mList, side, false);

    for(int i = 0; i < mList.end && cont; i++) {
        auto move = mList.list[i];
        Hist hist;
        board.make(move, hist);
        board.side = xside;

        if (!board.isIncheck(side)) {

            int score = getScore(board);

            if (score == chess_SCORE_MISSING) {
                if (!hist.cap.isEmpty() && board.pieceList_isDraw()) {
                    score = chess_SCORE_DRAW;
                } else {
                    if (chessVerbose) {
                        std::cerr << "Error: missing or broken data when probing:" << std::endl;
                        board.show();
                    }
                    return chess_SCORE_MISSING;
                }
            }
            if (score <= chess_SCORE_MATE) {
                legalMoveCnt++;
                score = -score;

                if (score > bestScore) {
                    bestMove = move;
                    bestScore = score;

                    if (score == chess_SCORE_MATE) {
                        cont = false;
                    }
                }
            }
        }

        board.takeBack(hist);
        board.side = side;
    }

    if (!legalMoveCnt) {
        return board.isIncheck(side) ? -chess_SCORE_MATE : chess_SCORE_DRAW;
    }

    if (bestScore != chess_SCORE_DRAW && bestScore < abs(chess_SCORE_MATE)) {
        bestScore += bestScore > 0 ? -1 : 1;
    }

    if (bestMove.isValid()) {
        moveList.add(bestMove);

        if (abs(bestScore) != chess_SCORE_MATE && bestScore != chess_SCORE_DRAW) {
            Hist hist;
            board.make(bestMove, hist);
            auto side = board.side;
            board.side = getXSide(side);

            probe(board, moveList);
            board.takeBack(hist);
            board.side = side;
        }
    }
    return bestScore;
}

