#include "uuid4/uuid4.hpp"

#include <random>

#include <uuid.h>

// Фасад над stduuid (mariusbancila, std-предложение P0959): генерация v4,
// посев энтропии и биты версий — библиотека. Не потокобезопасно (одинокий
// static движок); инструмент однопоточный.
// ВАЖНО: uuid_random_generator по умолчанию параметризован 32-битным mt19937 —
// движок указываем явно.
std::string makeUuid4() {
    static std::mt19937_64 engine{std::random_device{}()};
    static uuids::basic_uuid_random_generator<std::mt19937_64> generator{engine};
    return uuids::to_string(generator());
}
