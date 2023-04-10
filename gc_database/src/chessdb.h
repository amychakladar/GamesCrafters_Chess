#ifndef chessDb_h
#define chessDb_h

#include <vector>
#include <map>
#include <string>

#include "chess.h"
#include "chessFile.h"
#include "chessBoard.h"

namespace chess {

    class chessDb {
    protected:
        std::vector<std::string> folders;
        std::map<std::string, chessFile*> nameMap;

    public:
        std::vector<chessFile*> chessFileVec;

    public:
        chessDb();
        ~chessDb();

        // Call it to release memory
        void removeAllBuffers();

        int getSize() const {
            return (int)chessFileVec.size();
        }

        // Folders and files
        void setFolders(const std::vector<std::string>& folders);
        void addFolders(const std::string& folderName);

        void preload(chessMemMode chessMemMode = chessMemMode::tiny, chessLoadMode loadMode = chessLoadMode::onrequest);
        void preload(const std::string& folder, chessMemMode chessMemMode, chessLoadMode loadMode = chessLoadMode::onrequest);

        // Scores
        int getScore(chessBoardCore& board, Side side);
        int getScore(chessBoardCore& board);
        int getScore(const std::vector<Piece> pieceVec, Side side);

        // Probe (for getting the line of moves to win
        int probe(chessBoardCore& board, MoveList& moveList);
        int probe(const std::vector<Piece> pieceVec, Side side, MoveList& moveList);
        int probe(const char* fenString, MoveList& moveList);

    public:
        chessFile* getchessFile(const std::string& name);
        virtual chessFile* getchessFile(const chessBoardCore& board) const;

        void closeAll();

    private:
        void addchessFile(chessFile *chessFile);

        int getScoreOnePly(chessBoardCore& board, Side side);

    };

} //namespace chess

#endif /* chessFileMng_hpp */

