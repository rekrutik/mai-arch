#pragma once

#include "Poco/JSON/Object.h"

#include <cinttypes>
#include <optional>
#include <string>

struct User {
  int64_t id;
  std::string name;
  bool is_admin = false;
  bool is_deleted = false;

  void toJson(Poco::JSON::Object::Ptr ptr) const;
};

class UserRepository {
public:
  static UserRepository &instance();
  std::optional<User> getUserById(int64_t id) const;
  std::vector<User> findUsersByName(std::string name) const;
  void deleteUserById(int64_t id) const;
  int64_t createUser(User user) const;

private:
  std::string getShardingHint(int64_t id) const;
  static constexpr size_t MAX_SHARDS = 3;
  UserRepository();
};