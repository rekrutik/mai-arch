project(user_service CXX)

find_package(Poco REQUIRED)

add_executable(${PROJECT_NAME} src/main.cpp src/user_repository.cpp)
target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(${PROJECT_NAME} Poco::Poco common)
