#include <iostream>
#include <string>
#include <vector>

// !delete start
#include <cassert>
// !delete end

std::uint64_t MaskDiagonal[]{
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

std::uint64_t MaskAntiDiagonal[]{
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

[[nodiscard]] std::uint64_t slidingAttacks(std::uint32_t square, std::uint64_t occ, std::uint64_t mask) {
    return ((occ & mask) - (1ULL << square)
            ^ __builtin_bswap64(__builtin_bswap64(occ & mask) - __builtin_bswap64(1ULL << square)))
         & mask;
}

[[nodiscard]] std::uint64_t getDiagonalMoves(std::uint32_t sq, std::uint64_t occ) {
    return slidingAttacks(sq, occ, MaskDiagonal[7 + (sq & 7) - (sq >> 3)])
            ^ slidingAttacks(sq, occ, MaskAntiDiagonal[(sq & 7) + (sq >> 3)]);
}

// currently just file attacks
[[nodiscard]] std::uint64_t getOrthogonalMoves(std::uint32_t sq, std::uint64_t occ) {
    return slidingAttacks(sq, occ, (1ULL << sq) ^ 0x101010101010101 << (sq & 7));
}

[[nodiscard]] std::uint64_t getKingMoves(std::uint32_t sq, std::uint64_t) {
    const auto asBb = 1ULL << sq;
    // north south
    return asBb << 8
         | asBb >> 8
         // east, north east, south east
         | (asBb << 9 | asBb << 7 | asBb << 1) & ~0x101010101010101ULL
         // west, north west, south west
         | (asBb >> 9 | asBb >> 7 | asBb >> 1) & ~0x8080808080808080ULL;
}

[[nodiscard]] std::uint64_t getKnightMoves(std::uint32_t sq, std::uint64_t) {
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

struct BoardState {
    // pnbrqk ours theirs
    std::uint64_t boards[8];
    bool flags[2];  // black to move, in check
    // TODO castling rights might be smaller as a bitfield?
    bool castlingRights[2][2];  // [ours, theirs][short, long]
    std::uint32_t epSquare;
    // TODO other board state
};

struct Board {
    BoardState state{};
    std::vector<BoardState> history;

    Board() {
        history.reserve(512);
    }

    [[nodiscard]] std::uint32_t pieceOn(std::uint32_t sq) {
        return 6 * !((state.boards[6] | state.boards[7]) & (1ULL << sq))                  // return 6 (no piece) if no piece is on that square
             + 4 * !!((state.boards[4] | state.boards[5]) & (1ULL << sq))                 // add 4 (0b100) if there is a queen or a king on that square, as they both have that bit set
             + 2 * !!((state.boards[2] | state.boards[3]) & (1ULL << sq))                 // same with 2 (0b010) for bishops and rooks
             + !!((state.boards[1] | state.boards[3] | state.boards[5]) & (1ULL << sq));  // and 1 (0b001) for knights, rooks or kings
    }

    [[nodiscard]] bool attackedByOpponent(std::uint32_t sq) const {
        const auto bb = 1ULL << sq;
        return (((bb << 7) | (bb << 9)) & state.boards[0] & state.boards[7])                                                        // pawns
            || (getKnightMoves(sq, 0) & state.boards[1] & state.boards[7])                                                          // knights
            || (getKingMoves(sq, 0) & state.boards[5] & state.boards[7])                                                            // kings
            || (getOrthogonalMoves(sq, state.boards[6] | state.boards[7]) & (state.boards[3] | state.boards[4]) & state.boards[7])  // rooks and queens
            || (getDiagonalMoves(sq, state.boards[6] | state.boards[7]) & (state.boards[2] | state.boards[4]) & state.boards[7]);   // bishops and queens
    }

    void generateFromGetter(std::uint16_t *&moves, std::uint64_t targets,
                            std::uint64_t (*getter)(std::uint32_t, std::uint64_t), std::uint64_t pieces) const {
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

    void generateMoves(std::uint16_t *moves, bool quiescence) const {
        {  // left pawn attacks
            auto attacks = (((state.boards[0] & state.boards[6]) << 7) & 0x7F7F7F7F7F7F7F7FULL)
                         & (state.boards[7] | (state.epSquare < 64 ? (1ULL << state.epSquare) : 0));
            while (attacks) {
                const auto to = __builtin_ctzll(attacks);
                attacks &= attacks - 1;
                if (to > 55) {
                    *(moves++) = ((to - 7) << 10) | (to << 4) | 13;  //  queen promo = (3 << 4) + 1 == 13
                    *(moves++) = ((to - 7) << 10) | (to << 4) | 1;   // knight promo = (0 << 4) + 1 == 1
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
                } else
                    *(moves++) = ((to - 8) << 10) | (to << 4);
            }

            // if not in check, and we have short castling rights, and F1 and G1 are empty
            if (!state.flags[1] && state.castlingRights[0][0] && !((state.boards[6] | state.boards[7]) & 96 /* f1 | g1 */)
                // and F1 is not attacked
                && !attackedByOpponent(5 /* f1 */))
                *(moves++) = 4194;  // (e1 << 10) | (g1 << 4) | 3

            // if not in check, and we have short castling rights, and F1 and G1 are empty
            if (!state.flags[1] && state.castlingRights[0][1] && !((state.boards[6] | state.boards[7]) & 12 /* c1 | d1 */)
                // and D1 is not attacked
                && !attackedByOpponent(3 /* d1 */))
                *(moves++) = 4130;  // (e1 << 10) | (c1 << 4) | 3
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

    void makeMove(std::uint16_t move) {
        const auto piece = pieceOn(move >> 10);
        // !delete start
        assert(piece < 6);
        assert(piece == 0 || ((move & 3) != 1 && (move & 3) != 3));  // ensure promos and en passants are a pawn moving
        assert(piece == 5 || (move & 3) != 2);                       // ensure castling moves are a king moving
        assert((move >> 10) != (move >> 4 & 63));                    // ensure from != to
        // !delete end

        history.push_back(state);

        // remove captured piece
        if (state.boards[7] & 1ULL << (move >> 4 & 63)) {
            state.boards[pieceOn(move >> 4 & 63)] ^= 1ULL << (move >> 4 & 63);
            state.boards[7] ^= 1ULL << (move >> 4 & 63);
        }

        state.boards[6] ^= (1ULL << (move >> 10)) | (1ULL << (move >> 4 & 63));

        // promotion
        if ((move & 3) == 1) {
            state.boards[0] ^= 1ULL << (move >> 10);                        // unset piece
            state.boards[(move >> 2 & 3) + 1] ^= 1ULL << (move >> 4 & 63);  // set promo piece
        } else
            state.boards[piece] ^= (1ULL << (move >> 10)) | (1ULL << (move >> 4 & 63));

        // !delete start
        if (state.boards[7] & 1ULL << (move >> 4 & 63))
            assert(pieceOn(move >> 4 & 63) < 6);
        // !delete end

        // castling
        if ((move & 3) == 2) {
            // if bit 6 (0b1000000 = 64) is set, then the to square is on the right side of the board
            // therefore we're short castling, so move the rook from h1 to f1 (0b10100000 = 160)
            // otherwise, we're long castling - move the rook from a1 to d1 (0b1001 = 9)
            state.boards[3] ^= (move & 64) ? 160 : 9;
            state.boards[6] ^= (move & 64) ? 160 : 9;
        }

        state.epSquare = 64;
        // en passant
        if (piece == 0 && (move >> 7) - (move >> 13 & 7) == 2)
            state.epSquare = 40 + (move >> 4 & 7);

        // remove castling rights
        if (piece == 5)  // king moving
            state.castlingRights[0][0] = state.castlingRights[0][1] = false;
        // from square == h1
        state.castlingRights[0][0] &= (move & 1008) != 112;
        // from square == a1
        state.castlingRights[0][1] &= !!(move & 1008);
        // to square == h8
        state.castlingRights[1][0] &= (move & 64512) != 64512;
        // to square == a8
        state.castlingRights[1][1] &= (move & 64512) != 57344;

        state.flags[1] = attackedByOpponent(__builtin_ctzll(state.boards[5] & state.boards[6]));
        state.flags[0] = !state.flags[0];

        for (auto &board : state.boards)
            board = __builtin_bswap64(board);

        std::swap(state.boards[6], state.boards[7]);
        std::swap(state.castlingRights[0], state.castlingRights[1]);
    }

    void unmakeMove() {
        // !delete start
        assert(!history.empty());
        // !delete end

        state = history.back();
        history.pop_back();
    }
};

[[nodiscard]] std::string moveToString(std::uint16_t move, bool blackToMove) {
    auto str = std::string{
        (char)('a' + (move >> 10 & 7)),
        (char)('1' + (move >> 13 ^ (blackToMove ? 7 : 0))),
        (char)('a' + (move >> 4 & 7)),
        (char)('1' + (move >> 7 & 7 ^ (blackToMove ? 7 : 0)))};

    if ((move & 3) == 1)
        str += "nbrq"[move >> 2 & 3];

    return str;
}

// !delete start
std::size_t doPerft(Board &board, std::int32_t depth) {
    if (depth == 0)
        return 1;

    std::size_t total = 0;

    std::uint16_t moves[256] = {0};
    board.generateMoves(moves, false);

    std::size_t i = 0;
    while (const auto move = moves[i++]) {
        board.makeMove(move);

        if (board.attackedByOpponent(__builtin_ctzll(board.state.boards[5] & board.state.boards[6])))
            continue;

        total += doPerft(board, depth - 1);

        board.unmakeMove();
    }

    return total;
}

void perft(Board &board, std::int32_t depth) {
    std::size_t total = 0;

    std::uint16_t moves[256] = {0};
    board.generateMoves(moves, false);

    std::size_t i = 0;
    while (const auto move = moves[i++]) {
        board.makeMove(move);

        if (board.attackedByOpponent(__builtin_ctzll(board.state.boards[5] & board.state.boards[6])))
            continue;

        const auto value = doPerft(board, depth - 1);
        total += value;

        board.unmakeMove();

        std::cout << moveToString(move, board.state.flags[0]) << "\t" << value << std::endl;
    }

    std::cout << total << " nodes" << std::endl;
}
// !delete end

int main() {
    // !delete start
    Board board{};

    // rnbqkbnr/pppp1ppp/8/4p3/P7/8/1PPPPPPP/RNBQKBNR w KQkq - 0 1
    board.state.boards[0] = 0x00FF00000000FF00ULL;
    board.state.boards[1] = 0x4200000000000042ULL;
    board.state.boards[2] = 0x2400000000000024ULL;
    board.state.boards[3] = 0x8100000000000081ULL;
    board.state.boards[4] = 0x0800000000000008ULL;
    board.state.boards[5] = 0x1000000000000010ULL;
    board.state.boards[6] = 0x000000000000FFFFULL;
    board.state.boards[7] = 0xFFFF000000000000ULL;

    board.state.castlingRights[0][0] = true;
    board.state.castlingRights[0][1] = true;
    board.state.castlingRights[1][0] = true;
    board.state.castlingRights[1][1] = true;

    board.state.epSquare = 64;

    perft(board, 1);
    // !delete end
}
