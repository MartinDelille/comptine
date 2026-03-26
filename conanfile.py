import os

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

        # Read Qt version from .qt-version and prepend Qt to CMAKE_PREFIX_PATH
        # inside the toolchain file. Using list(PREPEND) in the toolchain ensures
        # it works even when Qt Creator passes its own -DCMAKE_PREFIX_PATH on the
        # command line, because normal CMake variables shadow cache variables.
        qt_version_file = os.path.join(self.source_folder, ".qt-version")
        if os.path.exists(qt_version_file):
            with open(qt_version_file) as f:
                qt_version = f.read().strip()
            qt_home = os.path.expanduser("~/Qt")
            if self.settings.os == "Macos":
                qt_path = os.path.join(qt_home, qt_version, "macos")
            elif self.settings.os == "Windows":
                qt_path = os.path.join(qt_home, qt_version, "msvc2022_64")
            else:
                qt_path = os.path.join(qt_home, qt_version, "gcc_64")
            qt_path = qt_path.replace("\\", "/")
            find_paths = tc.blocks["find_paths"]
            original_template = find_paths.template
            find_paths.template = (
                original_template
                + '\nlist(PREPEND CMAKE_PREFIX_PATH "'
                + qt_path
                + '")\n'
            )

        tc.generate()
