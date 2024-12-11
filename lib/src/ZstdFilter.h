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
    void doFilter(const HttpRequestPtr &req,
                  FilterCallback &&fcb,
                  FilterChainCallback &&fccb) override;

  private:
    static constexpr int COMPRESSION_LEVEL = 3;  // this can be adjusted
    static constexpr size_t CHUNK_SIZE = 4 * 1024;
    std::string compressData(const std::string &data);
    std::string decompressData(const std::string& data);
    bool shouldCompress(const drogon::HttpResponsePtr &resp);
    bool shouldCompress(const drogon::HttpRequestPtr &req);
};
}  // namespace drogon
