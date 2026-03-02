from conan import ConanFile
from conan.tools.cmake import CMakeToolchain


class ComptineConan(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeDeps"

    def requirements(self):
        self.requires("yaml-cpp/0.8.0")

    def generate(self):
        generator = None if self.settings.os == "Windows" else "Ninja"
        tc = CMakeToolchain(self, generator=generator)
        tc.variables["CMAKE_EXPORT_COMPILE_COMMANDS"] = True
        tc.generate()
