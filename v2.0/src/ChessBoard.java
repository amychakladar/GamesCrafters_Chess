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

    public void PopMove() throws ChessException {
        if (unmoveStack.empty())
            throw new ChessException("PopMove: unmove stack is empty.");

        isWhiteTurn = !isWhiteTurn;
        Unmove unmove = unmoveStack.peek();
        unmoveStack.pop();
        square[unmove.move.source] = square[unmove.move.dest];
        square[unmove.move.dest] = unmove.capture;
        if (square[unmove.move.source] == Square.WhiteKing)
            wkpos = unmove.move.source;
        else if (square[unmove.move.source] == Square.BlackKing)
            bkpos = unmove.move.source;
    }

    public void GenMoves(MoveList movelist) {
        if (isWhiteTurn)
            GenWhiteMoves(movelist);
        else
            GenBlackMoves(movelist);
    }

    public boolean IsCurrentPlayerInCheck() {
        return isWhiteTurn ? IsAttackedByBlack(wkpos) : IsAttackedByWhite(bkpos);
    }

    public void GenWhiteMoves(MoveList movelist) {
        movelist.length = 0;
        for (char file = 'a'; file <= 'h'; ++file) {
            for (char rank = '1'; rank <= '8'; ++rank) {
                int source = Offset(file, rank);
                switch (square[source]) {
                    case WhiteKing:
                        TryWhiteMove(movelist, source, source + North);
                        TryWhiteMove(movelist, source, source + NorthEast);
                        TryWhiteMove(movelist, source, source + East);
                        TryWhiteMove(movelist, source, source + SouthEast);
                        TryWhiteMove(movelist, source, source + South);
                        TryWhiteMove(movelist, source, source + SouthWest);
                        TryWhiteMove(movelist, source, source + West);
                        TryWhiteMove(movelist, source, source + NorthWest);
                        break;
                    case WhiteQueen:
                        TryWhiteRay(movelist, source, North);
                        TryWhiteRay(movelist, source, NorthEast);
                        TryWhiteRay(movelist, source, East);
                        TryWhiteRay(movelist, source, SouthEast);
                        TryWhiteRay(movelist, source, South);
                        TryWhiteRay(movelist, source, SouthWest);
                        TryWhiteRay(movelist, source, West);
                        TryWhiteRay(movelist, source, NorthWest);
                        break;
                    case WhiteRook:
                        TryWhiteRay(movelist, source, North);
                        TryWhiteRay(movelist, source, East);
                        TryWhiteRay(movelist, source, South);
                        TryWhiteRay(movelist, source, West);
                        break;
                    case WhiteBishop:
                        TryWhiteRay(movelist, source, NorthEast);
                        TryWhiteRay(movelist, source, SouthEast);
                        TryWhiteRay(movelist, source, SouthWest);
                        TryWhiteRay(movelist, source, NorthWest);
                        break;
                    case WhiteKnight:
                        TryWhiteMove(movelist, source, source + KnightDir1);
                        TryWhiteMove(movelist, source, source + KnightDir2);
                        TryWhiteMove(movelist, source, source + KnightDir3);
                        TryWhiteMove(movelist, source, source + KnightDir4);
                        TryWhiteMove(movelist, source, source + KnightDir5);
                        TryWhiteMove(movelist, source, source + KnightDir6);
                        TryWhiteMove(movelist, source, source + KnightDir7);
                        TryWhiteMove(movelist, source, source + KnightDir8);
                        break;
                    case WhitePawn:
                        throw new ChessException("Pawn movement not yet implemented.");
                    default:
                        break;  // ignore anything but White's pieces.
                }
            }
        }
    }

    public void GenBlackMoves(MoveList movelist) {
        movelist.length = 0;
        for (char file = 'a'; file <= 'h'; ++file) {
            for (char rank = '1'; rank <= '8'; ++rank) {
                int source = Offset(file, rank);
                switch (square[source]) {
                    case BlackKing:
                        TryBlackMove(movelist, source, source + North);
                        TryBlackMove(movelist, source, source + NorthEast);
                        TryBlackMove(movelist, source, source + East);
                        TryBlackMove(movelist, source, source + SouthEast);
                        TryBlackMove(movelist, source, source + South);
                        TryBlackMove(movelist, source, source + SouthWest);
                        TryBlackMove(movelist, source, source + West);
                        TryBlackMove(movelist, source, source + NorthWest);
                        break;

                    case BlackQueen:
                        TryBlackRay(movelist, source, North);
                        TryBlackRay(movelist, source, NorthEast);
                        TryBlackRay(movelist, source, East);
                        TryBlackRay(movelist, source, SouthEast);
                        TryBlackRay(movelist, source, South);
                        TryBlackRay(movelist, source, SouthWest);
                        TryBlackRay(movelist, source, West);
                        TryBlackRay(movelist, source, NorthWest);
                        break;

                    case BlackRook:
                        TryBlackRay(movelist, source, North);
                        TryBlackRay(movelist, source, East);
                        TryBlackRay(movelist, source, South);
                        TryBlackRay(movelist, source, West);
                        break;

                    case BlackBishop:
                        TryBlackRay(movelist, source, NorthEast);
                        TryBlackRay(movelist, source, SouthEast);
                        TryBlackRay(movelist, source, SouthWest);
                        TryBlackRay(movelist, source, NorthWest);
                        break;

                    case BlackKnight:
                        TryBlackMove(movelist, source, source + KnightDir1);
                        TryBlackMove(movelist, source, source + KnightDir2);
                        TryBlackMove(movelist, source, source + KnightDir3);
                        TryBlackMove(movelist, source, source + KnightDir4);
                        TryBlackMove(movelist, source, source + KnightDir5);
                        TryBlackMove(movelist, source, source + KnightDir6);
                        TryBlackMove(movelist, source, source + KnightDir7);
                        TryBlackMove(movelist, source, source + KnightDir8);
                        break;

                    case BlackPawn:
                        throw new ChessException("Pawn movement not yet implemented.");

                    default:
                        break; // ignore anything but Black's pieces.
                }
            }
        }
    }

    public void tryWhiteMove(MoveList movelist, int source, int dest) throws ChessException {
        Side mover = squareSide(square[source]);
        if (mover != Side.White) {
            throw new ChessException("TryWhiteMove: Attempt to move non-White piece.");
        }

        Side capture = squareSide(square[dest]);
        if (capture == Side.Nobody || capture == Side.Black) {
            Move move = new Move(source, dest);
            pushMove(move);
            boolean selfCheck = isAttackedByBlack(wkpos);
            popMove();
            if (!selfCheck) {
                movelist.add(move);
            }
        }
    }

    public void tryBlackMove(MoveList movelist, int source, int dest) throws ChessException {
        Side mover = squareSide(square[source]);
        if (mover != Side.Black) {
            throw new ChessException("TryBlackMove: Attempt to move non-Black piece.");
        }

        Side capture = squareSide(square[dest]);
        if (capture == Side.Nobody || capture == Side.White) {
            Move move = new Move(source, dest);
            pushMove(move);
            boolean selfCheck = isAttackedByWhite(bkpos);
            popMove();
            if (!selfCheck) {
                movelist.add(move);
            }
        }
    }

    public void tryWhiteRay(MoveList movelist, int source, int dir) {
        int dest;
        for (dest = source + dir; square[dest] == Square.Empty; dest += dir) {
            tryWhiteMove(movelist, source, dest);
        }

        if (squareSide(square[dest]) == Side.Black) {
            tryWhiteMove(movelist, source, dest);
        }
    }

    public void tryBlackRay(MoveList movelist, int source, int dir) {
        int dest;
        for (dest = source + dir; square[dest] == Square.Empty; dest += dir) {
            tryBlackMove(movelist, source, dest);
        }

        if (squareSide(square[dest]) == Side.White) {
            tryBlackMove(movelist, source, dest);
        }
    }
    public boolean isAttackedByWhite(int offset) {
        if (square[offset + North] == WhiteKing) return true;
        if (square[offset + NorthEast] == WhiteKing) return true;
        if (square[offset + East] == WhiteKing) return true;
        if (square[offset + SouthEast] == WhiteKing) return true;
        if (square[offset + South] == WhiteKing) return true;
        if (square[offset + SouthWest] == WhiteKing) return true;
        if (square[offset + West] == WhiteKing) return true;
        if (square[offset + NorthWest] == WhiteKing) return true;

        if (square[offset + KnightDir1] == WhiteKnight) return true;
        if (square[offset + KnightDir2] == WhiteKnight) return true;
        if (square[offset + KnightDir3] == WhiteKnight) return true;
        if (square[offset + KnightDir4] == WhiteKnight) return true;
        if (square[offset + KnightDir5] == WhiteKnight) return true;
        if (square[offset + KnightDir6] == WhiteKnight) return true;
        if (square[offset + KnightDir7] == WhiteKnight) return true;
        if (square[offset + KnightDir8] == WhiteKnight) return true;

        if (square[offset + SouthEast] == WhitePawn) return true;
        if (square[offset + SouthWest] == WhitePawn) return true;

        if (isAttackedRay(offset, North, WhiteRook, WhiteQueen)) return true;
        if (isAttackedRay(offset, East, WhiteRook, WhiteQueen)) return true;
        if (isAttackedRay(offset, South, WhiteRook, WhiteQueen)) return true;
        if (isAttackedRay(offset, West, WhiteRook, WhiteQueen)) return true;

        if (isAttackedRay(offset, NorthEast, WhiteBishop, WhiteQueen)) return true;
        if (isAttackedRay(offset, SouthEast, WhiteBishop, WhiteQueen)) return true;
        if (isAttackedRay(offset, SouthWest, WhiteBishop, WhiteQueen)) return true;
        if (isAttackedRay(offset, NorthWest, WhiteBishop, WhiteQueen)) return true;

        return false;
    }
    public boolean isAttackedByBlack(int offset) {
        if (square[offset + North] == BlackKing) return true;
        if (square[offset + NorthEast] == BlackKing) return true;
        if (square[offset + East] == BlackKing) return true;
        if (square[offset + SouthEast] == BlackKing) return true;
        if (square[offset + South] == BlackKing) return true;
        if (square[offset + SouthWest] == BlackKing) return true;
        if (square[offset + West] == BlackKing) return true;
        if (square[offset + NorthWest] == BlackKing) return true;

        if (square[offset + KnightDir1] == BlackKnight) return true;
        if (square[offset + KnightDir2] == BlackKnight) return true;
        if (square[offset + KnightDir3] == BlackKnight) return true;
        if (square[offset + KnightDir4] == BlackKnight) return true;
        if (square[offset + KnightDir5] == BlackKnight) return true;
        if (square[offset + KnightDir6] == BlackKnight) return true;
        if (square[offset + KnightDir7] == BlackKnight) return true;
        if (square[offset + KnightDir8] == BlackKnight) return true;

        if (square[offset + NorthEast] == BlackPawn) return true;
        if (square[offset + NorthWest] == BlackPawn) return true;

        if (isAttackedRay(offset, North, BlackRook, BlackQueen)) return true;
        if (isAttackedRay(offset, East, BlackRook, BlackQueen)) return true;
        if (isAttackedRay(offset, South, BlackRook, BlackQueen)) return true;
        if (isAttackedRay(offset, West, BlackRook, BlackQueen)) return true;

        if (isAttackedRay(offset, NorthEast, BlackBishop, BlackQueen)) return true;
        if (isAttackedRay(offset, SouthEast, BlackBishop, BlackQueen)) return true;
        if (isAttackedRay(offset, SouthWest, BlackBishop, BlackQueen)) return true;
        if (isAttackedRay(offset, NorthWest, BlackBishop, BlackQueen)) return true;

        return false;
    }

    public boolean IsAttackedRay(int source, int dir, Square piece1, Square piece2) {
        int dest;
        for (dest = source + dir; square[dest] == Empty; dest += dir) {
            // do nothing
        }

        return square[dest] == piece1 || square[dest] == piece2;
    }

    public boolean IsLegalPosition() {
        // Can't move if you're in check!
        return isWhiteTurn ? !IsAttackedByWhite(bkpos) : !IsAttackedByBlack(wkpos);
    }

}

