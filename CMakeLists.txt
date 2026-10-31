cmake_minimum_required(VERSION 3.16)
project(mdsd LANGUAGES CXX DESCRIPTION "Markdown <-> SabyDoc converter")

set(CMAKE_CXX_STANDARD 20) # minimum supported version
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# Warning flags — only for our targets (googletest is included separately)
set(MDSD_WARN_FLAGS -Wall -Wextra -Wpedantic)

# Libraries (header-only, via FetchContent): JSON, UTF-8, UUID.
# The mdsd binary gets no link-time dependencies.
# IMPORTANT: this block must be above add_executable — MDSD_LIB_DEPS is used when building mdsd.
include(FetchContent)

set(JSON_BuildTests OFF CACHE BOOL "" FORCE)
set(JSON_Install OFF CACHE BOOL "" FORCE)
FetchContent_Declare(nlohmann_json
  URL https://github.com/nlohmann/json/archive/refs/tags/v3.11.3.zip
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
FetchContent_MakeAvailable(nlohmann_json)

# Include paths are set manually: deterministic, no dependency on external CMake
FetchContent_Declare(utf8cpp
  URL https://github.com/nemtrif/utfcpp/archive/refs/tags/v4.2.1.zip
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
FetchContent_MakeAvailable(utf8cpp)
add_library(mdsd_utf8 INTERFACE)
target_include_directories(mdsd_utf8 INTERFACE ${utf8cpp_SOURCE_DIR}/source)

# Suppress third-party deprecation warnings (stduuid uses old cmake_minimum_required)
set(CMAKE_WARN_DEPRECATED OFF CACHE BOOL "" FORCE)

FetchContent_Declare(stduuid
  URL https://github.com/mariusbancila/stduuid/archive/refs/tags/v1.2.3.zip
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
FetchContent_MakeAvailable(stduuid)
add_library(mdsd_stduuid INTERFACE)
target_include_directories(mdsd_stduuid INTERFACE ${stduuid_SOURCE_DIR}/include)

set(MDSD_LIB_DEPS nlohmann_json::nlohmann_json mdsd_utf8 mdsd_stduuid)

set(MDSD_LIB_SOURCES
  src/utf8/impl/utf8.cpp
  src/zip_writer/impl/zip_writer.cpp  src/uuid4/impl/uuid4.cpp
  src/image_size/impl/image_size.cpp
  src/markdown/ast/impl/ast.cpp
  src/markdown/parser/impl/parser.cpp
  src/markdown/parser/chars/impl/chars.cpp
  src/markdown/parser/chars/link_def/impl/link_def.cpp
  src/markdown/parser/chars/marker/impl/marker.cpp
  src/markdown/parser/chars/entity/impl/entity.cpp
  src/markdown/parser/chars/html_tag/impl/html_tag.cpp
  src/markdown/parser/chars/detectors/impl/detectors.cpp
  src/markdown/parser/chars/table_row/impl/table_row.cpp
  src/markdown/parser/chars/inline_parser/impl/inline_parser_core.cpp
  src/markdown/parser/chars/inline_parser/impl/inline_parser_links.cpp
  src/markdown/parser/chars/inline_parser/impl/inline_parser_markup.cpp
  src/markdown/parser/chars/inline_parser/block_parser/impl/block_parser.cpp
  src/markdown/parser/chars/inline_parser/block_parser/impl/block_parser_input.cpp
  src/markdown/parser/chars/inline_parser/block_parser/impl/block_parser_blocks.cpp
  src/markdown/parser/chars/inline_parser/block_parser/impl/block_parser_align.cpp
  src/markdown/parser/chars/inline_parser/block_parser/impl/block_parser_table.cpp
  src/markdown/parser/chars/inline_parser/block_parser/impl/block_parser_containers.cpp
  src/markdown/parser/chars/inline_parser/block_parser/impl/block_parser_paragraph.cpp
  src/sabydoc/model/text_layout/impl/text_layout.cpp
  src/sabydoc/model/widget_writer/impl/widget_writer_text.cpp
  src/sabydoc/model/widget_writer/impl/widget_writer_blocks.cpp
  src/sabydoc/model/widget_writer/impl/widget_writer_table.cpp
  src/sabydoc/model/widget_writer/impl/widget_writer_frame.cpp
  src/sabydoc/model/widget_writer/document_writer/impl/document_writer.cpp
  src/sabydoc/model/resource_collector/impl/resource_collector.cpp
  src/mapping/md_to_sabydoc/impl/map_blocks.cpp
  src/mapping/md_to_sabydoc/impl/map_content.cpp
  src/mapping/md_to_sabydoc/impl/map_containers.cpp
)

add_executable(mdsd src/impl/main.cpp ${MDSD_LIB_SOURCES})
target_include_directories(mdsd PRIVATE src)
target_link_libraries(mdsd PRIVATE ${MDSD_LIB_DEPS})
target_compile_options(mdsd PRIVATE ${MDSD_WARN_FLAGS})

enable_testing()

# Google Test — самый популярный фреймворк; скачивается при конфигурации,
# касается только тестовой цели (mdsd остаётся без зависимостей).
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
  GoogleTest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(GoogleTest)

set(MDSD_TEST_SOURCES
  tests/utf8/impl/test_utf8.cpp
  tests/sabydoc/model/widget_writer/impl/test_json_output.cpp
  tests/zip_writer/impl/test_zip.cpp
  tests/image_size/impl/test_image_size.cpp
  tests/uuid4/impl/test_uuid4.cpp
  tests/sabydoc/model/text_layout/impl/test_text_layout.cpp
  tests/markdown/parser/chars/inline_parser/impl/test_inline.cpp
  tests/markdown/parser/chars/inline_parser/block_parser/impl/test_block_parser.cpp
  tests/markdown/parser/chars/impl/test_cyrillic.cpp
  tests/e2e/impl/test_e2e.cpp
)
add_executable(mdsd_tests ${MDSD_TEST_SOURCES} ${MDSD_LIB_SOURCES})
target_include_directories(mdsd_tests PRIVATE src tests)
target_compile_options(mdsd_tests PRIVATE ${MDSD_WARN_FLAGS})
target_link_libraries(mdsd_tests PRIVATE GTest::gtest_main ${MDSD_LIB_DEPS})
add_test(NAME unit COMMAND mdsd_tests)

# Интеграционный прогон: конвертация примера + структурный валидатор (проверяет и эталон)
add_test(NAME integration
         COMMAND bash -c
           "$<TARGET_FILE:mdsd> convert ${CMAKE_CURRENT_SOURCE_DIR}/examples/test.md -o ${CMAKE_CURRENT_BINARY_DIR}/e2e.sabydoc && python3 ${CMAKE_CURRENT_SOURCE_DIR}/scripts/validate.py ${CMAKE_CURRENT_BINARY_DIR}/e2e.sabydoc ${CMAKE_CURRENT_SOURCE_DIR}/TestDocument.sabydoc")

include(GNUInstallDirs)
install(TARGETS mdsd RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES docs/mdsd.1 DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)
