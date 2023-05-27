#include "user/user_repository.h"
#include "common/db.h"
#include <algorithm>
#include <optional>

using Poco::Data::Keywords::into;
using Poco::Data::Keywords::range;
using Poco::Data::Keywords::use;

void User::toJson(Poco::JSON::Object::Ptr ptr) const {
  ptr->set("id", id);
  ptr->set("name", name);
  ptr->set("is_admin", is_admin);
}

UserRepository::UserRepository() {}

UserRepository &UserRepository::instance() {
  static UserRepository INSTANCE;
  return INSTANCE;
}

std::optional<User> UserRepository::getUserById(int64_t id) const {
  auto session = Database::instance().createSession();
  try {
    Poco::Data::Statement stmt(session);
    User user;
    std::cout << "hint " << getShardingHint(id) << std::endl;
    user.id = id;
    stmt << "SELECT name, is_admin, is_deleted FROM users WHERE id = "
            "?",
        getShardingHint(id), into(user.name), into(user.is_admin),
        into(user.is_deleted), use(id), range(0, 1);
    stmt.execute();

    if (stmt.rowsExtracted() == 0 || user.is_deleted) {
      return std::nullopt;
    }
    return user;
  } catch (...) {
    session.rollback();
    throw;
  }
}

std::string UserRepository::getShardingHint(int64_t id) const {
  return fmt::format("-- sharding:{}", id % MAX_SHARDS);
}

std::vector<User> UserRepository::findUsersByName(std::string name) const {
  auto session = Database::instance().createSession();
  try {
    std::vector<User> users;
    for (size_t i = 0; i < MAX_SHARDS; i++) {
      Poco::Data::Statement stmt(session);
      User user;
      stmt << "SELECT id, name, is_admin, is_deleted FROM users WHERE name "
              "LIKE ?",
          into(user.id), into(user.name), into(user.is_admin),
          into(user.is_deleted), use(name), range(0, 1);
      while (!stmt.done()) {
        stmt.execute();
        if (stmt.rowsExtracted() == 0) {
          break;
        }
        if (!user.is_deleted &&
            !std::any_of(users.begin(), users.end(),
                         [&user](const auto &u) { return u.id == user.id; })) {
          users.push_back(user);
        }
      }
    }
    return users;
  } catch (...) {
    session.rollback();
    throw;
  }
}

void UserRepository::deleteUserById(int64_t id) const {
  auto session = Database::instance().createSession();
  try {
    Poco::Data::Statement stmt(session);
    User user;
    stmt << "UPDATE users SET is_deleted=1 WHERE id = ?", getShardingHint(id),
        use(id);
    stmt.execute();
  } catch (...) {
    session.rollback();
    throw;
  }
}

int64_t UserRepository::createUser(User user) const {
  auto session = Database::instance().createSession();
  try {
    Poco::Data::Statement stmt(session);
    stmt << "INSERT INTO users (id, name, is_admin, is_deleted) "
            "VALUES (?, ?, ?, ?)",
        getShardingHint(user.id), use(user.id), use(user.name),
        use(user.is_admin), use(user.is_deleted);
    stmt.execute();
    session.commit();
    return user.id;
  } catch (...) {
    session.rollback();
    throw;
  }
}