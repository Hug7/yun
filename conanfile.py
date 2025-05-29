from conan import ConanFile
from conan.tools.cmake import CMake, CMakeToolchain, CMakeDeps
from conan.tools.files import copy
import os


class WxRouteConan(ConanFile):
    name = "wx_route"
    version = "1.0.0"
    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeDeps", "CMakeToolchain"
    options = {"shared": [True, False]}
    default_options = {"shared": False}
    exports_sources = [
        "CMakeLists.txt",
        "common/*",
        "domain/*",
        "core/*",
        "tests/*",
    ]

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def requirements(self):
        self.requires("fmt/11.1.3", transitive_headers=True, transitive_libs=True)
        self.requires("spdlog/1.15.1", transitive_headers=True, transitive_libs=True)
        # self.requires("date/3.0.3", transitive_headers=True, transitive_libs=True)
        self.requires("yaml-cpp/0.8.0", transitive_headers=True, transitive_libs=True)

    def package(self):
        # 安装头文件
        copy(self, "*.h", src=os.path.join(self.source_folder, "common/include"), dst=os.path.join(self.package_folder, "include/common"))
        copy(self, "*.h", src=os.path.join(self.source_folder, "domain/include"), dst=os.path.join(self.package_folder, "include/domain"))
        copy(self, "*.h", src=os.path.join(self.source_folder, "core/tsp/include"), dst=os.path.join(self.package_folder, "include/tsp"))
        
        # 安装库文件（根据实际构建输出调整）
        copy(self, "*.lib", self.build_folder, os.path.join(self.package_folder, "lib"), keep_path=False)
        copy(self, "*.dll", self.build_folder, os.path.join(self.package_folder, "bin"), keep_path=False)
        copy(self, "*.so", self.build_folder, os.path.join(self.package_folder, "lib"), keep_path=False)
        copy(self, "*.dylib", self.build_folder, os.path.join(self.package_folder, "lib"), keep_path=False)
        copy(self, "*.a", self.build_folder, os.path.join(self.package_folder, "lib"), keep_path=False)
    
    def package_info(self):
        # self.cpp_info.set_property("cmake_target_name", "wx_route::wx_route")
        # 根据实际生成的库文件配置
        self.cpp_info.requires = ["fmt::fmt", "spdlog::spdlog", "yaml-cpp::yaml-cpp"]  # 关键：声明依赖关系传递
        self.cpp_info.libs = ["route_domain", "route_tsp"]  # 库名
        self.cpp_info.includedirs = []

        # # self.cpp_info.components["common"].libs = ["route_common"]
        self.cpp_info.components["route_common"].includedirs = ["include/common"]
        self.cpp_info.components["route_common"].requires = ["fmt::fmt", "spdlog::spdlog", "yaml-cpp::yaml-cpp"]

        self.cpp_info.components["route_domain"].libs = ["route_domain"]
        self.cpp_info.components["route_domain"].includedirs = ["include/domain"]

        self.cpp_info.components["route_tsp"].libs = ["route_tsp"]
        self.cpp_info.components["route_tsp"].includedirs = ["include/tsp"]
        self.cpp_info.components["route_tsp"].requires = ["route_common", "route_domain"]

