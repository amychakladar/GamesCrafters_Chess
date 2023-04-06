import java.util.ArrayList;
import java.util.List;
import java.util.Stack;
import java.util.Stack;
import java.util.Vector;

public class Chess {
    public static class ChessException extends Exception {
        private static final long serialVersionUID = 1L;

        public ChessException(String message) {
            super(message);
        }
    }

    public enum Square {
        Empty, OffBoard,

        WhitePawn, WhiteKnight, WhiteBishop, WhiteRook, WhiteQueen, WhiteKing,

        BlackPawn, BlackKnight, BlackBishop, BlackRook, BlackQueen, BlackKing,
    }

    public static class Move {
        public int source;
        public int dest;
        public short score;

        public Move() {
            this.score = Unscored;
        }

        public Move(short score) {
            this.score = score;
        }

        public Move(int source, int dest) throws ChessException {
            this.source = ValidateOffset(source);
            this.dest = ValidateOffset(dest);
            this.score = Unscored;
        }

        public String Algebraic() throws ChessException {
            return File(source) + "" + Rank(source) + "" + File(dest) + "" + Rank(dest);
        }
    }

    public static class Unmove {
        public Move move;
        public Square capture;

        public Unmove(Move move, Square capture) {
            this.move = move;
            this.capture = capture;
        }
    }

    public static class MoveList {
        public int length;
        public List<Move> movelist;

        public MoveList() {
            this.movelist = new ArrayList<Move>();
            this.length = 0;
        }

        public void Add(Move move) throws ChessException {
            if (length >= MaxMoves) {
                throw new ChessException("MoveList overflow");
            }
            movelist.add(move);
            length++;
        }
    }

    public enum Side {
        Invalid, Nobody, White, Black,
    }

    private static final int North = +10;
    private static final int NorthEast = +11;
    private static final int East = +1;
    private static final int SouthEast = -9;
    private static final int South = -10;
    private static final int SouthWest = -11;
    private static final int West = -1;
    private static final int NorthWest = +9;

    private static final int KnightDir1 = (2 * North + East);
    private static final int KnightDir2 = (2 * North + West);
    private static final int KnightDir3 = (2 * South + East);
    private static final int KnightDir4 = (2 * South + West);
    private static final int KnightDir5 = (2 * East + North);
    private static final int KnightDir6 = (2 * East + South);
    private static final int KnightDir7 = (2 * West + North);
    private static final int KnightDir8 = (2 * West + South);

    private static final short Unscored = -2000;
    private static final short BlackMates = -1000;
    private static final short WhiteMates = +1000;
    private static final short PosInf = +2000;
    private static final short Draw = 0;

    private static final int MaxMoves = 255;

    public class ChessBoard {
        private Square[] square = new Square[120]; // 8x8 chess board embedded in a 10x12 buffer.
        private int wkpos; // White King's position (index into 'square')
        private int bkpos; // Black King's position (index into 'square')
        private boolean isWhiteTurn; // is it White's turn to move?
        private Stack<Unmove> unmoveStack;

        public ChessBoard() {
            clear(true);
        }

        public void clear(boolean whiteToMove) {
            for (int i = 0; i < 120; i++) {
                if ((i < 21) || (i > 98) || (i % 10 == 0) || (i % 10 == 9)) {
                    square[i] = Square.OffBoard;
                } else {
                    square[i] = Square.Empty;
                }
            }
            wkpos = Offset('e', '1');
            bkpos = Offset('e', '8');
            isWhiteTurn = whiteToMove;
            unmoveStack = new Stack<Unmove>();
        }

        public boolean isWhiteTurn() {
            return isWhiteTurn;
        }

        public void genMoves(MoveList movelist) {
            if (isWhiteTurn) {
                genWhiteMoves(movelist);
            } else {
                genBlackMoves(movelist);
            }
        }

        public void pushMove(Move move) {
            Square capture = square[move.dest];
            Unmove unmove = new Unmove(move, capture);
            unmoveStack.push(unmove);
            square[move.dest] = square[move.source];
            square[move.source] = Square.Empty;
            if (square[move.dest] == Square.WhiteKing) {
                wkpos = move.dest;
            } else if (square[move.dest] == Square.BlackKing) {
                bkpos = move.dest;
            }
            isWhiteTurn = !isWhiteTurn;
        }

        public void popMove() {
            Unmove unmove = unmoveStack.pop();
            square[unmove.move.source] = square[unmove.move.dest];
            square[unmove.move.dest] = unmove.capture;
            if (square[unmove.move.source] == Square.WhiteKing) {
                wkpos = unmove.move.source;
            } else if (square[unmove.move.source] == Square.BlackKing) {
                bkpos = unmove.move.source;
            }
            isWhiteTurn = !isWhiteTurn;
        }

        public Square getSquare(int offset) {
            return square[offset];
        }

        public void setTurn(boolean whiteToMove) {
            isWhiteTurn = whiteToMove;
        }

        public void setSquare(int offset, Square value) {
            square[offset] = value;
        }

        public boolean isLegalPosition() {
            return (!isCurrentPlayerInCheck() && !isNextPlayerInCheck());
        }

        public boolean isCurrentPlayerInCheck() {
            if (isWhiteTurn) {
                return isAttackedByBlack(wkpos);
            } else {
                return isAttackedByWhite(bkpos);
            }
        }

        private void genWhiteMoves(MoveList movelist) {
            for (int i = 21; i <= 98; i++) {
                if (SquareSide(square[i]) != Side.White) {
                    continue;
                }
                switch (square[i]) {
                    case WhitePawn:
                        if (square[i + North] == Square.Empty) {
                            TryWhiteMove(movelist,



