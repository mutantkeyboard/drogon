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

#include "ZstdFilter.h"
#include <drogon/HttpAppFramework.h>
#include <drogon/drogon.h>
#include <zstd.h>
#include <string>
#include <memory>
#include <fstream>

using namespace drogon;

std::vector<char> drogon::ZstdFilter::basicCompress(const char *data,
                                                    size_t size)
{
    size_t maxSize = ZSTD_compressBound(size);
    std::vector<char> compressed(maxSize);
    size_t compressedSize = ZSTD_compress(
        compressed.data(), maxSize, data, size, compressionLevel_);

    if (ZSTD_isError(compressedSize))
    {
        LOG_ERROR << "Zstd compression error: "
                  << ZSTD_getErrorName(compressedSize);
        return std::vector<char>();
    }

    compressed.resize(compressedSize);
    return compressed;
}

std::vector<char> drogon::ZstdFilter::advancedCompress(const char *data,
                                                       size_t size)
{
    if (!cctx_)
        initCompressionContext();

    size_t maxSize = ZSTD_compressBound(size);
    std::vector<char> compressed(maxSize);

    // Using ZSTD_compress2 for more control over the compression process
    size_t compressedSize =
        ZSTD_compress2(cctx_, compressed.data(), maxSize, data, size);

    if (ZSTD_isError(compressedSize))
    {
        LOG_ERROR << "Zstd compression error: "
                  << ZSTD_getErrorName(compressedSize);
        return std::vector<char>();
    }

    compressed.resize(compressedSize);

    return compressed;
}

std::vector<char> drogon::ZstdFilter::compressWithDict(const char *data,
                                                       size_t size)
{
    if (!cDict_)
    {
        LOG_ERROR << "Zstd dictionary not initialized";
        return std::vector<char>();
    }
    size_t maxSize = ZSTD_compressBound(size);
    std::vector<char> compressed(maxSize);

    size_t compressedSize = ZSTD_compress_usingCDict(
        cctx_, compressed.data(), maxSize, data, size, cDict_);

    if (ZSTD_isError(compressedSize))
    {
        LOG_ERROR << "Zstd compression error: "
                  << ZSTD_getErrorName(compressedSize);
        return std::vector<char>();
    }

    compressed.resize(compressedSize);
    return compressed;
}

std::vector<char> drogon::ZstdFilter::basicDecompress(const char *data,
                                                      size_t size)
{
    size_t decompressedSize = ZSTD_getFrameContentSize(data, size);
    if (decompressedSize == ZSTD_CONTENTSIZE_ERROR)
    {
        LOG_ERROR << "Zstd decompression error: invalid frame content size";
        return std::vector<char>();
    }
    if (decompressedSize == ZSTD_CONTENTSIZE_UNKNOWN)
    {
        LOG_ERROR << "Zstd decompression error: unknown frame content size";
        return std::vector<char>();
    }

    std::vector<char> decompressed(decompressedSize);
    size_t actualSize =
        ZSTD_decompress(decompressed.data(), decompressedSize, data, size);
    if (ZSTD_isError(actualSize))
    {
        LOG_ERROR << "Zstd decompression error: "
                  << ZSTD_getErrorName(actualSize);
        return std::vector<char>();
    }

    decompressed.resize(actualSize);
    return decompressed;
}

std::vector<char> drogon::ZstdFilter::advancedDecompress(const char *data,
                                                         size_t size)
{
    if (!dctx_)
        initCompressionContext();

    size_t decompressedSize = ZSTD_getFrameContentSize(data, size);
    if (decompressedSize == ZSTD_CONTENTSIZE_ERROR)
    {
        LOG_ERROR << "Zstd decompression error: invalid frame content size";
        return std::vector<char>();
    }
    if (decompressedSize == ZSTD_CONTENTSIZE_UNKNOWN)
    {
        LOG_ERROR << "Zstd decompression error: unknown frame content size";
        return std::vector<char>();
    }

    std::vector<char> decompressed(decompressedSize);
    size_t actualSize = ZSTD_decompressDCtx(
        dctx_, decompressed.data(), decompressedSize, data, size);
    if (ZSTD_isError(actualSize))
    {
        LOG_ERROR << "Zstd decompression error: "
                  << ZSTD_getErrorName(actualSize);
        return std::vector<char>();
    }
    decompressed.resize(actualSize);
    return decompressed;
}

std::vector<char> drogon::ZstdFilter::decompressWithDict(const char *data,
                                                         size_t size)
{
    if (!dDict_)
    {
        LOG_ERROR << "Zstd dictionary not initialized";
        return std::vector<char>();
    }

    size_t decompressedSize = ZSTD_getFrameContentSize(data, size);
    if (decompressedSize == ZSTD_CONTENTSIZE_ERROR)
    {
        LOG_ERROR << "Zstd decompression error: invalid frame content size";
        return std::vector<char>();
    }
    if (decompressedSize == ZSTD_CONTENTSIZE_UNKNOWN)
    {
        LOG_ERROR << "Zstd decompression error: unknown frame content size";
        return std::vector<char>();
    }

    std::vector<char> decompressed(decompressedSize);
    size_t actualSize = ZSTD_decompress_usingDDict(
        dctx_, decompressed.data(), decompressedSize, data, size, dDict_);
    if (ZSTD_isError(actualSize))
    {
        LOG_ERROR << "Zstd decompression error: "
                  << ZSTD_getErrorName(actualSize);
        return std::vector<char>();
    }

    decompressed.resize(actualSize);
    return decompressed;
}

void drogon::ZstdFilter::initCompressionContext()
{
    cleanupCompressionContext();

    cctx_ = ZSTD_createCCtx();
    dctx_ = ZSTD_createDCtx();

    if (!cctx_ || !dctx_)
    {
        LOG_ERROR << "Failed to create Zstd compression context";
        exit(1);
    }

    ZSTD_CCtx_setParameter(cctx_, ZSTD_c_compressionLevel, compressionLevel_);
    ZSTD_CCtx_setParameter(cctx_,
                           ZSTD_c_checksumFlag,
                           1);  // enable content checksum
}

void drogon::ZstdFilter::cleanupCompressionContext()
{
    if (cctx_)
        ZSTD_freeCCtx(cctx_);
    if (dctx_)
        ZSTD_freeDCtx(dctx_);
    if (cDict_)
        ZSTD_freeCDict(cDict_);
    if (dDict_)
        ZSTD_freeDDict(dDict_);

    cctx_ = nullptr;
    dctx_ = nullptr;
    cDict_ = nullptr;
    dDict_ = nullptr;
}

bool drogon::ZstdFilter::shouldCompress(
    const drogon::HttpRequestPtr &req,
    const drogon::HttpResponsePtr &resp) const
{
    if (!acceptZstd(req))
        return false;

    if (resp->getHeader("Content-Encoding") != "")
    {
        return false;
    }

    auto contentType = resp->getContentType();
    static const std::unordered_set<drogon::ContentType> compressibleTypes = {
        drogon::CT_TEXT_PLAIN,
        drogon::CT_TEXT_HTML,
        drogon::CT_APPLICATION_JSON,
        drogon::CT_APPLICATION_XML,
        drogon::CT_TEXT_XSL,
        drogon::CT_TEXT_JAVASCRIPT,
        drogon::CT_TEXT_CSS,
        drogon::CT_APPLICATION_X_JAVASCRIPT};

    return compressibleTypes.count(contentType) > 0;
}

bool drogon::ZstdFilter::acceptZstd(const drogon::HttpRequestPtr &req) const
{
    auto acceptEncoding = req->getHeader("Accept-Encoding");
    return acceptEncoding.find("zstd") != std::string::npos;
}

drogon::ZstdFilter::~ZstdFilter()
{
    cleanupCompressionContext();
}

void drogon::ZstdFilter::doFilter(const HttpRequestPtr &req,
                                  FilterCallback &&fcb,
                                  FilterChainCallback &&fccb)
{
    if (req->method() != drogon::HttpMethod::Post || drogon::HttpMethod::Put ||
        drogon::HttpMethod::Patch)
    {
        fccb();
        return;
    }
    try
    {
        auto body = req->getBody();
        std::vector<char> compressedData;

        auto mode = req->getHeader("X-Compression-Mode");
        if (mode == "dict")
        {
            compressedData = compressWithDict(body.data(), body.length());
        }
        else if (mode == "advanced")
        {
            compressedData = advancedCompress(body.data(), body.length());
        }
        else
        {
            compressedData = basicCompress(body.data(), body.length());
        }

        req->setBody(std::string(compressedData.data(), compressedData.size()));
        req->addHeader("Content-Length", std::to_string(compressedData.size()));
        req->addHeader("X-Compression-Mode", mode);
        req->addHeader("Content-Encoding", "zstd");
        fccb();
    }
    catch (const std::exception &ex)
    {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::HttpStatusCode::k400BadRequest);
        resp->setBody("Compression error: " + std::string(ex.what()));
        fcb(resp);
    }
}

bool drogon::ZstdFilter::initWithDict(const std::string &dictPath,
                                      int compressionLevel)
{
    compressionLevel_ = compressionLevel;

    // Load the dictionary
    std::ifstream dictFile(dictPath, std::ios::binary | std::ios::ate);
    if (!dictFile)
    {
        LOG_ERROR << "Failed to open Zstd dictionary file: " << dictPath;
        return false;
    }

    // Read the dictionary file into memory
    size_t dictSize = dictFile.tellg();
    dictFile.seekg(0, std::ios::beg);

    dictData_.resize(dictSize);
    dictFile.read(dictData_.data(), dictSize);

    // Create a Zstd dictionary from the loaded data
    cDict_ = ZSTD_createCDict(dictData_.data(), dictSize, compressionLevel_);
    if (!cDict_)
    {
        LOG_ERROR << "Failed to create Zstd compression dictionary";
        return false;
    }

    dDict_ = ZSTD_createDDict(dictData_.data(), dictSize);
    if (!dDict_)
    {
        ZSTD_freeCDict(cDict_);
        cDict_ = nullptr;
        LOG_ERROR << "Failed to create Zstd decompression dictionary";
        return false;
    }

    initCompressionContext();
    return true;
}
