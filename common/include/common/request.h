#include "Poco/JSON/Parser.h"
#include "Poco/Net/HTTPClientSession.h"
#include "Poco/Net/HTTPRequest.h"
#include "Poco/Net/HTTPResponse.h"
#include "Poco/StreamCopier.h"
#include "Poco/URI.h"
#include "Poco/Util/Application.h"
#include "common/handler_wrapper.h"

#include <cstdlib>
#include <cstring>
#include <fmt/core.h>
#include <iostream>
#include <sstream>
#include <unordered_map>

inline std::string
makeRequest(const Poco::URI &uri,
            std::unordered_map<std::string, std::string> headers = {},
            std::string method = Poco::Net::HTTPRequest::HTTP_GET) {
  Poco::Net::HTTPClientSession session(uri.getHost(), uri.getPort());
  std::string path(uri.getPathAndQuery());
  if (path.empty())
    path = "/";
  Poco::Net::HTTPRequest req(method, path, Poco::Net::HTTPMessage::HTTP_1_1);
  for (const auto &[header, value] : headers) {
    req.add(header, value);
  }
  std::ostream &os = session.sendRequest(req);
  Poco::Net::HTTPResponse res;
  Poco::Util::Application::instance().logger().information(
      "%d %s", res.getStatus(), res.getReason());

  std::istream &is = session.receiveResponse(res);
  std::stringstream ss;
  Poco::StreamCopier::copyStream(is, ss);
  return ss.str();
}

inline bool validateAuth(std::string token, int64_t &id) {
  Poco::URI uri(std::getenv("AUTH_HOST"));
  uri.setPath("/auth/v1/validate");
  uri.addQueryParameter("token", token);
  auto res = makeRequest(uri, {{"X-API-KEY", std::getenv("AUTH_TOKEN")}},
                         Poco::Net::HTTPRequest::HTTP_POST);
  Poco::JSON::Parser parser;
  auto result = parser.parse(res);
  auto ptr = result.extract<Poco::JSON::Object::Ptr>();
  if (ptr->getValue<bool>("valid")) {
    id = ptr->getValue<int64_t>("id");
    return true;
  } else {
    return false;
  }
}

inline int64_t validateBearer(Poco::Net::HTTPServerRequest &request) {
  if (!request.has("Authorization")) {
    throw UnauthorizedException();
  }
  auto h = request.get("Authorization");
  if (!h.starts_with("Bearer ")) {
    throw UnauthorizedException();
  }
  int64_t res;
  if (!validateAuth(h.substr(std::strlen("Bearer ")), res)) {
    throw UnauthorizedException();
  }
  return res;
}

inline void validateBasic(Poco::Net::HTTPRequest &request) {
  if (!request.has("X-API-KEY") ||
      request.get("X-API-KEY") != std::getenv("AUTH_TOKEN")) {
    throw UnauthorizedException();
  }
}