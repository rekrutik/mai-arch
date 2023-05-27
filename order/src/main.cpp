#include "Poco/Data/RecordSet.h"
#include "Poco/DateTimeFormat.h"
#include "Poco/DateTimeFormatter.h"
#include "Poco/Exception.h"
#include "Poco/JSON/JSON.h"
#include "Poco/JSON/Object.h"
#include "Poco/JSON/Parser.h"
#include "Poco/Net/HTTPClientSession.h"
#include "Poco/Net/HTTPRequestHandler.h"
#include "Poco/Net/HTTPRequestHandlerFactory.h"
#include "Poco/Net/HTTPServer.h"
#include "Poco/Net/HTTPServerParams.h"
#include "Poco/Net/HTTPServerRequest.h"
#include "Poco/Net/HTTPServerResponse.h"
#include "Poco/Net/ServerSocket.h"
#include "Poco/ThreadPool.h"
#include "Poco/Timestamp.h"
#include "Poco/URI.h"
#include "Poco/Util/HelpFormatter.h"
#include "Poco/Util/Option.h"
#include "Poco/Util/OptionSet.h"
#include "Poco/Util/ServerApplication.h"

#include "common/db.h"
#include "common/handler_wrapper.h"
#include "common/request.h"
#include "common/token.h"
#include "order/order_repository.h"

#include <Poco/JSON/Array.h>
#include <Poco/StreamCopier.h>
#include <algorithm>
#include <iostream>
#include <string>
#include <unordered_map>

using Poco::DateTimeFormat;
using Poco::DateTimeFormatter;
using Poco::ThreadPool;
using Poco::Timestamp;
using Poco::URI;
using Poco::Net::HTTPRequestHandler;
using Poco::Net::HTTPRequestHandlerFactory;
using Poco::Net::HTTPServer;
using Poco::Net::HTTPServerParams;
using Poco::Net::HTTPServerRequest;
using Poco::Net::HTTPServerResponse;
using Poco::Net::ServerSocket;
using Poco::Util::Application;
using Poco::Util::HelpFormatter;
using Poco::Util::Option;
using Poco::Util::OptionCallback;
using Poco::Util::OptionSet;
using Poco::Util::ServerApplication;

class ServiceHandler : public HTTPRequestHandlerWrapper {
public:
  ServiceHandler() {}

  void handleGet(HTTPServerRequest &request, HTTPServerResponse &response) {
    auto my_id = validateBearer(request);
    Poco::URI uri(request.getURI());
    auto params = uri.getQueryParameters();
    auto id_it = std::find_if(params.begin(), params.end(),
                              [](const auto &p) { return p.first == "id"; });
    if (id_it == params.end()) {
      response.setStatusAndReason(
          Poco::Net::HTTPResponse::HTTPStatus::HTTP_BAD_REQUEST);
      response.send();
      return;
    }
    int64_t id;
    try {
      id = std::stoll(id_it->second);
    } catch (const std::exception &e) {
      throw BadRequestException();
    }
    auto service = OrderRepository::instance().getServiceById(id);

    if (!service) {
      response.setStatusAndReason(
          Poco::Net::HTTPResponse::HTTPStatus::HTTP_NOT_FOUND);
      response.send();
      return;
    }

    response.setContentType("application/json");
    std::ostream &ostr = response.send();
    Poco::JSON::Object::Ptr inner = new Poco::JSON::Object;
    Poco::JSON::Object::Ptr user_data = new Poco::JSON::Object;
    service->toJson(user_data);
    inner->set("service", user_data);
    inner->stringify(ostr);
  }

  void handlePut(HTTPServerRequest &request, HTTPServerResponse &response) {
    auto my_id = validateBearer(request);
    Poco::URI uri(request.getURI());
    auto params = uri.getQueryParameters();
    std::unordered_map<std::string, std::string> params_map(params.begin(),
                                                            params.end());
    if (!params_map.contains("name") || !params_map.contains("description") ||
        !params_map.contains("price")) {
      throw BadRequestException();
    }

    int64_t price;
    try {
      price = std::stoll(params_map["price"]);
    } catch (const std::exception &e) {
      throw BadRequestException();
    }

    Service service;
    service.name = params_map["name"];
    service.description = params_map["description"];
    service.price = price;
    service.seller_id = my_id;
    auto res_id = OrderRepository::instance().createService(service);
    response.setContentType("application/json");
    response.setStatusAndReason(
        Poco::Net::HTTPResponse::HTTPStatus::HTTP_CREATED);
    std::ostream &ostr = response.send();
    Poco::JSON::Object::Ptr inner = new Poco::JSON::Object;
    inner->set("id", res_id);
    inner->stringify(ostr);
  }

  void handleRequestImpl(HTTPServerRequest &request,
                         HTTPServerResponse &response) {
    if (request.getMethod() == "GET") {
      handleGet(request, response);
    } else if (request.getMethod() == "PUT") {
      handlePut(request, response);
    }
  }
};

class ServiceListHandler : public HTTPRequestHandlerWrapper {
public:
  ServiceListHandler() {}

  void handleGet(HTTPServerRequest &request, HTTPServerResponse &response) {
    auto my_id = validateBearer(request);
    auto services = OrderRepository::instance().getServiceList();

    response.setContentType("application/json");
    std::ostream &ostr = response.send();
    Poco::JSON::Array::Ptr user_data = new Poco::JSON::Array;
    Poco::JSON::Object::Ptr inner = new Poco::JSON::Object;
    for (const auto &service : services) {
      Poco::JSON::Object::Ptr service_obj = new Poco::JSON::Object;
      service.toJson(service_obj);
      user_data->add(service_obj);
    }
    inner->set("services", user_data);
    inner->stringify(ostr);
  }

  void handleRequestImpl(HTTPServerRequest &request,
                         HTTPServerResponse &response) {
    if (request.getMethod() == "GET") {
      handleGet(request, response);
    }
  }
};

class OrderHandler : public HTTPRequestHandlerWrapper {
public:
  OrderHandler() {}

  void handlePut(HTTPServerRequest &request, HTTPServerResponse &response) {
    auto my_id = validateBearer(request);
    Poco::URI uri(request.getURI());
    auto params = uri.getQueryParameters();
    std::unordered_map<std::string, std::string> params_map(params.begin(),
                                                            params.end());
    if (!params_map.contains("service_id")) {
      throw BadRequestException();
    }

    int64_t service_id;
    try {
      service_id = std::stoll(params_map["service_id"]);
    } catch (const std::exception &e) {
      throw BadRequestException();
    }

    auto res_id = OrderRepository::instance().createOrder(my_id, service_id);
    response.setContentType("application/json");
    response.setStatusAndReason(
        Poco::Net::HTTPResponse::HTTPStatus::HTTP_CREATED);
    std::ostream &ostr = response.send();
    Poco::JSON::Object::Ptr inner = new Poco::JSON::Object;
    inner->set("id", res_id);
    inner->stringify(ostr);
  }

  void handleRequestImpl(HTTPServerRequest &request,
                         HTTPServerResponse &response) {
    if (request.getMethod() == "PUT") {
      handlePut(request, response);
    }
  }
};

class OrderListHandler : public HTTPRequestHandlerWrapper {
public:
  OrderListHandler() {}

  void handleGet(HTTPServerRequest &request, HTTPServerResponse &response) {
    auto my_id = validateBearer(request);
    auto services = OrderRepository::instance().getServiceListForUser(my_id);

    response.setContentType("application/json");
    std::ostream &ostr = response.send();
    Poco::JSON::Array::Ptr user_data = new Poco::JSON::Array;
    Poco::JSON::Object::Ptr inner = new Poco::JSON::Object;
    for (const auto &service : services) {
      Poco::JSON::Object::Ptr service_obj = new Poco::JSON::Object;
      service.toJson(service_obj);
      user_data->add(service_obj);
    }
    inner->set("services", user_data);
    inner->stringify(ostr);
  }

  void handleRequestImpl(HTTPServerRequest &request,
                         HTTPServerResponse &response) {
    if (request.getMethod() == "GET") {
      handleGet(request, response);
    }
  }
};

class UserServerRequestHandlerFactory : public HTTPRequestHandlerFactory {
public:
  UserServerRequestHandlerFactory() {}

  HTTPRequestHandler *createRequestHandler(const HTTPServerRequest &request) {
    Poco::URI uri(request.getURI());
    auto path = uri.getPath();
    if (path == "/order/v1/service") {
      return new ServiceHandler();
    } else if (path == "/order/v1/service/list") {
      return new ServiceListHandler();
    } else if (path == "/order/v1/order") {
      return new OrderHandler();
    } else if (path == "/order/v1/order/list") {
      return new OrderListHandler();
    } else {
      return nullptr;
    }
  }
};

class HTTPTimeServer : public Poco::Util::ServerApplication {
protected:
  void initialize(Application &self) {
    loadConfiguration();
    ServerApplication::initialize(self);
  }

  void defineOptions(OptionSet &options) {
    ServerApplication::defineOptions(options);

    options.addOption(Option("help", "h", "display argument help information")
                          .required(false)
                          .repeatable(false)
                          .callback(OptionCallback<HTTPTimeServer>(
                              this, &HTTPTimeServer::handleHelp)));
  }

  void handleHelp(const std::string &name, const std::string &value) {
    HelpFormatter helpFormatter(options());
    helpFormatter.setCommand(commandName());
    helpFormatter.setUsage("OPTIONS");
    helpFormatter.setHeader("An order web-server.");
    helpFormatter.format(std::cout);
    stopOptionsProcessing();
    _helpRequested = true;
  }

  int main(const std::vector<std::string> &args) {
    if (!_helpRequested) {
      unsigned short port =
          static_cast<unsigned short>(config().getInt("UserServer.port", 8002));
      ServerSocket svs(port);
      HTTPServer srv(new UserServerRequestHandlerFactory(), svs,
                     new HTTPServerParams);
      srv.start();
      waitForTerminationRequest();
      srv.stop();
    }
    return Application::EXIT_OK;
  }

private:
  bool _helpRequested = false;
};

int main(int argc, char **argv) {
  HTTPTimeServer app;
  return app.run(argc, argv);
}