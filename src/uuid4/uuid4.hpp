#pragma once

#include <string>

/// \brief Generates a random UUID v4 string (for widget ids and resource file_id).
/// Not thread-safe (a lone static engine); the tool is single-threaded.
/// \return The UUID v4 in canonical string form.
std::string makeUuid4();
