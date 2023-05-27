#pragma once

#include <cinttypes>
#include <optional>
#include <string>

struct Account {
  int64_t id;
  std::string username;
  std::string password;
};

class AccountsRepository {
public:
  static AccountsRepository &instance();
  std::optional<Account> getAccountById(int64_t id) const;
  std::optional<Account> getAccountByUsername(std::string username) const;
  int64_t createAccount(Account account) const;

private:
  AccountsRepository();
};