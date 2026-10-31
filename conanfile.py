"""Conan recipe for dependency resolution only.

``yun`` itself is consumed through the prebuilt SDK published on GitHub
Releases (see ``scripts/build_sdk.py``); this recipe does not build or
package the project.
"""

from conan import ConanFile


class YunConan(ConanFile):
    name = "yun"
    version = "0.1.0"
    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeDeps", "CMakeToolchain"

    def requirements(self):
        self.requires("fmt/12.1.0", transitive_headers=True, transitive_libs=True)
        self.requires("spdlog/1.17.0", transitive_headers=True, transitive_libs=True)
        self.requires("yaml-cpp/0.8.0", transitive_headers=True, transitive_libs=True)
        self.requires("gtest/1.18.0", transitive_headers=True, transitive_libs=True)
        self.requires("magic_enum/0.9.8", transitive_headers=True, transitive_libs=True)
        # DSL: lua 虚拟机 + sol2 绑定层(sol2 为 header-only)
        self.requires(
            "lua/5.4.6", override=True, transitive_headers=True, transitive_libs=True
        )
        self.requires("sol2/3.5.0", transitive_headers=True, transitive_libs=True)
        self.requires("pybind11/2.13.6", transitive_headers=True)
        self.requires("libfork/3.8.0", transitive_headers=True)
