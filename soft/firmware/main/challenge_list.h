#pragma once

// Expression quest challenge
namespace EQuest {
enum ChResults {
    CR_Ok       = -1, // User entered answer right
    CR_Timeout  = -2, // Timeout hit (do logout)
    CR_Error    = -3  // Can't generate
    // >=0 - Value to match. User press 'help' icon
};

// Result - ChResults or value to pass to MultiSelect dialog
int run_challenge();
}
