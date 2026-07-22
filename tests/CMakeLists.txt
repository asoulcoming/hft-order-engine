# ── PriceLevel tests ────────────────────────────────────────
add_executable(test_price_level
    test_price_level.cpp
    ../src/book/order_book.cpp
    ../src/io/parser.cpp
    ../src/io/formatter.cpp
)
target_include_directories(test_price_level PRIVATE ../include)
target_link_libraries(test_price_level PRIVATE GTest::gtest_main)
target_compile_options(test_price_level PRIVATE -Wall -Wextra -Wpedantic)
gtest_discover_tests(test_price_level)

# ── OrderBook tests ─────────────────────────────────────────
add_executable(test_orderbook
    test_orderbook.cpp
    ../src/book/order_book.cpp
    ../src/io/parser.cpp
    ../src/io/formatter.cpp
)
target_include_directories(test_orderbook PRIVATE ../include)
target_link_libraries(test_orderbook PRIVATE GTest::gtest_main)
target_compile_options(test_orderbook PRIVATE -Wall -Wextra -Wpedantic)
gtest_discover_tests(test_orderbook)

# ── Parser tests ────────────────────────────────────────────
add_executable(test_parser
    test_parser.cpp
    ../src/io/parser.cpp
    ../src/book/order_book.cpp
    ../src/io/formatter.cpp
)
target_include_directories(test_parser PRIVATE ../include)
target_link_libraries(test_parser PRIVATE GTest::gtest_main)
target_compile_options(test_parser PRIVATE -Wall -Wextra -Wpedantic)
gtest_discover_tests(test_parser)
