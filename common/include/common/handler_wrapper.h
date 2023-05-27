#pragma once

#include "Poco/Data/MySQL/MySQLException.h"
#include "Poco/Net/HTTPRequestHandler.h"
#include "Poco/Net/HTTPServerRequest.h"
#include "Poco/Net/HTTPServerResponse.h"
#include "Poco/Util/Application.h"
#include <exception>

class UnauthorizedException : public std::exception {
public:
  const char *what() { return "Unauthorized"; }
};

class BadRequestException : public std::exception {
public:
  const char *what() { return "Bad request"; }
};

class ForbiddenException : public std::exception {
public:
  const char *what() { return "Forbidden"; }
};

class HTTPRequestHandlerWrapper : public Poco::Net::HTTPRequestHandler {
public:
  void handleRequest(Poco::Net::HTTPServerRequest &request,
                     Poco::Net::HTTPServerResponse &response) final {
    auto &app = Poco::Util::Application::instance();
    app.logger().information("Request from %s",
                             request.clientAddress().toString());
    try {
      handleRequestImpl(request, response);
    } catch (const UnauthorizedException &e) {
      response.setStatusAndReason(
          Poco::Net::HTTPResponse::HTTPStatus::HTTP_UNAUTHORIZED);
      response.send();
    } catch (const BadRequestException &e) {
      response.setStatusAndReason(
          Poco::Net::HTTPResponse::HTTPStatus::HTTP_BAD_REQUEST);
      response.send();
    } catch (Poco::Data::MySQL::ConnectionException &e) {
      app.logger().error("connection: %s :: %s", e.what(), e.message());
      response.setStatusAndReason(
          Poco::Net::HTTPResponse::HTTPStatus::HTTP_INTERNAL_SERVER_ERROR);
      response.send();
    } catch (Poco::Data::MySQL::StatementException &e) {
      app.logger().error("connection: %s :: %s", e.what(), e.message());
      response.setStatusAndReason(
          Poco::Net::HTTPResponse::HTTPStatus::HTTP_INTERNAL_SERVER_ERROR);
      response.send();
    } catch (Poco::Exception &e) {
      app.logger().error("connection: %s :: %s", e.what(), e.message());
      response.setStatusAndReason(
          Poco::Net::HTTPResponse::HTTPStatus::HTTP_INTERNAL_SERVER_ERROR);
      response.send();
    } catch (const std::exception &e) {
      throw;
    }
  }

protected:
  virtual void handleRequestImpl(Poco::Net::HTTPServerRequest &request,
                                 Poco::Net::HTTPServerResponse &response) = 0;
};
