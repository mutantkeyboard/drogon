/**
 *  @file ZstdServerTest.cc
 *  @author Antonio Nesic <nesic.antonio@gmail.com>
 *  Copyright 2025, Antonio Nesic. All rights reserved.
 *  Use of this source code is governed by a MIT license.
 */

 #include <drogon/drogon_test.h>
 #include <drogon/HttpAppFramework.h>
 #include <drogon/HttpController.h>
 #include <drogon/utils/Utilities.h>
 
 // Controller to echo back the decompressed body
 class ZstdEchoController : public drogon::HttpController<ZstdEchoController>
 {
 public:
     METHOD_LIST_BEGIN
     ADD_METHOD_TO(ZstdEchoController::echoBody, "/zstd/echo", drogon::Post);
     METHOD_LIST_END
 
     void echoBody(const drogon::HttpRequestPtr &req,
                   std::function<void(const drogon::HttpResponsePtr &)> &&callback)
     {
         auto resp = drogon::HttpResponse::newHttpResponse();
         resp->setBody(std::string(req->getBody()));  // Echo the decompressed body
         callback(resp);
     }
 };
 
 // Controller for Zstd-compressed response
 class ZstdResponseController : public drogon::HttpController<ZstdResponseController>
 {
 public:
     METHOD_LIST_BEGIN
     ADD_METHOD_TO(ZstdResponseController::getCompressed, "/zstd/get", drogon::Get);
     METHOD_LIST_END
 
     void getCompressed(const drogon::HttpRequestPtr &req,
                        std::function<void(const drogon::HttpResponsePtr &)> &&callback)
     {
         std::string body(4994, 'a');  // Fixed size for client test
         auto resp = drogon::HttpResponse::newHttpResponse();
 #ifdef USE_ZSTD
         if (req->getHeader("Accept-Encoding").find("zstd") != std::string::npos)
         {
             resp->setBody(drogon::utils::zstdCompress(body.data(), body.size()));
             resp->addHeader("Content-Encoding", "zstd");
         }
         else
 #endif
         {
             resp->setBody(body);
         }
         callback(resp);
     }
 };
 
 // Minimal test to start the server
 DROGON_TEST(ZstdServerSetup)
 {
 #ifdef USE_ZSTD
     auto &app = drogon::app();
     app.addListener("127.0.0.1", 8850);  // Changed port from 8848
     app.setThreadNum(1);
     app.run();
 
     // Keep server running until manually stopped or client tests complete
     std::cout << "Zstd test server running on port 8850..." << std::endl;
 #else
     std::cout << "Zstd not enabled, skipping ZstdServerSetup" << std::endl;
 #endif
 }
 
 int main(int argc, char** argv)
 {
     trantor::Logger::setLogLevel(trantor::Logger::kDebug);
     int testStatus = drogon::test::run(argc, argv);
     return testStatus;
 }