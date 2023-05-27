#include "order/order_repository.h"
#include "common/db.h"
#include <cassert>
#include <optional>

using Poco::Data::Keywords::into;
using Poco::Data::Keywords::range;
using Poco::Data::Keywords::use;

void Service::toJson(Poco::JSON::Object::Ptr ptr) const {
  ptr->set("id", id);
  ptr->set("name", name);
  ptr->set("description", description);
  ptr->set("price", price);
  ptr->set("seller_id", seller_id);
  ptr->set("created_at", created_at.utcTime());
}

OrderRepository::OrderRepository() {}

OrderRepository &OrderRepository::instance() {
  static OrderRepository INSTANCE;
  return INSTANCE;
}

std::optional<Service> OrderRepository::getServiceById(int64_t id) const {
  auto session = Database::instance().createSession();
  try {
    Poco::Data::Statement stmt(session);
    Service service;
    service.id = id;
    stmt << "SELECT name, description, price, seller_id, created_at FROM "
            "services "
            "WHERE id = "
            "?",
        into(service.name), into(service.description), into(service.price),
        into(service.seller_id), into(service.created_at), use(id), range(0, 1);
    stmt.execute();

    if (stmt.rowsExtracted() == 0) {
      return std::nullopt;
    }
    return service;
  } catch (...) {
    session.rollback();
    throw;
  }
}

std::vector<Service> OrderRepository::getServiceList() const {
  auto session = Database::instance().createSession();
  try {
    Poco::Data::Statement stmt(session);
    Service service;
    stmt << "SELECT id, name, description, price, seller_id, created_at FROM "
            "services",
        into(service.id), into(service.name), into(service.description),
        into(service.price), into(service.seller_id), into(service.created_at),
        range(0, 1);
    std::vector<Service> res;
    while (!stmt.done()) {
      stmt.execute();
      if (stmt.rowsExtracted() == 0) {
        break;
      }
      res.push_back(service);
    }
    return res;
  } catch (...) {
    session.rollback();
    throw;
  }
}

int64_t OrderRepository::createOrder(int64_t user_id,
                                     int64_t service_id) const {
  auto session = Database::instance().createSession();
  try {
    Poco::Data::Statement stmt(session);
    stmt << "INSERT INTO orders (buyer_id, service_id) "
            "VALUES (?, ?)",
        use(user_id), use(service_id);
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
std::vector<Service>
OrderRepository::getServiceListForUser(int64_t user_id) const {
  auto session = Database::instance().createSession();
  try {
    Poco::Data::Statement stmt(session);
    Service service;
    stmt << "SELECT services.id, services.name, services.description, "
            "services.price, services.seller_id, services.created_at FROM "
            "orders LEFT JOIN services ON orders.service_id = services.id AND "
            "orders.buyer_id = ?",
        into(service.id), into(service.name), into(service.description),
        into(service.price), into(service.seller_id), into(service.created_at),
        use(user_id), range(0, 1);
    std::vector<Service> res;
    while (!stmt.done()) {
      stmt.execute();
      if (stmt.rowsExtracted() == 0) {
        break;
      }
      res.push_back(service);
    }
    return res;
  } catch (...) {
    session.rollback();
    throw;
  }
}

// void UserRepository::deleteUserById(int64_t id) const {
//   auto session = Database::instance().createSession();
//   try {
//     Poco::Data::Statement stmt(session);
//     User user;
//     stmt << "UPDATE users SET is_deleted=1 WHERE id = ?", use(id);
//     stmt.execute();
//   } catch (...) {
//     session.rollback();
//     throw;
//   }
// }

int64_t OrderRepository::createService(Service service) const {
  auto session = Database::instance().createSession();
  try {
    Poco::Data::Statement stmt(session);
    stmt << "INSERT INTO services (name, description, price, seller_id) "
            "VALUES (?, ?, ?, ?)",
        use(service.name), use(service.description), use(service.price),
        use(service.seller_id);
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