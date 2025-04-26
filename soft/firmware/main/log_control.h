#pragma once

class Ans;

// Start ExtLog system. Can be called multiple times
void process_initial_log_setup();

// Process JSON with Log setup from WEB (action LogSystemSet)
void process_log_setup(const char* json);

// Process WEB Var LogSystemSetup
void log_send_setup(Ans& ans);

// Status request (action LogSystemStatus)
void log_send_status(Ans& ans, bool with_clear);

// Accumulated log (as HTML piece). Action LogSystemLogData
void log_send_data(Ans& ans);
