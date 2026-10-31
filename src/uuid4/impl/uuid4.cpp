#include "uuid4/uuid4.hpp"

#include <random>

#include <uuid.h>

// Facade over stduuid (mariusbancila, std proposal P0959): v4 generation,
// entropy seeding, and version bits come from the library. Not thread-safe
// (a lone static engine); the tool is single-threaded.
// IMPORTANT: uuid_random_generator by default is parameterized with 32-bit mt19937 —
// the engine must be specified explicitly.
std::string makeUuid4() {
    static std::mt19937_64 engine{std::random_device{}()};
    static uuids::basic_uuid_random_generator<std::mt19937_64> generator{engine};
    return uuids::to_string(generator());
}
