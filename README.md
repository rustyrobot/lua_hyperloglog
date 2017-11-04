Lua HyperLogLog Library
-----------------------

## Overview
HyperLogLog is an algorithm for the count-distinct problem, approximating the number of distinct elements in a multiset (the cardinality).

## Installation

### Prerequisites
* C compiler (GCC 4.7+, Visual Studio 2013, MinGW (Lua 5.1))
* Lua 5.1, Lua 5.2, or LuaJIT
* [CMake (2.8.7+)](http://cmake.org/cmake/resources/software.html)

### CMake Build  Instructions

    git clone https://github.com/mozilla-services/lua_hyperloglog.git
    cd lua_hyperloglog 
    mkdir release
    cd release

    # UNIX
    cmake -DCMAKE_BUILD_TYPE=release ..
    make

    # Windows Visual Studio 2013
    cmake -DCMAKE_BUILD_TYPE=release -G "NMake Makefiles" ..
    nmake

    ctest
    cpack

## Module

### Example Usage
```lua
require "hyperloglog"

local hll = hyperloglog.new()
hll:add("test")
local estimate = hll:count()
-- estimate == 1

```

### API Functions

#### new
```lua
require "hyperloglog"
local hll = hyperloglog.new()
```

Import Lua _hyperloglog_ via the Lua 'require' function. The module is
globally registered and returned by the require function. The _new_ function
takes no arguments and returns a hyperloglog userdata object.

#### version
```lua
local v = hyperloglog.version()
-- v == "0.1.0"
```

Returns a string with the running version of hyperloglog.

*Arguments*
- none

*Return*
- Semantic version string

### API Methods

#### add
```lua
local altered = hll:add(key)
```

Adds an item to the hyperloglog.

*Arguments*
- key (string/number) The item key to add to the hyperloglog.

*Return*
- True if the estimate was altered, false if it remains unchanged.

#### count
```lua
local estimate = hll:count()
```

Returns the approximated item count.

*Arguments*
- none

*Return*
- Returns the approximated number of distinct items in the set.

#### clear
```lua
hll:clear()
```

Resets the hyperloglog to an empty set.

*Arguments*
- none

*Return*
- none
