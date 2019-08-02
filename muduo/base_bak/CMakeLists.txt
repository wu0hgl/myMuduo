set(base_SRCS
  #Condition.cc
  Condition.cpp
  #CountDownLatch.cc
  CountDownLatch.cpp
  #Exception.cc
  Exception.cpp
  #FileUtil.cc
  FileUtil.cpp
  #LogFile.cc
  LogFile.cpp
  #Logging.cc
  Logging.cpp
  #LogStream.cc
  LogStream.cpp
  #ProcessInfo.cc
  ProcessInfo.cpp
  #Thread.cc
  Thread.cpp
  #ThreadPool.cc
  ThreadPool.cpp
  #Timestamp.cc
  Timestamp.cpp
)

add_library(muduo_base ${base_SRCS})
target_link_libraries(muduo_base pthread rt)

install(TARGETS muduo_base DESTINATION lib)
file(GLOB HEADERS "*.h")
install(FILES ${HEADERS} DESTINATION include/muduo/base)

if(NOT CMAKE_BUILD_NO_EXAMPLES)
  add_subdirectory(tests)
endif()
