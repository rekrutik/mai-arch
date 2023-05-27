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
#include "user/user_repository.h"

#include <Poco/StreamCopier.h>
#include <algorithm>
#include <cstdlib>
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

class UserHandler : public HTTPRequestHandlerWrapper {
public:
  UserHandler() {}

  void handleGet(HTTPServerRequest &request, HTTPServerResponse &response) {
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
    auto user = UserRepository::instance().getUserById(id);

    if (!user) {
      response.setStatusAndReason(
          Poco::Net::HTTPResponse::HTTPStatus::HTTP_NOT_FOUND);
      response.send();
      return;
    }

    response.setContentType("application/json");
    std::ostream &ostr = response.send();
    Poco::JSON::Object::Ptr inner = new Poco::JSON::Object;
    Poco::JSON::Object::Ptr user_data = new Poco::JSON::Object;
    user->toJson(user_data);
    inner->set("user", user_data);
    inner->stringify(ostr);
  }

  void handleDelete(HTTPServerRequest &request, HTTPServerResponse &response) {
    auto caller_id = validateBearer(request);
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

    if (caller_id != id) {
      auto caller = UserRepository::instance().getUserById(caller_id);
      if (!caller || !caller->is_admin) {
        throw ForbiddenException();
      }
    }

    UserRepository::instance().deleteUserById(id);
    response.setStatusAndReason(
        Poco::Net::HTTPResponse::HTTPStatus::HTTP_NO_CONTENT);
    response.send();
  }

  void handlePut(HTTPServerRequest &request, HTTPServerResponse &response) {
    Poco::URI uri(request.getURI());
    auto params = uri.getQueryParameters();
    std::unordered_map<std::string, std::string> params_map(params.begin(),
                                                            params.end());
    if (!params_map.contains("username") || !params_map.contains("name") ||
        !params_map.contains("password")) {
      throw BadRequestException();
    }

    Poco::URI auth_uri(std::getenv("AUTH_HOST"));
    auth_uri.setPath("/auth/v1/sign_up");
    auth_uri.addQueryParameter("username", params_map["username"]);
    auth_uri.addQueryParameter("password", params_map["password"]);
    auto ss = makeRequest(auth_uri, {{"X-API-KEY", std::getenv("AUTH_TOKEN")}},
                          Poco::Net::HTTPRequest::HTTP_POST);

    Poco::JSON::Parser parser;
    auto result = parser.parse(ss);
    auto ptr = result.extract<Poco::JSON::Object::Ptr>();

    User user;
    user.id = ptr->getValue<int64_t>("id");
    user.name = params_map["name"];
    auto res_id = UserRepository::instance().createUser(user);

    response.setContentType("application/json");
    response.setStatusAndReason(
        Poco::Net::HTTPResponse::HTTPStatus::HTTP_CREATED);
    std::ostream &ostr = response.send();
    Poco::JSON::Object::Ptr inner = new Poco::JSON::Object;
    inner->set("token", generateToken(res_id));
    inner->stringify(ostr);
  }

  void handleRequestImpl(HTTPServerRequest &request,
                         HTTPServerResponse &response) {
    if (request.getMethod() == "GET") {
      handleGet(request, response);
    } else if (request.getMethod() == "PUT") {
      handlePut(request, response);
    } else if (request.getMethod() == "DELETE") {
      handleDelete(request, response);
    }
  }
};

class UserSearchHandler : public HTTPRequestHandlerWrapper {
public:
  UserSearchHandler() {}

  void handleGet(HTTPServerRequest &request, HTTPServerResponse &response) {
    Poco::URI uri(request.getURI());
    auto params = uri.getQueryParameters();
    auto name_it =
        std::find_if(params.begin(), params.end(),
                     [](const auto &p) { return p.first == "name"; });
    if (name_it == params.end()) {
      response.setStatusAndReason(
          Poco::Net::HTTPResponse::HTTPStatus::HTTP_BAD_REQUEST);
      response.send();
      return;
    }
    auto users = UserRepository::instance().findUsersByName(name_it->second);
    response.setContentType("application/json");
    std::ostream &ostr = response.send();
    Poco::JSON::Array::Ptr user_data = new Poco::JSON::Array;
    Poco::JSON::Object::Ptr inner = new Poco::JSON::Object;
    for (const auto &user : users) {
      Poco::JSON::Object::Ptr user_obj = new Poco::JSON::Object;
      user.toJson(user_obj);
      user_data->add(user_obj);
    }
    inner->set("users", user_data);
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
    Application &app = Application::instance();
    app.logger().information("Request from %s %s",
                             request.clientAddress().toString(), path);
    if (path == "/user/v1/user") {
      return new UserHandler();
    } else if (path == "/user/v1/search") {
      return new UserSearchHandler();
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
    helpFormatter.setHeader("A user web-server.");
    helpFormatter.format(std::cout);
    stopOptionsProcessing();
    _helpRequested = true;
  }

  int main(const std::vector<std::string> &args) {
    if (!_helpRequested) {
      unsigned short port =
          static_cast<unsigned short>(config().getInt("UserServer.port", 8001));
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