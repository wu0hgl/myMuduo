set(net_SRCS
  #Acceptor.cc
  Acceptor.cpp
  #Buffer.cc
  Buffer.cpp
  #Channel.cc
  Channel.cpp
  #Connector.cc
  Connector.cpp
  #EventLoop.cc
  EventLoop.cpp
  #EventLoopThread.cc
  EventLoopThread.cpp
  #EventLoopThreadPool.cc
  EventLoopThreadPool.cpp
  #InetAddress.cc
  InetAddress.cpp
  #Poller.cc
  Poller.cpp
  #poller/DefaultPoller.cc
  poller/DefaultPoller.cpp
  #poller/EPollPoller.cc
  poller/EPollPoller.cpp
  #poller/PollPoller.cc
  poller/PollPoller.cpp
  #Socket.cc
  Socket.cpp
  #SocketsOps.cc
  SocketsOps.cpp
  #TcpClient.cc
  TcpClient.cpp
  #TcpConnection.cc
  TcpConnection.cpp
  #TcpServer.cc
  TcpServer.cpp
  #Timer.cc
  Timer.cpp
  #TimerQueue.cc
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
#
#if(PROTOBUF_FOUND)
#  add_subdirectory(protorpc)
#else()
#  add_subdirectory(protorpc EXCLUDE_FROM_ALL)
#endif()
