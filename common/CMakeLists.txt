project(common CXX)

find_package(Poco REQUIRED)
find_package(fmt REQUIRED)

add_library(${PROJECT_NAME} db.cpp)
target_link_libraries(${PROJECT_NAME} Poco::Poco fmt::fmt)
target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/include)
