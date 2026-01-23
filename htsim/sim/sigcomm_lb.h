#ifndef SIGCOMM_LB
#define SIGCOMM_LB

#include <cstdint>
#include "uec_mp.h"

class SigcommLoadBalancing : public UecMultipath {
public:
//     SigcommLoadBalancing(uint16_t no_of_paths, bool debug, uint8_t max_penality=255);
//     void processEv(uint16_t path_id, PathFeedback feedback, simtime_picosec delay) override;
//     uint16_t nextEntropy(uint64_t seq_sent, uint64_t cur_cwnd_in_pkts) override;

// private:
//     struct PathState {
//         float smoothed_delay;
//         float congestion;
//         uint8_t penalty;
//         bool explored = false;
//     };

//     constexpr static const float gain = 1.f / 16;
//     constexpr static const float smoothing_factor = 0.125f;
//     constexpr static const float scale = 3.f;

//     // Bigger than Max_Delay * Max_Congestion_Multiplier
//     // It think that is 500ms
//     constexpr static const double weight = 5e5f;

//     uint16_t _no_of_paths;       // must be a power of 2
//     uint16_t _path_random;       // random upper bits of EV, set at startup and never changed

//     uint16_t _path_xor;          // random value set each time we wrap the EV set - XOR with _current_ev_index
//     uint16_t _current_ev_index;  // count through _no_of_paths and then wrap.  XOR with _path_xor

//     uint8_t _max_penalty;        // max value we allow in _path_penalties (typically 1 or 2).

//     vector<PathState> _ev_states; // paths scores for load balancing

//     double eval(PathState *) const;
//     uint16_t advance();
public:
    SigcommLoadBalancing(uint16_t no_of_paths, bool debug);
    
    void processEv(uint16_t path_id, PathFeedback feedback, simtime_picosec) override;
    uint16_t nextEntropy(uint64_t seq_sent, uint64_t cur_cwnd_in_pkts) override;

private:
    uint16_t _no_of_paths;
    uint16_t _mask;               // Precomputed: _no_of_paths - 1
    uint16_t _path_random;        // Upper bits of EV
    uint16_t _path_xor;           // Random permutation key
    uint16_t _current_ev_index;   
    
    vector<uint8_t> _ev_skip_bitmap; 
    uint8_t _max_penalty;
    
    uint16_t advance();
};

#endif
