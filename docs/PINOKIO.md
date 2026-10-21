# Pinokio integration

Balance includes a Pinokio launcher for users who prefer a button-driven setup over manually entering CMake commands. Pinokio runs the same native build; it does not bundle a compiler, OpenSSL, libcurl, or ncurses.

## Current scope

- **macOS and Linux:** supported when the C17 compiler, CMake 3.20+, OpenSSL 3 development files, libcurl development files, and curses/ncurses development files are already installed.
- **Windows:** use WSL2 and follow the Linux build guide inside the WSL distribution. Native Windows compilation is not currently supported. The Pinokio shell does not magically turn the WSL executable into a Windows executable.
- **Security:** inspect `pinokio.js`, `install.js`, and `start.js` before running them, just as you should inspect any automation script.

## Use it

1. Install Pinokio using its official instructions.
2. Add/open the public Balance repository as a local Pinokio app. The repository URL is `https://github.com/qoddit/balance`.
3. Install native build dependencies for the host OS using [`BUILD.md`](BUILD.md).
4. Select **Install / Build** in the launcher and wait for CMake to finish.
5. Select **Launch Balance** to run `./build/balance` in Pinokio's shell.

If your Pinokio version uses a different launcher menu presentation, run the included scripts from the app's script list; the scripts are plain, inspectable JavaScript objects using Pinokio's script API.

## Files

- `pinokio.js` — launcher menu.
- `install.js` — configures and compiles the release build.
- `start.js` — launches the compiled TUI.

The install script deliberately does not run `sudo`, modify system package sources, or install global dependencies. Install prerequisites explicitly so users can see what changes are made to their systems. The scripts use the repository-local `build/` directory.

## Troubleshooting

- If install fails with missing headers/libraries, install the matching OS development packages from [`BUILD.md`](BUILD.md) and run **Install / Build** again.
- If the repository was moved between operating systems or architectures, remove `build/` and rebuild locally.
- If terminal UI input is not captured as expected, launch Balance from a real terminal/shell panel rather than a non-interactive log/output pane.
- Pinokio compatibility has been added as a launcher integration, but has not been certified or submitted to Pinokio's verified Discover catalog.

## References

- [Pinokio documentation](https://desktop.pinokio.co/docs/)
- [Pinokio project and script policy](https://github.com/pinokiocomputer/pinokio)
