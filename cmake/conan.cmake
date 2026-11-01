# Conan provider 包装层：只补 CMake/Conan 无法自动推断的设置。
#
# 注意：不要在这里设置 CONAN_HOST_PROFILE，否则会覆盖 provider 自动生成的
# tools.build:compiler_executables（clang-cl 构建依赖包时需要）。

set(_yun_conan_args "--build=missing")
if(DEFINED CONAN_INSTALL_ARGS)
  set(_yun_conan_args ${CONAN_INSTALL_ARGS})
endif()

list(APPEND _yun_conan_args "-s" "compiler.cppstd=20")

if(WIN32 AND CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  # clang-cl：使用 MSVC 的 STL/ABI，运行时必须是动态 CRT（与 Python / pybind11 一致）
  list(APPEND _yun_conan_args "-s" "compiler.runtime=dynamic")
  if(CMAKE_CONFIGURATION_TYPES)
    message(WARNING
      "clang-cl + 多配置生成器：compiler.runtime_type 无法按配置自动设置；"
      "请使用 Ninja/Unix Makefiles 单配置生成器。")
  elseif(CMAKE_BUILD_TYPE STREQUAL "Debug")
    list(APPEND _yun_conan_args "-s" "compiler.runtime_type=Debug")
  else()
    list(APPEND _yun_conan_args "-s" "compiler.runtime_type=Release")
  endif()
endif()

# Windows ARM64：provider 依赖 CMAKE_CXX_COMPILER_ARCHITECTURE_ID 推断 arch；
# 当使用 x64 clang-cl 交叉编译 ARM64 时，显式指定 Conan arch，避免依赖包按 x86_64 构建。
if(WIN32 AND (CMAKE_CXX_COMPILER_TARGET MATCHES "aarch64|arm64"
              OR CMAKE_SYSTEM_PROCESSOR MATCHES "ARM64|arm64|aarch64"))
  list(APPEND _yun_conan_args "-s" "arch=armv8")
endif()

if(WIN32 AND CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  # MinGW GCC（可选平台）：显式声明线程/异常模型，避免 Conan 包 ID 不一致
  list(APPEND _yun_conan_args
    "-s" "compiler.threads=posix"
    "-s" "compiler.exception=seh")
endif()

set(CONAN_INSTALL_ARGS "${_yun_conan_args}" CACHE STRING "" FORCE)

include("${CMAKE_CURRENT_LIST_DIR}/../conan_provider.cmake")
