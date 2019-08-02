set(net_SRCS
  Acceptor.cpp
  Buffer.cpp
  Channel.cpp
  Connector.cpp
  EventLoop.cpp
  EventLoopThread.cpp
  EventLoopThreadPool.cpp
  InetAddress.cpp
  Poller.cpp
  poller/DefaultPoller.cpp
  poller/EPollPoller.cpp
  poller/PollPoller.cpp
  Socket.cpp
  SocketsOps.cpp
  TcpClient.cpp
  TcpConnection.cpp
  TcpServer.cpp
  Timer.cpp
  TimerQueue.cpp
  )

add_library(muduo_net ${net_SRCS})
target_link_libraries(muduo_net muduo_base)

install(TARGETS muduo_net DESTINATION lib)
set(HEADERS
  Acceptor.h
  Buffer.h
  Channel.h
  Endian.h
  EventLoop.h
  EventLoopThread.h
  EventLoopThreadPool.h
  InetAddress.h
  TcpClient.h
  TcpConnection.h
  TcpServer.h
  TimerId.h
  )
install(FILES ${HEADERS} DESTINATION include/muduo/net)

add_subdirectory(http)
add_subdirectory(inspect)

if(NOT CMAKE_BUILD_NO_EXAMPLES)
  add_subdirectory(tests)
endif()

#if(PROTOBUF_FOUND)
#  add_subdirectory(protorpc)
#else()
#  add_subdirectory(protorpc EXCLUDE_FROM_ALL)
#endif()