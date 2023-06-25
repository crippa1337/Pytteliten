#include <algorithm>
#include <chrono>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

// minify enable filter delete
#include <cassert>
#include <cctype>
// minify disable filter delete

using namespace std;
vector<string> split(const string &str, char delim) {
    vector<string> result{};

    istringstream stream{str};

    for (string token{}; getline(stream, token, delim);) {
        if (token.empty())
            continue;

        result.push_back(token);
    }

    return result;
}

uint64_t MaskDiagonal[]{
    0x80,
    0x8040,
    0x804020,
    0x80402010,
    0x8040201008,
    0x804020100804,
    0x80402010080402,
    0x8040201008040201,
    0x4020100804020100,
    0x2010080402010000,
    0x1008040201000000,
    0x804020100000000,
    0x402010000000000,
    0x201000000000000,
    0x100000000000000,
};

uint64_t MaskAntiDiagonal[]{
    0x1,
    0x102,
    0x10204,
    0x1020408,
    0x102040810,
    0x10204081020,
    0x1020408102040,
    0x102040810204080,
    0x204081020408000,
    0x408102040800000,
    0x810204080000000,
    0x1020408000000000,
    0x2040800000000000,
    0x4080000000000000,
    0x8000000000000000,
};

uint64_t ZobristPieces[768]{};

[[nodiscard]] auto slidingAttacks(uint32_t square, uint64_t occ, uint64_t mask) {
    return ((occ & mask) - (1ULL << square)
            ^ __builtin_bswap64(__builtin_bswap64(occ & mask) - __builtin_bswap64(1ULL << square)))
         & mask;
}

[[nodiscard]] auto getDiagonalMoves(uint32_t sq, uint64_t occ) {
    return slidingAttacks(sq, occ, (1ULL << sq) ^ MaskDiagonal[7 + (sq >> 3) - (sq & 7)])
         ^ slidingAttacks(sq, occ, (1ULL << sq) ^ MaskAntiDiagonal[(sq & 7) + (sq >> 3)]);
}

[[nodiscard]] auto getFileMoves(uint32_t sq, uint64_t occ) {
    return slidingAttacks(sq, occ, (1ULL << sq) ^ 0x101010101010101 << (sq & 7));
}

[[nodiscard]] auto getOrthogonalMoves(uint32_t sq, uint64_t occ) {
    return getFileMoves(sq, occ)
         | (((getFileMoves(8 * (7 - sq), (((occ >> (sq - (sq & 7)) & 0xFF) * 0x8040201008040201) >> 7)
                                             & 0x0101010101010101)
              * 0x8040201008040201)
                 >> 56
             & 0xFF)
            << (sq - (sq & 7)));
}

[[nodiscard]] auto getKingMoves(uint32_t sq, uint64_t) {
    const auto asBb = 1ULL << sq;
    // north south
    return asBb << 8 | asBb >> 8
         // east, north east, south east
         | (asBb << 9 | asBb >> 7 | asBb << 1) & ~0x101010101010101ULL
         // west, north west, south west
         | (asBb >> 9 | asBb << 7 | asBb >> 1) & ~0x8080808080808080ULL;
}

[[nodiscard]] auto getKnightMoves(uint32_t sq, uint64_t) {
    const auto asBb = 1ULL << sq;
    return (asBb << 15 | asBb >> 17) & 0x7F7F7F7F7F7F7F7FULL | (asBb << 17 | asBb >> 15) & 0xFEFEFEFEFEFEFEFEULL
         | (asBb << 10 | asBb >> 6) & 0xFCFCFCFCFCFCFCFCULL | (asBb << 6 | asBb >> 10) & 0x3F3F3F3F3F3F3F3FULL;
}

// 0 = pawn
// 1 = knight
// 2 = bishop
// 3 = rook
// 4 = queen
// 5 = king
// 6 = no piece

// moves are ((from << 10) | (to << 4) | (promo << 2) | flag), where
//    promo = 0 (knight), 1 (bishop), 2 (rook), 3 (queen)
//     flag = 0 (normal), 1 (promotion), 2 (castling), 3 (en passant)
// we don't generate bishop or rook promos

// minify enable filter delete
[[nodiscard]] auto pieceFromChar(char c) {
    switch (c) {
        case 'p':
        case 'P':
            return 0;
        case 'n':
        case 'N':
            return 1;
        case 'b':
        case 'B':
            return 2;
        case 'r':
        case 'R':
            return 3;
        case 'q':
        case 'Q':
            return 4;
        case 'k':
        case 'K':
            return 5;
        default:
            return 6;
    }
}
// minify disable filter delete

[[nodiscard]] auto moveToString(auto move, auto blackToMove) {
    auto str = string{
        (char)('a' + (move >> 10 & 7)),
        (char)('1' + (move >> 13 ^ (blackToMove ? 7 : 0))),
        (char)('a' + (move >> 4 & 7)),
        (char)('1' + (move >> 7 & 7 ^ (blackToMove ? 7 : 0)))};

    if ((move & 3) == 1)
        str += "nbrq"[move >> 2 & 3];

    return str;
}

struct BoardState {
    // pnbrqk ours theirs
    uint64_t boards[8] = {
        0xff00000000ff00,
        0x4200000000000042,
        0x2400000000000024,
        0x8100000000000081,
        0x800000000000008,
        0x1000000000000010,
        0xffff,
        0xffff000000000000};
    bool flags[2] = {false, false};  // black to move, in check
    // TODO castling rights might be smaller as a bitfield?
    bool castlingRights[2][2] = {{true, true}, {true, true}};  // [ours, theirs][short, long]
    uint32_t epSquare = 0;
    uint64_t hash;
    // minify enable filter delete
    uint32_t halfmove = 0;
    uint32_t fullmove = 1;
    bool operator==(const BoardState &) const;
    // minify disable filter delete

    [[nodiscard]] int32_t pieceOn(auto sq) {
        return 6 * !((boards[6] | boards[7]) & (1ULL << sq))            // return 6 (no piece) if no piece is on that square
             + 4 * !!((boards[4] | boards[5]) & (1ULL << sq))           // add 4 (0b100) if there is a queen or a king on that square, as they both have that bit set
             + 2 * !!((boards[2] | boards[3]) & (1ULL << sq))           // same with 2 (0b010) for bishops and rooks
             + !!((boards[1] | boards[3] | boards[5]) & (1ULL << sq));  // and 1 (0b001) for knights, rooks or kings
    }

    [[nodiscard]] bool attackedByOpponent(auto sq) const {
        const auto bb = 1ULL << sq;
        return ((((bb << 7) & 0x7F7F7F7F7F7F7F7FULL) | ((bb << 9) & 0xFEFEFEFEFEFEFEFEULL)) & boards[0] & boards[7])  // pawns
            || (getKnightMoves(sq, 0) & boards[1] & boards[7])                                                        // knights
            || (getKingMoves(sq, 0) & boards[5] & boards[7])                                                          // kings
            || (getOrthogonalMoves(sq, boards[6] | boards[7]) & (boards[3] | boards[4]) & boards[7])                  // rooks and queens
            || (getDiagonalMoves(sq, boards[6] | boards[7]) & (boards[2] | boards[4]) & boards[7]);                   // bishops and queens
    }

    void setHash() {
        hash = 0;
        for (auto i = 0; i < 12; i++) {
            auto bb = boards[i - 6 * (i / 6)] & boards[6 + i / 6];
            while (bb) {
                auto sq = __builtin_ctzll(bb);
                bb &= bb - 1;
                hash ^= ZobristPieces[64 * i + sq];
            }
        }
    }

    // minify enable filter delete
    void setPiece(auto sq, auto piece, auto black) {
        const auto bit = 1ULL << sq;
        boards[piece] |= bit;
        boards[black == flags[0] ? 6 : 7] |= bit;
    }

    void flip() {
        for (auto &board : boards)
            board = __builtin_bswap64(board);

        swap(boards[6], boards[7]);
        swap(castlingRights[0], castlingRights[1]);
    }
    // minify disable filter delete
};

struct Board {
    BoardState state{};
    vector<BoardState> history;

    Board() {
        history.reserve(512);
    }

    void generateFromGetter(auto *&moves, auto targets,
                            auto(*getter)(uint32_t, uint64_t), auto pieces) const {
        while (pieces) {
            const auto from = __builtin_ctzll(pieces);
            pieces &= pieces - 1;
            auto attacks = getter(from, state.boards[6] | state.boards[7]) & targets;
            while (attacks) {
                const auto to = __builtin_ctzll(attacks);
                attacks &= attacks - 1;
                *(moves++) = (from << 10) | (to << 4);
            }
        }
    }

    void generateMoves(auto *moves, auto quiescence) const {
        {  // left pawn attacks
            auto attacks = (((state.boards[0] & state.boards[6]) << 7) & 0x7F7F7F7F7F7F7F7FULL)
                         & (state.boards[7] | (state.epSquare < 64 ? (1ULL << state.epSquare) : 0));
            while (attacks) {
                const auto to = __builtin_ctzll(attacks);
                attacks &= attacks - 1;
                if (to > 55) {
                    *(moves++) = ((to - 7) << 10) | (to << 4) | 13;  //  queen promo = (3 << 2) + 1 == 13
                    *(moves++) = ((to - 7) << 10) | (to << 4) | 1;   // knight promo = (0 << 2) + 1 == 1
                    // minify enable filter delete
                    *(moves++) = ((to - 7) << 10) | (to << 4) | 9;  //   rook promo = (2 << 2) + 1 == 9
                    *(moves++) = ((to - 7) << 10) | (to << 4) | 5;  // bishop promo = (1 << 2) + 1 == 5
                    // minify disable filter delete
                } else
                    *(moves++) = ((to - 7) << 10) | (to << 4) | (to == state.epSquare ? 3 : 0);
            }
        }

        {  // right pawn attacks
            auto attacks = (((state.boards[0] & state.boards[6]) << 9) & 0xFEFEFEFEFEFEFEFEULL)
                         & (state.boards[7] | (state.epSquare < 64 ? (1ULL << state.epSquare) : 0));
            while (attacks) {
                const auto to = __builtin_ctzll(attacks);
                attacks &= attacks - 1;
                if (to > 55) {
                    *(moves++) = ((to - 9) << 10) | (to << 4) | 13;
                    *(moves++) = ((to - 9) << 10) | (to << 4) | 1;
                    // minify enable filter delete
                    *(moves++) = ((to - 9) << 10) | (to << 4) | 9;
                    *(moves++) = ((to - 9) << 10) | (to << 4) | 5;
                    // minify disable filter delete
                } else
                    *(moves++) = ((to - 9) << 10) | (to << 4) | (to == state.epSquare ? 3 : 0);
            }
        }

        if (!quiescence) {
            auto singlePushes = ((state.boards[0] & state.boards[6]) << 8) & ~(state.boards[6] | state.boards[7]);
            auto doublePushes = ((singlePushes & 0xFF0000) << 8) & ~(state.boards[6] | state.boards[7]);

            while (doublePushes) {
                const auto to = __builtin_ctzll(doublePushes);
                doublePushes &= doublePushes - 1;
                *(moves++) = ((to - 16) << 10) | (to << 4);
            }

            while (singlePushes) {
                const auto to = __builtin_ctzll(singlePushes);
                singlePushes &= singlePushes - 1;
                if (to > 55) {
                    *(moves++) = ((to - 8) << 10) | (to << 4) | 13;
                    *(moves++) = ((to - 8) << 10) | (to << 4) | 1;
                    // minify enable filter delete
                    *(moves++) = ((to - 8) << 10) | (to << 4) | 9;
                    *(moves++) = ((to - 8) << 10) | (to << 4) | 5;
                    // minify disable filter delete
                } else
                    *(moves++) = ((to - 8) << 10) | (to << 4);
            }

            // if not in check, and we have short castling rights, and F1 and G1 are empty
            if (!state.flags[1] && state.castlingRights[0][0] && !((state.boards[6] | state.boards[7]) & 96 /* f1 | g1 */)
                // and F1 is not attacked
                && !state.attackedByOpponent(5 /* f1 */))
                *(moves++) = 4194;  // (e1 << 10) | (g1 << 4) | 2

            // if not in check, and we have short castling rights, and F1 and G1 are empty
            if (!state.flags[1] && state.castlingRights[0][1] && !((state.boards[6] | state.boards[7]) & 14 /* b1 | c1 | d1 */)
                // and D1 is not attacked
                && !state.attackedByOpponent(3 /* d1 */))
                *(moves++) = 4130;  // (e1 << 10) | (c1 << 4) | 2
        }

        generateFromGetter(moves, quiescence ? state.boards[7] : ~state.boards[6],
                           getKnightMoves, state.boards[1] & state.boards[6]);
        generateFromGetter(moves, quiescence ? state.boards[7] : ~state.boards[6],
                           getDiagonalMoves, (state.boards[2] | state.boards[4]) & state.boards[6]);
        generateFromGetter(moves, quiescence ? state.boards[7] : ~state.boards[6],
                           getOrthogonalMoves, (state.boards[3] | state.boards[4]) & state.boards[6]);
        generateFromGetter(moves, quiescence ? state.boards[7] : ~state.boards[6],
                           getKingMoves, state.boards[5] & state.boards[6]);
    }

    bool makeMove(auto move) {
        const auto piece = state.pieceOn(move >> 10);
        // minify enable filter delete
        assert(piece < 6);
        assert(piece == 0 || ((move & 3) != 1 && (move & 3) != 3));  // ensure promos and en passants are a pawn moving
        assert(piece == 5 || (move & 3) != 2);                       // ensure castling moves are a king moving
        assert((move >> 10) != (move >> 4 & 63));                    // ensure from != to
        // minify disable filter delete

        history.push_back(state);

        // minify enable filter delete
        state.halfmove++;
        if (piece == 0) state.halfmove = 0;

        if (state.boards[7] & 1ULL << (move >> 4 & 63))
            assert(state.pieceOn(move >> 4 & 63) < 6);
        // minify disable filter delete

        // remove captured piece
        if (state.boards[7] & 1ULL << (move >> 4 & 63)) {
            // minify enable filter delete
            state.halfmove = 0;
            // minify disable filter delete
            state.boards[state.pieceOn(move >> 4 & 63)] ^= 1ULL << (move >> 4 & 63);
            state.boards[7] ^= 1ULL << (move >> 4 & 63);
        }

        state.boards[6] ^= (1ULL << (move >> 10)) | (1ULL << (move >> 4 & 63));

        // promotion
        if ((move & 3) == 1) {
            state.boards[0] ^= 1ULL << (move >> 10);                        // unset piece
            state.boards[(move >> 2 & 3) + 1] ^= 1ULL << (move >> 4 & 63);  // set promo piece
        } else
            state.boards[piece] ^= (1ULL << (move >> 10)) | (1ULL << (move >> 4 & 63));

        // castling
        if ((move & 3) == 2) {
            // if bit 6 (0b1000000 = 64) is set, then the to square is on the right side of the board
            // therefore we're short castling, so move the rook from h1 to f1 (0b10100000 = 160)
            // otherwise, we're long castling - move the rook from a1 to d1 (0b1001 = 9)
            state.boards[3] ^= (move & 64) ? 160 : 9;
            state.boards[6] ^= (move & 64) ? 160 : 9;
        }

        // en passant
        if ((move & 3) == 3) {
            state.boards[0] ^= 1ULL << ((move >> 4 & 63) - 8);
            state.boards[7] ^= 1ULL << ((move >> 4 & 63) - 8);
        }

        state.epSquare = 64;
        // double push - set en passant square
        if (piece == 0 && (move >> 7 & 7) - (move >> 13 & 7) == 2)
            state.epSquare = 40 + (move >> 4 & 7);

        // remove castling rights
        // king moving
        if (piece == 5)
            state.castlingRights[0][0] = state.castlingRights[0][1] = false;
        // from square == h1
        state.castlingRights[0][0] &= (move & 64512) != 7168;
        // from square == a1
        state.castlingRights[0][1] &= (move & 64512) != 0;
        // to square == h8
        state.castlingRights[1][0] &= (move & 1008) != 1008;
        // to square == a8
        state.castlingRights[1][1] &= (move & 1008) != 896;

        if (state.attackedByOpponent(__builtin_ctzll(state.boards[5] & state.boards[6])))
            return true;

        state.flags[0] = !state.flags[0];

        for (auto &board : state.boards)
            board = __builtin_bswap64(board);

        swap(state.boards[6], state.boards[7]);
        swap(state.castlingRights[0], state.castlingRights[1]);
        state.flags[1] = state.attackedByOpponent(__builtin_ctzll(state.boards[5] & state.boards[6]));
        state.setHash();

        return false;
    }

    void unmakeMove() {
        // minify enable filter delete
        assert(!history.empty());
        // minify disable filter delete

        state = history.back();
        history.pop_back();
    }

    // minify enable filter delete
    void parseFen(const auto &fen) {
        const auto tokens = split(fen, ' ');

        if (tokens.size() != 6) {
            cerr << "invalid fen (" << (tokens.size() < 6 ? "not enough" : "too many") << " tokens)" << endl;
            return;
        }

        const auto ranks = split(tokens[0], '/');

        if (ranks.size() != 8) {
            cerr << "invalid fen (" << (ranks.size() < 8 ? "not enough" : "too many") << " ranks)" << endl;
            return;
        }

        BoardState newState{};

        for (auto &board : newState.boards)
            board = 0;

        newState.castlingRights[0][0] = false;
        newState.castlingRights[0][1] = false;
        newState.castlingRights[1][0] = false;
        newState.castlingRights[1][1] = false;

        for (uint32_t rank = 0; rank < 8; ++rank) {
            const auto &pieces = ranks[rank];

            uint32_t file = 0;
            for (const auto c : pieces) {
                if (file >= 8) {
                    cerr << "invalid fen (too many files in rank" << rank << ")" << endl;
                    return;
                }

                if (c >= '0' && c <= '9')
                    file += c - '0';
                else if (const auto piece = pieceFromChar(c); c != 6) {
                    newState.setPiece(((7 - rank) << 3) | file, piece, islower(c));
                    ++file;
                } else {
                    cerr << "invalid fen (invalid piece '" << c << " in rank " << rank << ")" << endl;
                    return;
                }
            }

            if (file != 8) {
                cerr << "invalid fen (" << (file < 8 ? "not enough" : "too many") << " files in rank" << rank << ")" << endl;
                return;
            }
        }

        if (tokens[1].length() != 1) {
            cerr << "invalid fen (invalid side to move)" << endl;
            return;
        }

        switch (tokens[1][0]) {
            case 'b':
                newState.flags[0] = true;
                break;
            case 'w':
                break;
            default:
                cerr << "invalid fen (invalid side to move)" << endl;
                return;
        }

        if (!newState.flags[0])
            newState.flip();

        if (newState.attackedByOpponent(__builtin_ctzll(newState.boards[5] & newState.boards[6]))) {
            cerr << "invalid fen (opponent is in check)" << endl;
            return;
        }

        if (!newState.flags[0])
            newState.flip();

        if (tokens[2].length() > 4) {
            cerr << "invalid fen (too many castling rights)" << endl;
            return;
        }

        if (tokens[2].length() > 1 || tokens[2][0] != '-') {
            for (const char flag : tokens[2]) {
                switch (flag) {
                    case 'K':
                        newState.castlingRights[0][0] = true;
                        break;
                    case 'k':
                        newState.castlingRights[1][0] = true;
                        break;
                    case 'Q':
                        newState.castlingRights[0][1] = true;
                        break;
                    case 'q':
                        newState.castlingRights[1][1] = true;
                        break;
                    default:
                        cerr << "invalid fen (invalid castling rights flag ' << " << flag << "')" << endl;
                        return;
                }
            }
        }

        if (tokens[3] != "-") {
            if (tokens[3].length() != 2
                || tokens[3][0] < 'a' || tokens[3][0] > 'h'
                || tokens[3][1] < '1' || tokens[3][1] > '8'
                || tokens[3][1] != (newState.flags[0] ? '3' : '6')) {
                cerr << "invalid fen (invalid en passant square)" << endl;
                return;
            }

            // pre-flip, always set as if black just moved
            newState.epSquare = (5 << 3) | (tokens[3][0] - 'a');
        }

        try {
            newState.halfmove = stoul(tokens[4]);
        } catch (...) {
            cerr << "invalid fen (invalid halfmove clock)" << endl;
            return;
        }

        try {
            newState.fullmove = stoul(tokens[5]);
        } catch (...) {
            cerr << "invalid fen (invalid fullmove number)" << endl;
            return;
        }

        history.clear();

        state = newState;
        if (state.flags[0])
            state.flip();

        state.flags[1] = state.attackedByOpponent(__builtin_ctzll(state.boards[5] & state.boards[6]));
        state.setHash();
    }

    [[nodiscard]] static Board fromFen(const auto &fen) {
        Board board{};
        board.parseFen(fen);
        return board;
    }
    // minify disable filter delete

    int32_t evaluateColor(auto color) {
        return __builtin_popcountll(state.boards[0] & state.boards[6 + color]) * 100 + __builtin_popcountll(state.boards[1] & state.boards[6 + color]) * 300 + __builtin_popcountll(state.boards[2] & state.boards[6 + color]) * 300 + __builtin_popcountll(state.boards[3] & state.boards[6 + color]) * 500 + __builtin_popcountll(state.boards[4] & state.boards[6 + color]) * 900;
    }

    int32_t evaluate() {
        return evaluateColor(false) - evaluateColor(true);
    }
};

// minify enable filter delete
uint64_t doPerft(auto &board, auto depth) {
    if (depth == 0)
        return 1;

    uint64_t total = 0;

    uint16_t moves[256] = {0};
    board.generateMoves(moves, false);

    uint64_t i = 0;
    while (const auto move = moves[i++]) {
        if (board.makeMove(move)) {
            board.unmakeMove();
            continue;
        }

        total += depth > 1 ? doPerft(board, depth - 1) : 1;

        board.unmakeMove();
    }

    return total;
}

void perft(auto &board, auto depth) {
    uint64_t total = 0;

    uint16_t moves[256] = {0};
    board.generateMoves(moves, false);

    uint64_t i = 0;
    while (const auto move = moves[i++]) {
        if (board.makeMove(move)) {
            board.unmakeMove();
            continue;
        }

        const auto value = doPerft(board, depth - 1);
        total += value;

        board.unmakeMove();

        cout << moveToString(move, board.state.flags[0]) << "\t" << value << endl;
    }

    cout << total << " nodes" << endl;
}
// minify disable filter delete

struct ThreadData {
    uint16_t bestMove = 0;

    // minify enable filter delete
    uint64_t nodes = 0;
    // minify disable filter delete

    bool searchComplete = true;
};

int32_t negamax(auto &board, auto &threadData, auto ply, auto depth, auto alpha, auto beta, auto hardTimeLimit) {
    if (chrono::high_resolution_clock::now() >= hardTimeLimit) {
        threadData.searchComplete = false;
        return 0;
    }

    int32_t staticEval;
    if (depth < 1) {
        staticEval = board.evaluate();

        if (staticEval >= beta)
            return staticEval;

        alpha = max(alpha, staticEval);
    } else if (ply > 0 && board.history.size() > 1)
        for (auto i = board.history.size() - 2; i > 1; i -= 2)
            if (board.state.hash == board.history[i].hash) return 0;

    auto i = 0;
    uint16_t moves[256] = {0};
    board.generateMoves(moves, depth < 1);

    // mvv-lva sorting
    pair<int32_t, uint16_t> scoredMoves[256];
    while (auto move = moves[i++])
        scoredMoves[i] = {board.state.pieceOn(move >> 4 & 63) > 5 ? 0 : 9 + 9 * board.state.pieceOn(move >> 4 & 63) - board.state.pieceOn(move >> 10),
                          move};
    stable_sort(scoredMoves, scoredMoves + i, greater());

    int32_t bestScore = depth < 1 ? staticEval : -32000;

    auto movesMade = 0;
    i = 0;
    while (const auto move = scoredMoves[i++].second) {
        if (board.makeMove(move)) {
            board.unmakeMove();
            continue;
        }

        movesMade++;
        // minify enable filter delete
        threadData.nodes++;
        // minify disable filter delete

        const int32_t score = -negamax(board, threadData, ply + 1, depth - 1, -beta, -alpha, hardTimeLimit);

        board.unmakeMove();

        if (score > bestScore) {
            bestScore = score;
            if (!ply)
                threadData.bestMove = move;

            if (score > alpha) {
                alpha = score;
                if (alpha >= beta)
                    break;
            }
        }
    }

    if (!movesMade)
        return depth < 1 ? alpha : board.state.flags[1] ? -32000 + ply
                                                        : 0;

    return bestScore;
}

void searchRoot(auto &board, auto &threadData, auto timeRemaining, auto increment
                // minify enable filter delete
                ,
                int32_t searchDepth = 64
                // minify disable filter delete
) {
    auto bestMove = 0;
    auto startTime = chrono::high_resolution_clock::now();
    for (auto depth = 1; depth <
                         // minify enable filter delete
                         searchDepth +
                             // minify disable filter delete
                             64;
         depth++) {
        // minify enable filter delete
        if (chrono::high_resolution_clock::now() >= startTime + chrono::milliseconds(timeRemaining / 40 + increment / 2)) {
            break;
        }
        // minify disable filter delete

        // minify enable filter delete
        auto value =
            // minify disable filter delete
            negamax(board, threadData, 0, depth, -32000, 32000,
                    startTime + chrono::milliseconds(timeRemaining / 40 + increment / 2));

        if (threadData.searchComplete)
            bestMove = threadData.bestMove;

        // minify enable filter delete
        cout << "info depth " << depth << " nodes " << threadData.nodes << " score cp " << value << " pv " << moveToString(threadData.bestMove, board.state.flags[0]) << endl;
        // minify disable filter delete
    }

    cout << "bestmove " << moveToString(bestMove, board.state.flags[0]) << endl;
}

// minify enable filter delete
const string fens[]{
    "r3k2r/2pb1ppp/2pp1q2/p7/1nP1B3/1P2P3/P2N1PPP/R2QK2R w KQkq a6 0 14",
    "4rrk1/2p1b1p1/p1p3q1/4p3/2P2n1p/1P1NR2P/PB3PP1/3R1QK1 b - - 2 24",
    "r3qbrk/6p1/2b2pPp/p3pP1Q/PpPpP2P/3P1B2/2PB3K/R5R1 w - - 16 42",
    "6k1/1R3p2/6p1/2Bp3p/3P2q1/P7/1P2rQ1K/5R2 b - - 4 44",
    "8/8/1p2k1p1/3p3p/1p1P1P1P/1P2PK2/8/8 w - - 3 54",
    "7r/2p3k1/1p1p1qp1/1P1Bp3/p1P2r1P/P7/4R3/Q4RK1 w - - 0 36",
    "r1bq1rk1/pp2b1pp/n1pp1n2/3P1p2/2P1p3/2N1P2N/PP2BPPP/R1BQ1RK1 b - - 2 10",
    "3r3k/2r4p/1p1b3q/p4P2/P2Pp3/1B2P3/3BQ1RP/6K1 w - - 3 87",
    "2r4r/1p4k1/1Pnp4/3Qb1pq/8/4BpPp/5P2/2RR1BK1 w - - 0 42",
    "4q1bk/6b1/7p/p1p4p/PNPpP2P/KN4P1/3Q4/4R3 b - - 0 37",
    "2q3r1/1r2pk2/pp3pp1/2pP3p/P1Pb1BbP/1P4Q1/R3NPP1/4R1K1 w - - 2 34",
    "1r2r2k/1b4q1/pp5p/2pPp1p1/P3Pn2/1P1B1Q1P/2R3P1/4BR1K b - - 1 37",
    "r3kbbr/pp1n1p1P/3ppnp1/q5N1/1P1pP3/P1N1B3/2P1QP2/R3KB1R b KQkq b3 0 17",
    "8/6pk/2b1Rp2/3r4/1R1B2PP/P5K1/8/2r5 b - - 16 42",
    "1r4k1/4ppb1/2n1b1qp/pB4p1/1n1BP1P1/7P/2PNQPK1/3RN3 w - - 8 29",
    "8/p2B4/PkP5/4p1pK/4Pb1p/5P2/8/8 w - - 29 68",
    "3r4/ppq1ppkp/4bnp1/2pN4/2P1P3/1P4P1/PQ3PBP/R4K2 b - - 2 20",
    "5rr1/4n2k/4q2P/P1P2n2/3B1p2/4pP2/2N1P3/1RR1K2Q w - - 1 49",
    "1r5k/2pq2p1/3p3p/p1pP4/4QP2/PP1R3P/6PK/8 w - - 1 51",
    "q5k1/5ppp/1r3bn1/1B6/P1N2P2/BQ2P1P1/5K1P/8 b - - 2 34",
    "r1b2k1r/5n2/p4q2/1ppn1Pp1/3pp1p1/NP2P3/P1PPBK2/1RQN2R1 w - - 0 22",
    "r1bqk2r/pppp1ppp/5n2/4b3/4P3/P1N5/1PP2PPP/R1BQKB1R w KQkq - 0 5",
    "r1bqr1k1/pp1p1ppp/2p5/8/3N1Q2/P2BB3/1PP2PPP/R3K2n b Q - 1 12",
    "r1bq2k1/p4r1p/1pp2pp1/3p4/1P1B3Q/P2B1N2/2P3PP/4R1K1 b - - 2 19",
    "r4qk1/6r1/1p4p1/2ppBbN1/1p5Q/P7/2P3PP/5RK1 w - - 2 25",
    "r7/6k1/1p6/2pp1p2/7Q/8/p1P2K1P/8 w - - 0 32",
    "r3k2r/ppp1pp1p/2nqb1pn/3p4/4P3/2PP4/PP1NBPPP/R2QK1NR w KQkq - 1 5",
    "3r1rk1/1pp1pn1p/p1n1q1p1/3p4/Q3P3/2P5/PP1NBPPP/4RRK1 w - - 0 12",
    "5rk1/1pp1pn1p/p3Brp1/8/1n6/5N2/PP3PPP/2R2RK1 w - - 2 20",
    "8/1p2pk1p/p1p1r1p1/3n4/8/5R2/PP3PPP/4R1K1 b - - 3 27",
    "8/4pk2/1p1r2p1/p1p4p/Pn5P/3R4/1P3PP1/4RK2 w - - 1 33",
    "8/5k2/1pnrp1p1/p1p4p/P6P/4R1PK/1P3P2/4R3 b - - 1 38",
    "8/8/1p1kp1p1/p1pr1n1p/P6P/1R4P1/1P3PK1/1R6 b - - 15 45",
    "8/8/1p1k2p1/p1prp2p/P2n3P/6P1/1P1R1PK1/4R3 b - - 5 49",
    "8/8/1p4p1/p1p2k1p/P2npP1P/4K1P1/1P6/3R4 w - - 6 54",
    "8/8/1p4p1/p1p2k1p/P2n1P1P/4K1P1/1P6/6R1 b - - 6 59",
    "8/5k2/1p4p1/p1pK3p/P2n1P1P/6P1/1P6/4R3 b - - 14 63",
    "8/1R6/1p1K1kp1/p6p/P1p2P1P/6P1/1Pn5/8 w - - 0 67",
    "1rb1rn1k/p3q1bp/2p3p1/2p1p3/2P1P2N/PP1RQNP1/1B3P2/4R1K1 b - - 4 23",
    "4rrk1/pp1n1pp1/q5p1/P1pP4/2n3P1/7P/1P3PB1/R1BQ1RK1 w - - 3 22",
    "r2qr1k1/pb1nbppp/1pn1p3/2ppP3/3P4/2PB1NN1/PP3PPP/R1BQR1K1 w - - 4 12",
    "2r2k2/8/4P1R1/1p6/8/P4K1N/7b/2B5 b - - 0 55",
    "6k1/5pp1/8/2bKP2P/2P5/p4PNb/B7/8 b - - 1 44",
    "2rqr1k1/1p3p1p/p2p2p1/P1nPb3/2B1P3/5P2/1PQ2NPP/R1R4K w - - 3 25",
    "r1b2rk1/p1q1ppbp/6p1/2Q5/8/4BP2/PPP3PP/2KR1B1R b - - 2 14",
    "6r1/5k2/p1b1r2p/1pB1p1p1/1Pp3PP/2P1R1K1/2P2P2/3R4 w - - 1 36",
    "rnbqkb1r/pppppppp/5n2/8/2PP4/8/PP2PPPP/RNBQKBNR b KQkq c3 0 2",
    "2rr2k1/1p4bp/p1q1p1p1/4Pp1n/2PB4/1PN3P1/P3Q2P/2RR2K1 w - f6 0 20",
    "3br1k1/p1pn3p/1p3n2/5pNq/2P1p3/1PN3PP/P2Q1PB1/4R1K1 w - - 0 23",
    "2r2b2/5p2/5k2/p1r1pP2/P2pB3/1P3P2/K1P3R1/7R w - - 23 93",
    "5k2/4q1p1/3P1pQb/1p1B4/pP5p/P1PR4/5PP1/1K6 b - - 0 38",
    "6k1/6p1/8/6KQ/1r6/q2b4/8/8 w - - 0 32",
    "5rk1/1rP3pp/p4n2/3Pp3/1P2Pq2/2Q4P/P5P1/R3R1K1 b - - 0 32",
    "4r1k1/4r1p1/8/p2R1P1K/5P1P/1QP3q1/1P6/3R4 b - - 0 1",
    "R4r2/4q1k1/2p1bb1p/2n2B1Q/1N2pP2/1r2P3/1P5P/2B2KNR w - - 3 31",
    "r6k/pbR5/1p2qn1p/P2pPr2/4n2Q/1P2RN1P/5PBK/8 w - - 2 31",
    "rn2k3/4r1b1/pp1p1n2/1P1q1p1p/3P4/P3P1RP/1BQN1PR1/1K6 w - - 6 28",
    "3q1k2/3P1rb1/p6r/1p2Rp2/1P5p/P1N2pP1/5B1P/3QRK2 w - - 1 42",
    "4r2k/1p3rbp/2p1N1p1/p3n3/P2NB1nq/1P6/4R1P1/B1Q2RK1 b - - 4 32",
    "4r1k1/1q1r3p/2bPNb2/1p1R3Q/pB3p2/n5P1/6B1/4R1K1 w - - 2 36",
    "3qr2k/1p3rbp/2p3p1/p7/P2pBNn1/1P3n2/6P1/B1Q1RR1K b - - 1 30",
    "3qk1b1/1p4r1/1n4r1/2P1b2B/p3N2p/P2Q3P/8/1R3R1K w - - 2 39",
};

void bench() {
    Board board{};

    uint64_t totalNodes = 0;

    const auto startTimePoint = chrono::high_resolution_clock::now();

    for (const auto &fen : fens) {
        board.parseFen(fen);

        ThreadData threadData{};
        searchRoot(board, threadData, 86'400'000, 86'400'000, 5 - 64);

        totalNodes += threadData.nodes;
    }

    const auto endTime = chrono::high_resolution_clock::now();
    const auto elapsedTimePoint = chrono::duration_cast<chrono::milliseconds>(endTime
                                                                              - startTimePoint);
    auto elapsedTime = elapsedTimePoint.count();
    elapsedTime = max<int64_t>(elapsedTime, 1);

    const auto nps = static_cast<uint64_t>(static_cast<double>(totalNodes) / (static_cast<double>(elapsedTime) / 1000.0));

    cout << "info time " << elapsedTime << endl;
    cout << totalNodes << " nodes " << nps << " nps" << endl;
}
// minify disable filter delete

int32_t main(
    // minify enable filter delete
    int argc, char *argv[]
    // minify disable filter delete
) {
    // initialise zobrist hashes, xor-shift prng
    uint64_t seed = 0x179827108ULL;
    for (auto i = 0; i < 768; i++)
        ZobristPieces[i] = seed ^= (seed ^= (seed ^= seed << 13) >> 7) << 17;

    // minify enable filter delete
    if (argc > 1 && string{argv[1]} == "bench") {
        bench();
        return 0;
    }
    // minify disable filter delete

    Board board{};
    string line;

    board.state.setHash();

    while (getline(cin, line)) {
        const auto tokens = split(line, ' ');

        if (tokens[0] == "quit")
            break;
        else if (tokens[0] == "uci") {
            cout << "id name Pytteliten 0.1\nid author Crippa\nuciok" << endl;
        } else if (tokens[0] == "isready")
            cout << "readyok" << endl;
        // else if (tokens[0] == "ucinewgame")
        else if (tokens[0] == "position") {
            // assume that the second token is 'startpos'
            board = Board{};
            board.state.setHash();

            // minify enable filter delete
            if (tokens[1] == "fen") {
                string fen;

                // concatenate the fen back together
                for (auto i = 2; i < 8; i++)
                    fen += tokens[i] + " ";

                board.parseFen(fen);

                auto move_string = find(tokens.begin(), tokens.end(), "moves");

                if (move_string != tokens.end()) {
                    for (move_string++; move_string != tokens.end(); move_string++) {
                        uint16_t moves[256] = {0};
                        board.generateMoves(moves, false);
                        for (const auto &move : moves)
                            if (*move_string == moveToString(move, board.state.flags[0])) {
                                board.makeMove(move);
                                break;
                            }
                    }
                }

                continue;
            }
            // minify disable filter delete

            if (tokens.size() > 2) {
                // assume that the third token is 'moves'
                for (auto i = 3; i < tokens.size(); i++) {
                    uint16_t moves[256] = {0};
                    board.generateMoves(moves, false);
                    for (const auto &move : moves)
                        if (tokens[i] == moveToString(move, board.state.flags[0])) {
                            board.makeMove(move);
                            break;
                        }
                }
            }
        } else if (tokens[0] == "go") {
            ThreadData threadData{};
            searchRoot(board,
                       threadData,
                       stoi(tokens[board.state.flags[0] ? 4 : 2]),
                       stoi(tokens[board.state.flags[0] ? 8 : 6]));
        }
        // minify enable filter delete
        else if (tokens[0] == "perft") {
            perft(board, stoi(tokens[1]));
        }
        // minify disable filter delete
    }
}
