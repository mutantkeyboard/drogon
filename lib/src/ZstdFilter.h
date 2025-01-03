/**
 *
 *  @file ZstdFilter.h
 *  @author Antonio Nesic
 *
 *  Copyright 2024, Antonio Nesic.  All rights reserved.
 *  https://github.com/an-tao/drogon
 *  Use of this source code is governed by a MIT license
 *  that can be found in the License file.
 *
 *  Drogon
 *
 */

#pragma once

#include <drogon/HttpFilter.h>
#include <zstd.h>

namespace drogon
{
class DROGON_EXPORT ZstdFilter : public HttpFilter<ZstdFilter>
{
  public:
    ZstdFilter() = default;
    virtual ~ZstdFilter();
    void doFilter(const HttpRequestPtr &req,
                  FilterCallback &&fcb,
                  FilterChainCallback &&fccb) override;

    bool initWithDict(const std::string &dictPath, int compressionLevel = 3);

  private:
    int compressionLevel_ = 3;  // this can be adjusted
    size_t maxCompressionSize_{0};

    // Zstd compression context
    ZSTD_CCtx *cctx_{nullptr};
    ZSTD_DCtx *dctx_{nullptr};

    // Zstd dictionary support
    ZSTD_CDict *cDict_{nullptr};
    ZSTD_DDict *dDict_{nullptr};
    std::vector<char> dictData_;

    // Zstd compression methods
    // NOTE: We're using std::vector<char> over std::string to allow
    // preservation of iterators, references, etc Refer to:
    // https://stackoverflow.com/a/12609370/1737811
    std::vector<char> basicCompress(const char *data, size_t size);
    std::vector<char> advancedCompress(const char *data, size_t size);
    std::vector<char> compressWithDict(const char *data, size_t size);

    // Zstd decompression methods
    std::vector<char> basicDecompress(const char *data, size_t size);
    std::vector<char> advancedDecompress(const char *data, size_t size);
    std::vector<char> decompressWithDict(const char *data, size_t size);

    // utility methods
    void initCompressionContext();
    void cleanupCompressionContext();
};
}  // namespace drogon
