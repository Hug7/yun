from conan import ConanFile
from conan.tools.cmake import CMake, CMakeToolchain, CMakeDeps
from conan.tools.files import copy
import os


class YunConan(ConanFile):
    name = "yun"
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
        "model/*"
        "tests/*",
    ]

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def requirements(self):
        self.requires("fmt/12.1.0", transitive_headers=True, transitive_libs=True)
        self.requires("spdlog/1.17.0", transitive_headers=True, transitive_libs=True)
        # self.requires("date/3.0.3", transitive_headers=True, transitive_libs=True)
        self.requires("yaml-cpp/0.8.0", transitive_headers=True, transitive_libs=True)
        # self.requires("boost/1.91.0", transitive_headers=True, transitive_libs=True)

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
        # self.cpp_info.set_property("cmake_target_name", "yun::yun")
        # 根据实际生成的库文件配置
        self.cpp_info.requires = ["fmt::fmt", "spdlog::spdlog", "yaml-cpp::yaml-cpp"]  # 关键：声明依赖关系传递
        self.cpp_info.libs = ["yun_domain", "yun_tsp"]  # 库名
        self.cpp_info.includedirs = []

        # # self.cpp_info.components["common"].libs = ["yun_common"]
        self.cpp_info.components["yun_common"].includedirs = ["include/common"]
        self.cpp_info.components["yun_common"].requires = ["fmt::fmt", "spdlog::spdlog", "yaml-cpp::yaml-cpp"]

        self.cpp_info.components["yun_domain"].libs = ["yun_domain"]
        self.cpp_info.components["yun_domain"].includedirs = ["include/domain"]
        self.cpp_info.components["yun_domain"].requires = ["yun_common"]

        self.cpp_info.components["yun_tsp"].libs = ["yun_tsp"]
        self.cpp_info.components["yun_tsp"].includedirs = ["include/tsp"]
        self.cpp_info.components["yun_tsp"].requires = ["yun_common", "yun_domain"]

