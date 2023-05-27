#pragma once

#include "Poco/Base64Decoder.h"
#include "Poco/Base64Encoder.h"
#include "Poco/JWT/Signer.h"
#include "Poco/JWT/Token.h"
#include <istream>
#include <ostream>
#include <sstream>
#include <string>

inline std::string getJWTKey() {
  if (std::getenv("JWT_KEY") != nullptr) {
    return std::getenv("JWT_KEY");
  }
  return "0123456789ABCDEF0123456789ABCDEF";
}

inline std::string generateToken(const int64_t &id) {
  Poco::JWT::Token token;
  token.setType("JWT");
  token.setSubject("auth");
  token.payload().set("id", id);
  token.setIssuedAt(Poco::Timestamp());
  Poco::JWT::Signer signer(getJWTKey());
  return signer.sign(token, Poco::JWT::Signer::ALGO_HS256);
}

inline bool extractPayload(const std::string &jwt_token, int64_t &id) {
  if (jwt_token.length() == 0) {
    return false;
  }

  Poco::JWT::Signer signer(getJWTKey());
  try {
    Poco::JWT::Token token = signer.verify(jwt_token);
    if (token.payload().has("id")) {
      id = token.payload().getValue<int64_t>("id");
      return true;
    }
    std::cout << "Not enough fields in token" << std::endl;

  } catch (...) {
    std::cout << "Token verification failed" << std::endl;
  }
  return false;
}
