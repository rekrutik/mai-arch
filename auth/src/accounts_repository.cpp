#include "auth/accounts_repository.h"
#include "common/db.h"
#include <optional>

using Poco::Data::Keywords::into;
using Poco::Data::Keywords::range;
using Poco::Data::Keywords::use;

AccountsRepository::AccountsRepository() {}

AccountsRepository &AccountsRepository::instance() {
  static AccountsRepository INSTANCE;
  return INSTANCE;
}

std::optional<Account> AccountsRepository::getAccountById(int64_t id) const {
  auto session = Database::instance().createSession();
  try {
    Poco::Data::Statement stmt(session);
    Account account;
    account.id = id;
    stmt << "SELECT username, password FROM accounts WHERE id = ?",
        into(account.username), into(account.password), use(id), range(0, 1);
    stmt.execute();

    if (stmt.rowsExtracted() == 0) {
      return std::nullopt;
    }
    return account;
  } catch (...) {
    session.rollback();
    throw;
  }
}

std::optional<Account>
AccountsRepository::getAccountByUsername(std::string username) const {
  auto session = Database::instance().createSession();
  try {
    Poco::Data::Statement stmt(session);
    Account account;
    account.username = username;
    stmt << "SELECT id, password FROM accounts WHERE username = ?",
        into(account.id), into(account.password), use(username), range(0, 1);
    stmt.execute();

    if (stmt.rowsExtracted() == 0) {
      return std::nullopt;
    }
    return account;
  } catch (...) {
    session.rollback();
    throw;
  }
}

int64_t AccountsRepository::createAccount(Account account) const {
  auto session = Database::instance().createSession();
  try {
    Poco::Data::Statement stmt(session);
    stmt << "INSERT INTO accounts (username, password) VALUES (?, ?)",
        use(account.username), use(account.password);
    stmt.execute();
    Poco::Data::Statement stmt2(session);
    int64_t id;
    stmt2 << "SELECT LAST_INSERT_ID()", into(id);
    stmt2.execute();
    session.commit();
    return id;
  } catch (...) {
    session.rollback();
    throw;
  }
}