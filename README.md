# Casino Royale

## Third-Party Code

This project uses SFML as a multimedia library to deal with visuals and audio.
The original author has dedicated the code to the public domain.
See `THIRD_PARTY_NOTICES.md` for details.

## Build Configuration

The project is configured with:

- **Generator:** Ninja (fast incremental builds)
- **Compiler:** MSVC (Visual Studio 2022)
- **Standard:** C++17
- **Dependencies:** SFML 3.0.2, GameNetworkingSockets, OpenSSL (via vcpkg)

To reconfigure the build system:

```powershell
cd build
cmake -G Ninja -DCMAKE_BUILD_TYPE=Debug ..
cd ..
```
