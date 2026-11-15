#include "analysis/histogram.h"

namespace stegoscan {

bool are_histograms_equal(const HistogramResult& a, const HistogramResult& b) {
    if (a.channel_count != b.channel_count) {
        return false;
    }
    if (a.samples_per_channel != b.samples_per_channel) {
        return false;
    }
    for (std::size_t c = 0; c < a.channel_count; ++c) {
        for (std::size_t bin = 0; bin < kHistogramBins; ++bin) {
            if (a.channels[c][bin] != b.channels[c][bin]) {
                return false;
            }
        }
    }
    return true;
}

} // namespace stegoscan
