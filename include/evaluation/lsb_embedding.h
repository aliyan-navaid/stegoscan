#ifndef STEGOSCAN_EVALUATION_LSB_EMBEDDING_H_
#define STEGOSCAN_EVALUATION_LSB_EMBEDDING_H_

#include "image/image.h"

#include <cstdint>
#include <vector>

namespace stegoscan {

/**
 * @brief Outcome of a deterministic LSB-replacement embedding.
 *
 * `positions[i]` is a flat sample index into `stego.pixels` (row-major, interleaved
 * channels) and `bits[i]` is the payload bit written there. Both vectors have the same
 * length, which equals round(payload_rate * total_channel_samples). The records exist so
 * tests (and callers) can verify the embedding exactly instead of inferring it.
 */
struct LsbEmbeddingResult {
    Image stego;
    std::vector<std::uint64_t> positions;
    std::vector<std::uint8_t> bits;
};

/**
 * @brief Embeds pseudo-random payload bits into a copy of `cover` by LSB replacement.
 *
 * Contract:
 * - selected = round(payload_rate * total_channel_samples) distinct sample positions are
 *   drawn without replacement using a seeded, platform-independent PRNG.
 * - For each selected sample: new = (old & 0xFE) | bit. All other samples are unchanged.
 * - `cover` is never modified. The result is a pure function of (cover, rate, seed).
 * - Not parallel (no OpenMP); this is a fixture generator, not a benchmarked component.
 *
 * @throws std::invalid_argument if `cover` is invalid, the rate is not finite or not in
 *         (0, 1], or the rate rounds to zero selected samples for this image size.
 */
LsbEmbeddingResult embed_lsb_replacement(const Image& cover,
                                         double payload_rate,
                                         std::uint64_t seed);

} // namespace stegoscan

#endif // STEGOSCAN_EVALUATION_LSB_EMBEDDING_H_
