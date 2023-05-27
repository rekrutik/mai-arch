#include "common/db.h"
#include <Poco/Util/Application.h>

Database &Database::instance() {
  static Database INSTANCE;
  return INSTANCE;
}

Poco::Data::Session Database::createSession() const {
  return Poco::Data::Session(pool_->get());
}

Database::Database() {
  conn_str_ = fmt::format("host={};port={};db={};user={};password={}",
                          std::getenv("DB_HOST"), std::getenv("DB_PORT"),
                          std::getenv("DB_DB"), std::getenv("DB_USER"),
                          std::getenv("DB_PASSWORD"));
  Poco::Data::MySQL::Connector::registerConnector();
  pool_ = std::make_unique<Poco::Data::SessionPool>(
      Poco::Data::MySQL::Connector::KEY, conn_str_);
}