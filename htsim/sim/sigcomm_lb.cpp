#include "sigcomm_lb.h"
#include <cstdint>

// SigcommLoadBalancing::SigcommLoadBalancing(uint16_t no_of_paths, bool debug, uint8_t max_penalty)
//     : UecMultipath(debug),
//       _no_of_paths(no_of_paths),
//       _current_ev_index(0),
//       _max_penalty(max_penalty),
//       _ev_states() {
//     uint16_t mask = _no_of_paths - 1;
//     _path_random = rand() & UINT16_MAX; // random upper bits of EV
//     _path_random &= ~mask;  // save only upper bits

//     _path_xor = rand() & mask;

//     _ev_states.resize(_no_of_paths);
//     for (uint32_t i = 0; i < _no_of_paths; i++) {
//         _ev_states[i].smoothed_delay = 0;
//         _ev_states[i].congestion = 0;
//         _ev_states[i].penalty = 0;
//         _ev_states[i].explored = false;
//     }

//     if (_debug) {
//         cout << "Multipath"
//              << " Sigcomm"
//              << " _no_of_paths " << _no_of_paths << " _path_random " << _path_random
//              << " _path_xor " << _path_xor << " _max_penalty " << (uint32_t)_max_penalty << endl;
//     }
// }

// void SigcommLoadBalancing::processEv(uint16_t path_id,
//                                      PathFeedback feedback,
//                                      simtime_picosec delay) {
//     const auto s = &_ev_states[path_id];

//     if (feedback == UecMultipath::PATH_TIMEOUT) {
//         s->penalty = _max_penalty;
//         return;  // Do not update delay on timeout (ambiguous latency)
//     }

//     if (feedback == UecMultipath::PATH_NACK) {
//         // Cast to ensure we don't overflow uint8_t before clamping
//         int new_penalty = (int)s->penalty + (_max_penalty / 2);
//         s->penalty = (uint8_t)std::min((int)_max_penalty, new_penalty);
//         return;
//     }

//     float f = 0;
//     if (feedback == UecMultipath::PATH_ECN) {
//         f = 1;
//     }
//     s->congestion = (1 - gain) * s->congestion + gain * f;

//     if (s->explored == false) {
//         s->smoothed_delay = delay;
//         s->explored = true;
//     } else {
//         s->smoothed_delay = (1 - smoothing_factor) * s->smoothed_delay + smoothing_factor *
//         delay;
//     }
// }

// double SigcommLoadBalancing::eval(PathState* s) const {
//     // If we haven't explored this path, we must do it now!
//     if (s->explored == false) {
//         return 0.0f;
//     }

//     double capacity_multiplier = 1.0f + (s->congestion * scale);

//     // Convert penalty (uint8) into Delay Units (picoseconds).
//     // Higher than any realistic delay (e.g., 10ms or 100ms in picoseconds).
//     // 1 ms = 1,000,000,000 ps
//     double reliability_cost = s->penalty * weight;

//     double d = s->smoothed_delay / 1e6f;
//     return (d * capacity_multiplier) + reliability_cost;
// }

uint16_t SigcommLoadBalancing::advance() {
    uint16_t mask = _no_of_paths - 1;

    uint16_t res = (_current_ev_index ^ _path_xor) & mask;

    _current_ev_index++;
    if (_current_ev_index == _no_of_paths) {
        _current_ev_index = 0;
        _path_xor = rand() & mask;
    }

    return res;
}

// uint16_t SigcommLoadBalancing::nextEntropy(uint64_t seq_sent, uint64_t cur_cwnd_in_pkts) {
//     double cost = std::numeric_limits<double>::max();
//     uint16_t best = 0;

//     for (unsigned int i = 0; i < 2; i++) {
//         const uint16_t path_id = advance();
//         const auto s = &_ev_states[path_id];

//         double e = eval(s);
//         if (e < cost) {
//             best = path_id;
//             cost = e;
//         }
//     }

//     return best | _path_random;
// }

SigcommLoadBalancing::SigcommLoadBalancing(uint16_t no_of_paths, bool debug)
    : UecMultipath(debug),
      _no_of_paths(no_of_paths),
      _mask(no_of_paths - 1),
      _current_ev_index(0),
      _ev_skip_bitmap(no_of_paths, 0),  // Initialize with size and 0
      _max_penalty(15)
{
    _path_random = rand() & UINT16_MAX;
    _path_xor = rand() & _mask;

    _ev_skip_bitmap.resize(_no_of_paths);
    for (uint32_t i = 0; i < _no_of_paths; i++) {
        _ev_skip_bitmap[i] = 0;
    }

    if (_debug)
        cout << "Multipath"
            << " Bitmap"
            << " _no_of_paths " << _no_of_paths
            << " _path_random " << _path_random
            << " _path_xor " << _path_xor
            << " _max_penalty " << (uint32_t)_max_penalty
            << endl;
}

void SigcommLoadBalancing::processEv(uint16_t path_id, PathFeedback feedback, simtime_picosec) {
    uint16_t mask = _no_of_paths - 1;
    path_id &= mask;

    // Heal the path on good feedback
    if (feedback == PathFeedback::PATH_GOOD) {
        if (_ev_skip_bitmap[path_id] > 0) {
            _ev_skip_bitmap[path_id]--;
        }
        return; // No penalty to add
    }

    uint8_t penalty = 0;
    if (feedback == PathFeedback::PATH_ECN)
        penalty = 1;
    else if (feedback == PathFeedback::PATH_NACK)
        penalty = 4;
    else if (feedback == PathFeedback::PATH_TIMEOUT)
        penalty = _max_penalty;

    // Saturating add
    int new_val = _ev_skip_bitmap[path_id] + penalty;
    if (new_val > _max_penalty) new_val = _max_penalty;
    _ev_skip_bitmap[path_id] = (uint8_t)new_val;
}

uint16_t SigcommLoadBalancing::nextEntropy(uint64_t seq_sent, uint64_t cur_cwnd_in_pkts) {
    uint16_t mask = _no_of_paths - 1;
    uint16_t res = advance();
    uint16_t best_score = _ev_skip_bitmap[res];

    if (best_score > 0) {
        uint16_t new_try = rand() & mask;
        if (_ev_skip_bitmap[new_try] < best_score) {
            res = new_try;
        }
    }

    // Construct the final entropy (Original upper bits logic)
    uint16_t entropy = res;
    entropy |= _path_random ^ (_path_random & mask); 

    return entropy;
}
