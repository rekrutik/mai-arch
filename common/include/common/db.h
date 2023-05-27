#include <cstdlib>
#include <format>
#include <string>

#include "fmt/core.h"

#include "Poco/Data/MySQL/Connector.h"
#include "Poco/Data/MySQL/MySQLException.h"
#include "Poco/Data/SessionFactory.h"
#include "Poco/Data/SessionPool.h"

#pragma once

class Database {

public:
  static Database &instance();

  Poco::Data::Session createSession() const;

private:
  Database();
  std::string conn_str_;
  std::unique_ptr<Poco::Data::SessionPool> pool_;
};
