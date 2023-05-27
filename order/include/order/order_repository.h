#pragma once

#include "Poco/JSON/Object.h"

#include <cinttypes>
#include <optional>
#include <string>

struct Service {
  int64_t id;
  std::string name;
  std::string description;
  int64_t price;
  int64_t seller_id;
  Poco::DateTime created_at;

  void toJson(Poco::JSON::Object::Ptr ptr) const;
};

class OrderRepository {
public:
  static OrderRepository &instance();
  std::optional<Service> getServiceById(int64_t id) const;
  std::vector<Service> getServiceList() const;
  int64_t createService(Service service) const;

  int64_t createOrder(int64_t user_id, int64_t service_id) const;
  std::vector<Service> getServiceListForUser(int64_t user_id) const;

private:
  OrderRepository();
};