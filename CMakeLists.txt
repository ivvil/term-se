cmake_minimum_required(VERSION 3.15)
project(term-se LANGUAGES CXX)

find_package(Boost REQUIRED COMPONENTS system)
find_package(Threads REQUIRED)

add_executable(termse)
target_sources(termse PRIVATE src/server.cpp)
target_compile_features(termse PRIVATE cxx_std_17)

target_link_libraries(termse
  Boost::boost
  Boost::system
  Threads::Threads
)
