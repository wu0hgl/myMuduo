add_executable(asynclogging_test AsyncLogging_test.cpp)
target_link_libraries(asynclogging_test muduo_base)

add_executable(atomic_unittest Atomic_unittest.cpp)
#target_link_libraries(atomic_unittest muduo_base)

add_executable(blockingqueue_bench BlockingQueue_bench.cpp)
target_link_libraries(blockingqueue_bench muduo_base)   

add_executable(blockingqueue_test BlockingQueue_test.cpp)
target_link_libraries(blockingqueue_test muduo_base)   

add_executable(boundedblockingqueue_test BoundedBlockingQueue_test.cpp)
target_link_libraries(boundedblockingqueue_test muduo_base)   

add_executable(exception_test Exception_test.cpp)
target_link_libraries(exception_test muduo_base)   

add_executable(fileutil_test FileUtil_test.cpp)
target_link_libraries(fileutil_test muduo_base)   

add_executable(logfile_test LogFile_test.cpp)
target_link_libraries(logfile_test muduo_base)     

add_executable(logging_test Logging_test.cpp)
target_link_libraries(logging_test muduo_base)     

add_executable(logstream_bench LogStream_bench.cpp)
target_link_libraries(logstream_bench muduo_base)     

if(BOOSTTEST_LIBRARY)
add_executable(logstream_test LogStream_test.cpp)
target_link_libraries(logstream_test  muduo_base boost_unit_test_framework)     # 链接boost库单元测试框架
endif()

add_executable(mutex_test Mutex_test.cpp)
target_link_libraries(mutex_test muduo_base)

add_executable(singleton_test Singleton_test.cpp)
target_link_libraries(singleton_test muduo_base)

add_executable(singletonthreadlocal_test SingletonThreadLocal_test.cpp)
target_link_libraries(singletonthreadlocal_test muduo_base)

add_executable(thread_test Thread_test.cpp)
target_link_libraries(thread_test muduo_base)

add_executable(threadlocal_test ThreadLocal_test.cpp)
target_link_libraries(threadlocal_test muduo_base)

add_executable(threadlocalsingleton_test ThreadLocalSingleton_test.cpp)
target_link_libraries(threadlocalsingleton_test muduo_base)

add_executable(threadpool_test ThreadPool_test.cpp)
target_link_libraries(threadpool_test muduo_base)

add_executable(timestamp_unittest Timestamp_unittest.cpp)
target_link_libraries(timestamp_unittest muduo_base)   
