public class ChessBoard {

    private final Square[] square = new Square[120];
    private boolean isWhiteTurn;
    private int wkpos, bkpos;
    private final Stack<Unmove> unmoveStack = new Stack<>();

    public ChessBoard() {
        Clear(true);
    }

    public void Clear(boolean whiteToMove) {
        for (int x = 0; x < 10; ++x) {
            for (int y = 0; y < 12; ++y) {
                square[10 * y + x] = (x >= 1 && x <= 8 && y >= 2 && y <= 9) ? Square.Empty : Square.OffBoard;
            }
        }

        wkpos = Offset('e', '1');
        square[wkpos] = Square.WhiteKing;

        bkpos = Offset('e', '8');
        square[bkpos] = Square.BlackKing;

        isWhiteTurn = whiteToMove;
        unmoveStack.clear();
    }

    public Square GetSquare(int offset) throws ChessException {
        if (offset < 0 || offset >= 120 || square[offset] == Square.OffBoard) {
            throw new ChessException("ChessBoard::GetSquare: invalid offset");
        }

        return square[offset];
    }

    public void SetSquare(int offset, Square value) throws ChessException {
        ValidateOffset(offset);
        if (SquareSide(value) == Side.Invalid) {
            throw new ChessException("SetSquare: invalid square value");
        }

        if (value == Square.WhiteKing) {
            // There must be exactly one White King on the board.
            square[wkpos] = Square.Empty;
            wkpos = offset;
        } else if (value == Square.BlackKing) {
            // There must be exactly one Black King on the board.
            square[bkpos] = Square.Empty;
            bkpos = offset;
        }

        square[offset] = value;

        if (square[wkpos] != Square.WhiteKing) {
            throw new ChessException("White King is missing");
        }

        if (square[bkpos] != Square.BlackKing) {
            throw new ChessException("Black King is missing");
        }
    }

    public void PushMove(Move move) throws ChessException {
        Square mover = square[move.source];
        Square capture = square[move.dest];

        if (isWhiteTurn) {
            if (SquareSide(mover) != Side.White) {
                throw new ChessException("PushMove: attempt to move non-White piece");
            }
            if (mover == Square.WhiteKing) {
                wkpos = move.dest;
            }
        } else {
            if (SquareSide(mover) != Side.Black) {
                throw new ChessException("PushMove: attempt to move non-Black piece");
            }
            if (mover == Square.BlackKing) {
                bkpos = move.dest;
            }
        }

        unmoveStack.push(new Unmove(move, capture));
        square[move.dest] = mover;
        square[move.source] = Square.Empty;
        isWhiteTurn = !isWhiteTurn;
    }


