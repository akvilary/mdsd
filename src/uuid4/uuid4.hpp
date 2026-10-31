#pragma once

#include <string>

// Генерация случайного UUID v4 (для id виджетов и file_id ресурсов).
// Не потокобезопасно (одинокий static движок); инструмент однопоточный.
std::string makeUuid4();
