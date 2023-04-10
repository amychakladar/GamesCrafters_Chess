#include <iostream>

/*
 * Need to add only one .h file
 */
#include "chess.h"

/*
 * The EGT should declare only one since it may take time to load data and some memory
 */
chess::chessDb chessDb;

// Utility function
std::string explainScore(int score);

int main() {
    std::cout << "Welcome to NhatMinh Chess Endgame databases - version: " << chess::getVersion() << std::endl;

    /*
     * Allow chess to print out more information
     */
    chess::chessVerbose = true;

    /*
     * Preload endgames into memory
     * Depend on memory mode, it could load partly or whole data into memory
     * If this process of loading is slow (when you load all into memory for huge endgames),
     * you may use a background thread to load it and use when data is ready
     */
    // Note: this project has been attached already endgames 3-4 men. For 5 men you need to download separately
    const char* chessDataFolder = "./chess";

    // You may add folders one by one
    chessDb.addFolders(chessDataFolder);

    // Data can be loaded all into memory or tiny or smart (let programm decide between all-tiny)
    chess::chessMemMode chessMemMode = chess::chessMemMode::all;
    // Data can be loaded right now or don't load anything until the first request
    chess::chessLoadMode loadMode = chess::chessLoadMode::onrequest;

    chessDb.preload(chessMemMode, loadMode);

    // The numbers of endgames should not be zero
    if (chessDb.getSize() == 0) {
        std::cerr << "Error: chess could not load any endgames from folder " << chessDataFolder << ". 3 + 4 chess should have totally 35 endgames. Please check!" << std::endl;
        return -1;
    }

    std::cout << "chess database size: " << chessDb.getSize() << std::endl << std::endl;

    /*
     * Query scores
     * To enter a chess board to query, you may use a fen strings or an array of pieces, each piece has piece type, side and position
     * You may put input (fen string or vector of pieces) directly to chess or via internal board of chess
     *
     * WARNING: the chess board must be valid, otherwise the score may have no meaning (just a random number)
     */

    // Query with internal board of chess
    // The advantages of using that board you may quickly show the board or check the valid
    chess::chessBoard board;

    board.setFen(""); // default starting board
    board.show();

    auto score = chessDb.getScore(board);
    std::cout << "Query the starting board, score: " << score << ", explaination: " << explainScore(score) << std::endl << std::endl;

    board.setFen("K2k4/2p5/8/8/8/8/8/8 w - - 0 1");
    board.show();

    score = chessDb.getScore(board);
    std::cout << "Query with a fen string, score: " << score << ", explaination: " << explainScore(score) << std::endl << std::endl;

    // Use a vector of pieces
    /*
     Squares is defined in chess.h as below:
     enum Squares {
     A8, B8, C8, D8, E8, F8, G8, H8,
     A7, B7, C7, D7, E7, F7, G7, H7,
     A6, B6, C6, D6, E6, F6, G6, H6,
     A5, B5, C5, D5, E5, F5, G5, H5,
     A4, B4, C4, D4, E4, F4, G4, H4,
     A3, B3, C3, D3, E3, F3, G3, H3,
     A2, B2, C2, D2, E2, F2, G2, H2,
     A1, B1, C1, D1, E1, F1, G1, H1
     };
     */

    std::vector<chess::Piece> pieces;
    pieces.push_back(chess::Piece(chess::PieceType::king, chess::Side::white, chess::Squares::B3));
    pieces.push_back(chess::Piece(chess::PieceType::rook, chess::Side::white, chess::Squares::A5));

    pieces.push_back(chess::Piece(chess::PieceType::king, chess::Side::black, chess::Squares::G8));
    pieces.push_back(chess::Piece(chess::PieceType::queen, chess::Side::black, chess::Squares::H1));

    if (board.setup(pieces, chess::Side::white) && board.isValid()) { // vector of pieces and side to move
        board.show();
        auto score = chessDb.getScore(board);
        std::cout << "Query with a vector of pieces, score: " << score << ", explaination: " << explainScore(score) << std::endl << std::endl;
    } else {
        std::cerr << "Error on board setup" << std::endl;
    }

    // Not use internal board:
    score = chessDb.getScore(pieces, chess::Side::black);
    std::cout << "Query directly (not using internal board) with a vector of pieces, score: " << score << ", explaination: " << explainScore(score) << std::endl << std::endl;

    /*
     * Probe a position via a vector of pieces
     * Different from getScore, probe will return a list of moves which lead to the mate
     */
    chess::MoveList moveList;
    score = chessDb.probe(board, moveList);
    std::cout << "Probe directly with a vector of pieces, score: " << score << ", explaination: " << explainScore(score) << std::endl;
    std::cout << "moves to mate: " << moveList.toString() << std::endl << std::endl;


    return 0;
}

std::string explainScore(int score) {
    std::string str = "";

    switch (score) {
        case chess_SCORE_DRAW:
            str = "draw";
            break;

        case chess_SCORE_MISSING:
            str = "missing (board is incorrect or missing some endgame databases)";
            break;
        case chess_SCORE_MATE:
            str = "mate";
            break;

        case chess_SCORE_WINNING:
            str = "winning";
            break;

        case chess_SCORE_ILLEGAL:
            str = "illegal";
            break;

        case chess_SCORE_UNKNOWN:
            str = "unknown";
            break;

        default: {
            auto mateInPly = chess_SCORE_MATE - abs(score);
            int mateIn = (mateInPly + 1) / 2; // devide 2 for full moves
            if (score < 0) mateIn = -mateIn;

            std::ostringstream stringStream;
            stringStream << "mate in " << mateIn << " (" << mateInPly << " " << (mateInPly <= 1 ? "ply" : "plies") << ")";
            str = stringStream.str();
            break;
        }
    }
    return str;
}

