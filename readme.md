# Hanabi Command Line Game
This command line game reproduces the [Hanabi Game](https://boardgamegeek.com/boardgame/98778/hanabi). It runs a server that game players can connect to via TCP connections.

## Dependencies and Set Up

### Dependencies
- **C++ Compiler**: Requires a C++ compiler that supports C++23.
- **Boost Libraries**: Requires Boost 1.70 or higher with the following components:
  - `filesystem`
  - `system`
  - `log`
- **CMake**: Version 3.5.0 or higher.

### Set Up
1. **Install Dependencies**:
   - **Boost Libraries**: Follow the instructions on the [Boost website](https://www.boost.org/) to download and install Boost.
   - **CMake**: Follow the instructions on the [CMake website](https://cmake.org/install/) to download and install CMake.
   - **C++ Compiler**: Ensure you have a C++ compiler that supports C++23. For example, GCC 13 or 14

    For Ubuntu, we can use 
    ```sh
    sudo apt-get install libboost-all-dev cmake
    ```
2. **Build the Project**: 
    - Create a build directory:
      ```sh
      mkdir build
      cd build
      ```
    - Run CMake to configure the project:
      ```sh
      cmake ..
      ```
    - Build the project using CMake:
      ```sh
      cmake --build .
      ```
    Note that you can also just use the VSCode `CMake: Build` command to build.