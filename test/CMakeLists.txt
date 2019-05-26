add_executable (test_atomic_wait test_atomic_wait.cpp)

target_include_directories(test_atomic_wait PUBLIC
  ${CMAKE_SOURCE_DIR}/include
)

target_compile_features(test_atomic_wait PUBLIC
  cxx_std_17
)

target_link_libraries(test_atomic_wait ${CMAKE_THREAD_LIBS_INIT})
