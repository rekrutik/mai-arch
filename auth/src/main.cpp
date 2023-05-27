#include "Poco/Data/RecordSet.h"
#include "Poco/DateTimeFormat.h"
#include "Poco/DateTimeFormatter.h"
#include "Poco/Exception.h"
#include "Poco/JSON/JSON.h"
#include "Poco/JSON/Object.h"
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

#include "auth/accounts_repository.h"
#include "common/db.h"
#include "common/handler_wrapper.h"
#include "common/request.h"
#include "common/token.h"

#include <Poco/Crypto/CryptoException.h>
#include <algorithm>
#include <iostream>
#include <string>

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

class SignInHandler : public HTTPRequestHandlerWrapper {
public:
  SignInHandler() {}

  void handleRequestImpl(HTTPServerRequest &request,
                         HTTPServerResponse &response) {
    Poco::URI uri(request.getURI());
    auto params = uri.getQueryParameters();
    auto id_it = std::find_if(params.begin(), params.end(),
                              [](const auto &p) { return p.first == "id"; });
    auto username_it =
        std::find_if(params.begin(), params.end(),
                     [](const auto &p) { return p.first == "username"; });
    auto password_it =
        std::find_if(params.begin(), params.end(),
                     [](const auto &p) { return p.first == "password"; });
    if ((id_it == params.end() && password_it == params.end()) ||
        (username_it == params.end() && password_it == params.end())) {
      throw BadRequestException();
    }

    std::optional<Account> account_opt;

    if (id_it != params.end()) {
      int64_t id;
      try {
        id = std::stoll(id_it->second);
      } catch (const std::exception &e) {
        throw BadRequestException();
      }
      auto acc_opt = AccountsRepository::instance().getAccountById(id);
    } else {
      auto acc_opt = AccountsRepository::instance().getAccountByUsername(username_it->second);
    }

    if (!account_opt || password_it->second != account_opt->password) {
      throw UnauthorizedException();
    }
    auto account = *account_opt;

    response.setContentType("application/json");
    std::ostream &ostr = response.send();
    Poco::JSON::Object::Ptr inner = new Poco::JSON::Object;
    inner->set("token", generateToken(account.id));
    inner->stringify(ostr);
  }
};

class SignUpHandler : public HTTPRequestHandlerWrapper {
public:
  SignUpHandler() {}

  void handleRequestImpl(HTTPServerRequest &request,
                         HTTPServerResponse &response) {
    validateBasic(request);
    Poco::URI uri(request.getURI());
    auto params = uri.getQueryParameters();
    auto username_it =
        std::find_if(params.begin(), params.end(),
                     [](const auto &p) { return p.first == "username"; });
    auto password_it =
        std::find_if(params.begin(), params.end(),
                     [](const auto &p) { return p.first == "password"; });
    if (username_it == params.end() || password_it == params.end()) {
      throw BadRequestException();
    }
    auto account = Account{.id = 0,
                           .username = username_it->second,
                           .password = password_it->second};
    Poco::JSON::Object::Ptr inner = new Poco::JSON::Object;
    inner->set("id", AccountsRepository::instance().createAccount(account));
    response.setContentType("application/json");
    std::ostream &ostr = response.send();
    inner->stringify(ostr);
  }
};

class ValidateHandler : public HTTPRequestHandlerWrapper {
public:
  ValidateHandler() {}

  void handleRequestImpl(HTTPServerRequest &request,
                         HTTPServerResponse &response) {
    validateBasic(request);
    std::cerr << "zxc\n";
    Poco::URI uri(request.getURI());
    auto params = uri.getQueryParameters();
    auto it = std::find_if(params.begin(), params.end(),
                           [](const auto &p) { return p.first == "token"; });
    if (it == params.end()) {
      throw BadRequestException();
    }
    Poco::JSON::Object::Ptr inner = new Poco::JSON::Object;
    int64_t id;
    if (extractPayload(it->second, id)) {
      inner->set("valid", true);
      inner->set("id", id);
    } else {
      inner->set("valid", false);
    }
    response.setContentType("application/json");
    response.setStatusAndReason(
        Poco::Net::HTTPResponse::HTTPStatus::HTTP_CREATED);
    std::ostream &ostr = response.send();
    inner->stringify(ostr);
  }
};

class AuthServerRequestHandlerFactory : public HTTPRequestHandlerFactory {
public:
  AuthServerRequestHandlerFactory() {}

  HTTPRequestHandler *createRequestHandler(const HTTPServerRequest &request) {
    Poco::URI uri(request.getURI());
    auto path = uri.getPath();
    if (path == "/auth/v1/sign_in") {
      return new SignInHandler();
    } else if (path == "/auth/v1/sign_up") {
      return new SignUpHandler();
    } else if (path == "/auth/v1/validate") {
      return new ValidateHandler();
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
    helpFormatter.setHeader("An auth web-server.");
    helpFormatter.format(std::cout);
    stopOptionsProcessing();
    _helpRequested = true;
  }

  int main(const std::vector<std::string> &args) {
    if (!_helpRequested) {
      unsigned short port =
          static_cast<unsigned short>(config().getInt("AuthServer.port", 8000));
      ServerSocket svs(port);
      HTTPServer srv(new AuthServerRequestHandlerFactory(), svs,
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