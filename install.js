const os = process.platform;
if (os === "win32") {
  module.exports = {
    version: "8.0.0",
    run: [{
      method: "shell.run",
      params: {
        message: "echo Native Windows builds are not supported yet. Use WSL2, open the repository inside the Linux filesystem, and follow docs/BUILD.md.",
        path: "."
      }
    }]
  };
} else {
  module.exports = {
    version: "8.0.0",
    run: [{
      method: "shell.run",
      params: {
        message: "cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build --parallel && ctest --test-dir build --output-on-failure",
        path: "."
      }
    }]
  };
}
