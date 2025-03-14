#include <drogon/drogon_test.h>
#include <drogon/utils/Utilities.h>
#include <string>
#include <iostream>

DROGON_TEST(ZstdCompressionTest)
{
#ifdef USE_ZSTD
    SUBSECTION(EmptyString)
    {
        std::string source("");
        auto compressed = drogon::utils::zstdCompress(source.data(), source.length());
        CHECK(compressed.empty());  // Empty input should produce empty output
        auto decompressed = drogon::utils::zstdDecompress(compressed.data(), compressed.length());
        CHECK(decompressed == source);
    }

    SUBSECTION(ShortText)
    {
        std::string source("shortTextForTesting");
        auto compressed = drogon::utils::zstdCompress(source.data(), source.length());
        CHECK(!compressed.empty());  // Compression should produce something
        auto decompressed = drogon::utils::zstdDecompress(compressed.data(), compressed.length());
        CHECK(decompressed == source);
    }

    SUBSECTION(LongText)
    {
        std::string source;
        for (size_t i = 0; i < 100000; ++i)
        {
            source.append(std::to_string(i));
        }
        auto compressed = drogon::utils::zstdCompress(source.data(), source.length());
        CHECK(!compressed.empty());
        auto decompressed = drogon::utils::zstdDecompress(compressed.data(), compressed.length());
        CHECK(decompressed == source);
    }

    SUBSECTION(SmallInput)
    {
        std::string source("a");  // Very small input
        auto compressed = drogon::utils::zstdCompress(source.data(), source.length());
        CHECK(!compressed.empty());
        auto decompressed = drogon::utils::zstdDecompress(compressed.data(), compressed.length());
        CHECK(decompressed == source);
    }
#else
    std::cout << "Zstd not enabled, skipping ZstdCompressionTest" << std::endl;
#endif
}