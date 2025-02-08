#pragma once

enum ChResults {
    CR_Ok       = -1, // User entered answer right
    CR_Timeout  = -2, // Timeout hit (do logout)
    CR_Error    = -3  // Can't generate
    // >=0 - Value to match. User press 'help' icon (used internally - can't be returned by run_challenge)
};

// Expression quest challenge
namespace EQuest {
    // Result - ChResults
    int run_challenge();
}

// Riddle challenge
namespace Riddle {
    // Result - ChResults
    int run_challenge(int ch_index=-1);
}

namespace Game15 {
    // Result - ChResults
    int run_challenge();
}

namespace TileGame {
    // Result - ChResults
    int run_challenge();
}
