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

using namespace drogon;

void drogon::ZstdFilter::doFilter(const HttpRequestPtr &req,
                                  FilterCallback &&fcb,
                                  FilterChainCallback &&fccb)
{
    if (req->method() == drogon::Post && this->shouldCompress(req))
    {
        // Handle request body compression
        auto body = req->getBody();
        if (!body.empty())
        {
            try
            {
                std::string decompressed =
                    decompressData(std::string(body.data(), body.length()));
                req->setBody(decompressed);
            }
            catch (const std::exception &e)
            {
                auto resp = drogon::HttpResponse::newHttpResponse();
                resp->setStatusCode(drogon::k400BadRequest);
                resp->setBody("Invalid compressed data");
                fcb(resp);
                return;
            }
        }
    }

    // Modify the filter callback to handle response compression
    fccb();
    fcb = [this, fcb = std::move(fcb)](const drogon::HttpResponsePtr &resp) {
        if (this->shouldCompress(resp))
        {
            auto body = resp->body();
            if (!body.empty())
            {
                try
                {
                    std::string compressed =
                        compressData(std::string(body.data(), body.length()));
                    resp->setBody(compressed);
                    resp->addHeader("Content-Encoding", "zstd");
                }
                catch (const std::exception &e)
                {
                    // If compression fails, send uncompressed
                    LOG_ERROR << "Compression failed: " << e.what();
                }
            }
        }
        fcb(resp);
    };
}

std::string ZstdFilter::compressData(const std::string &data)
{
    size_t const cBuffSize = ZSTD_compressBound(data.size());
    std::string compressed(cBuffSize, '\0');

    size_t const cSize = ZSTD_compress(
        &compressed[0], cBuffSize, data.data(), data.size(), COMPRESSION_LEVEL);

    if (ZSTD_isError(cSize))
    {
        throw std::runtime_error(std::string("Compression error: ") +
                                 ZSTD_getErrorName(cSize));
    }

    compressed.resize(cSize);
    return compressed;
}

std::string ZstdFilter::decompressData(const std::string &data)
{
    unsigned long long const rSize =
        ZSTD_getFrameContentSize(data.data(), data.size());

    if (rSize == ZSTD_CONTENTSIZE_ERROR || rSize == ZSTD_CONTENTSIZE_UNKNOWN)
    {
        throw std::runtime_error("Invalid compressed data");
    }

    std::string decompressed(rSize, '\0');
    size_t const dSize =
        ZSTD_decompress(&decompressed[0], rSize, data.data(), data.size());

    if (ZSTD_isError(dSize))
    {
        throw std::runtime_error(std::string("Decompression error: ") +
                                 ZSTD_getErrorName(dSize));
    }

    decompressed.resize(dSize);
    return decompressed;
}

bool ZstdFilter::shouldCompress(const drogon::HttpResponsePtr &resp)
{
    auto contentType = resp->getContentType();
    return (contentType == drogon::CT_APPLICATION_JSON ||
            contentType == drogon::CT_TEXT_PLAIN ||
            contentType == drogon::CT_TEXT_HTML) &&
           resp->body().length() >
               1024;  // Only compress responses larger than 1KB
}

std::string ZstdFilter::compressStream(std::istream &input)
{
    ZSTD_CCtx *cctx = ZSTD_createCCtx();
    if (!cctx) {
        throw std::runtime_error("Failed to create ZSTD compression context");
    }

    std::ostringstream output;

    const size_t bufferSize = ZSTD_CStreamOutSize();
    std::vector<char> inBuffer(bufferSize);
    std::vector<char> outBuffer(bufferSize);

    ZSTD_inBuffer inputBuffer = {inBuffer.data(), 0, 0};
    ZSTD_outBuffer outputBuffer = {outBuffer.data(), outBuffer.size(), 0};

    while (input.good()) {
        input.read(inBuffer.data(), bufferSize);
        inputBuffer.size = input.gcount();
        inputBuffer.pos = 0;

        while (inputBuffer.pos < inputBuffer.size) {
            outputBuffer.pos = 0;
            size_t ret = ZSTD_compressStream2(cctx, &outputBuffer, &inputBuffer, ZSTD_e_continue);
            if (ZSTD_isError(ret)) {
                ZSTD_freeCCtx(cctx);
                throw std::runtime_error(std::string("Compression error: ") + ZSTD_getErrorName(ret));
            }
            output.write(outBuffer.data(), outputBuffer.pos);
        }
    }

    outputBuffer.pos = 0;
    size_t ret = ZSTD_endStream(cctx, &outputBuffer);
    if (ZSTD_isError(ret)) {
        ZSTD_freeCCtx(cctx);
        throw std::runtime_error(std::string("Compression error: ") + ZSTD_getErrorName(ret));
    }
    output.write(outBuffer.data(), outputBuffer.pos);

    ZSTD_freeCCtx(cctx);
}

std::string ZstdFilter::decompressStream(std::istream &input)
{
    ZSTD_DCtx *dctx = ZSTD_createDCtx();
    if (!dctx) {
        throw std::runtime_error("Failed to create ZSTD decompression context");
    }

    std::ostringstream output;

    const size_t bufferSize = ZSTD_DStreamOutSize();
    std::vector<char> inBuffer(bufferSize);
    std::vector<char> outBuffer(bufferSize);

    ZSTD_inBuffer inputBuffer = {inBuffer.data(), 0, 0};
    ZSTD_outBuffer outputBuffer = {outBuffer.data(), outBuffer.size(), 0};

    while (input.good()) {
        input.read(inBuffer.data(), bufferSize);
        inputBuffer.size = input.gcount();
        inputBuffer.pos = 0;

        while (inputBuffer.pos < inputBuffer.size) {
            outputBuffer.pos = 0;
            size_t ret = ZSTD_decompressStream(dctx, &outputBuffer, &inputBuffer);
            if (ZSTD_isError(ret)) {
                ZSTD_freeDCtx(dctx);
                throw std::runtime_error(std::string("Decompression error: ") + ZSTD_getErrorName(ret));
            }
            output.write(outBuffer.data(), outputBuffer.pos);
        }
    }

    ZSTD_freeDCtx(dctx);
}

std::string ZstdFilter::advancedCompressData(const std::string &data)
{
    ZSTD_CCtx *cctx = ZSTD_createCCtx();
    if (!cctx)
    {
        throw std::runtime_error("Failed to create ZSTD compression context");
    }

    size_t bound = ZSTD_compressBound(data.size());
    std::string compressedData(bound, '\0');

    size_t compressedSize = ZSTD_compressCCtx(cctx,
                                              &compressedData[0],
                                              compressedData.size(),
                                              data.data(),
                                              data.size(),
                                              COMPRESSION_LEVEL);

    ZSTD_freeCCtx(cctx);

    if (ZSTD_isError(compressedSize))
    {
        throw std::runtime_error(std::string("Compression error: ") +
                                 ZSTD_getErrorName(compressedSize));
    }

    compressedData.resize(compressedSize);
    return compressedData;
}

std::string ZstdFilter::advancedDecompressData(const std::string &data)
{
    ZSTD_DCtx *dctx = ZSTD_createDCtx();
    if (!dctx)
    {
        throw std::runtime_error("Failed to create ZSTD decompression context");
    }

    size_t capacity = ZSTD_getFrameContentSize(data.data(), data.size());
    if (capacity == ZSTD_CONTENTSIZE_ERROR)
    {
        ZSTD_freeDCtx(dctx);
        throw std::runtime_error("Invalid data for decompression");
    }
    else if (capacity == ZSTD_CONTENTSIZE_UNKNOWN)
    {
        ZSTD_freeDCtx(dctx);
        throw std::runtime_error("Original size unknown");
    }

    std::string decompressedData(capacity, '\0');
    size_t decompressedSize = ZSTD_decompressDCtx(
        dctx, &decompressedData[0], capacity, data.data(), data.size());

    ZSTD_freeDCtx(dctx);

    if (ZSTD_isError(decompressedSize))
    {
        throw std::runtime_error(std::string("Decompression error: ") +
                                 ZSTD_getErrorName(decompressedSize));
    }

    decompressedData.resize(decompressedSize);
    return decompressedData;
}

std::string ZstdFilter::advancedCompressStream(std::istream &input)
{
    ZSTD_CCtx *cctx = ZSTD_createCCtx();
    if (!cctx)
    {
        throw std::runtime_error("Failed to create ZSTD compression context");
    }

    std::ostringstream output;

    const size_t bufferSize = ZSTD_CStreamOutSize();
    std::vector<char> inBuffer(bufferSize);
    std::vector<char> outBuffer(bufferSize);

    ZSTD_inBuffer inputBuffer = {inBuffer.data(), 0, 0};
    ZSTD_outBuffer outputBuffer = {outBuffer.data(), outBuffer.size(), 0};

    while (input.good())
    {
        input.read(inBuffer.data(), bufferSize);
        inputBuffer.size = input.gcount();
        inputBuffer.pos = 0;

        while (inputBuffer.pos < inputBuffer.size)
        {
            outputBuffer.pos = 0;
            size_t ret = ZSTD_compressStream(cctx, &outputBuffer, &inputBuffer);
            if (ZSTD_isError(ret))
            {
                ZSTD_freeCCtx(cctx);
                throw std::runtime_error(std::string("Compression error: ") +
                                         ZSTD_getErrorName(ret));
            }
            output.write(outBuffer.data(), outputBuffer.pos);
        }
    }

    outputBuffer.pos = 0;
    size_t ret = ZSTD_endStream(cctx, &outputBuffer);
    if (ZSTD_isError(ret))
    {
        ZSTD_freeCCtx(cctx);
        throw std::runtime_error(std::string("Compression error: ") +
                                 ZSTD_getErrorName(ret));
    }
    output.write(outBuffer.data(), outputBuffer.pos);

    ZSTD_freeCCtx(cctx);
}

std::string ZstdFilter::advancedDecompressStream(std::istream &input)
{
    ZSTD_DCtx *dctx = ZSTD_createDCtx();
    if (!dctx)
    {
        throw std::runtime_error("Failed to create ZSTD decompression context");
    }

    std::ostringstream output;

    const size_t bufferSize = ZSTD_DStreamOutSize();
    std::vector<char> inBuffer(bufferSize);
    std::vector<char> outBuffer(bufferSize);

    ZSTD_inBuffer inputBuffer = {inBuffer.data(), 0, 0};
    ZSTD_outBuffer outputBuffer = {outBuffer.data(), outBuffer.size(), 0};

    while (input.good())
    {
        input.read(inBuffer.data(), bufferSize);
        inputBuffer.size = input.gcount();
        inputBuffer.pos = 0;

        while (inputBuffer.pos < inputBuffer.size)
        {
            outputBuffer.pos = 0;
            size_t ret =
                ZSTD_decompressStream(dctx, &outputBuffer, &inputBuffer);
            if (ZSTD_isError(ret))
            {
                ZSTD_freeDCtx(dctx);
                throw std::runtime_error(std::string("Decompression error: ") +
                                         ZSTD_getErrorName(ret));
            }
            output.write(outBuffer.data(), outputBuffer.pos);
        }
    }

    ZSTD_freeDCtx(dctx);
}

