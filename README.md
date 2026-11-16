# GoogleTest test folder

Put GoogleTest test source files (`.cpp`) in this folder. The Makefile
automatically discovers `tests/*.cpp`.

`test_gtest_setup.cpp` is only a framework smoke test. It checks that
GoogleTest runs; it does not test Smart File Explorer functionality.
Add tests that include your project headers and exercise your real classes.

Example:

```cpp
#include <gtest/gtest.h>
#include "FileItem.h"

// Write tests using the actual constructor and public API in FileItem.h.
```

Install GoogleTest on Ubuntu/Debian:

```bash
sudo apt update
sudo apt install libgtest-dev
```
