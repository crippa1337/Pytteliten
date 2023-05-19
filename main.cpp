#include <iostream>

std::uint64_t MaskFile[] = {
    0x101010101010101,
    0x202020202020202,
    0x404040404040404,
    0x808080808080808,
    0x1010101010101010,
    0x2020202020202020,
    0x4040404040404040,
    0x8080808080808080,
};

std::uint64_t MaskRank[]
    = {0xff, 0xff00, 0xff0000, 0xff000000, 0xff00000000, 0xff0000000000, 0xff000000000000, 0xff00000000000000};

std::uint64_t MaskDiagonal[] = {
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

std::uint64_t MaskAntiDiagonal[] = {
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

auto slidingAttacks(std::uint64_t square, std::uint64_t occ, std::uint64_t mask) {
    return ((occ & mask) - (1ULL << square)
            ^ __builtin_bswap64(__builtin_bswap64(occ & mask) - __builtin_bswap64((1ULL << square))))
         & mask;
}

auto bishop(std::uint64_t sq, std::uint64_t occ) {
    return slidingAttacks(sq, occ, MaskDiagonal[7 + (sq >> 3) - (sq & 7)] ^ MaskAntiDiagonal[(sq >> 3) + (sq & 7)]);
}

auto rook(std::uint64_t sq, std::uint64_t occ) {
    return slidingAttacks(sq, occ, MaskRank[sq >> 3] ^ MaskFile[sq & 7]);
}

auto king(std::uint64_t sq) {
    const auto as_bb = 1ULL << sq;
    // north south
    return as_bb << 8
         | as_bb >> 8
         // east, north east, south east
         | (as_bb << 9 | as_bb << 7 | as_bb << 1) & ~0x101010101010101ULL
         // west, north west, south west
         | (as_bb >> 9 | as_bb >> 7 | as_bb >> 1) & ~0x8080808080808080ULL;
}

auto knight(uint64_t sq) {
    const auto as_bb = 1ULL << sq;
    return (as_bb << 15 | as_bb >> 17) & 0x7F7F7F7F7F7F7F7FULL | (as_bb << 17 | as_bb >> 15) & 0xFEFEFEFEFEFEFEFEULL
         | (as_bb << 10 | as_bb >> 6) & 0xFCFCFCFCFCFCFCFCULL | (as_bb << 6 | as_bb >> 10) & 0x3F3F3F3F3F3F3F3FULL;
}

int main() {
    std::cout << "hello :)";
}