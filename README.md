This is unstable software, with absolutely no warranty or guarantee whatsoever, whether express or implied, including but not limited to those of merchantability, fitness for specific purpose, or non-infringement.

This package builds upon tm_infra, and provides the following pre-packaged functionalities for codes written with tm_infra:

* Clock (real-time, simulated real-time, or single pass iteration).

* Serialization (mostly CBOR-based), with macros to facilitate the creations of serializable structs.

* Certain classes used in type-level manipulations.

* Logging.

* Certain pre-defined nodes and node combinations.

* Transaction-like requests and service components.

* Nodes reading from and writing to a shared "chain" (linked list).

INSTALLATION NOTES:

The requirements of tm_basic are, in addition to requirements of tm_infra:

* tm_infra

* boost >= 1.73.0

* spdlog
 
* nlohmann_json (https://github.com/nlohmann/json)

* simdjson (https://github.com/simdjson/simdjson)

* date_tz (https://github.com/HowardHinnant/date) (for C++20, this is actually not needed, but I have to include it in meson build file so as to allow lower C++ version)

boost and spdlog may be installed through Linux package manager, or through vcpkg (for both Windows and Linux), then made detectable for meson by providing pkg_config file (for Windows, pkg-config-lite works fine with meson). For Linux, vcpkg-generated pkg_config files usually work, but for Windows, hand-written pkg_config files may be needed.

nlohmann_json and simdjson are required for Json serialization/deserialization functionalities. This package also supports CBOR and protobuf serialization/deserialization, but those are directly implemented in wire protocol format and does not depend on libraries.

For simdjson that is installed via vcpkg on Windows, for some reason, ${VCPKG_ROOT}/buildtrees/simdjson/src/${SIMDJSON_VERSION}/singleheader/simdjson.cpp has *not* been compiled into the library (as of version v1.0.2-054cb6851f.clean). So, if an app uses simdjson functionality and needs to be compiled on Windows, then this file needs to be added to the source list of the app, otherwise there will be link errors.

There is one header file that uses HDF5. Because that is a single header not included in any other header, HDF5 has not been listed as a dependency for this package. If a project needs this header, that project should have a dependency to hdf5_cpp (if vcpkg is used for providing dependencies). Also, on Windows, that project would have to define H5_BUILT_AS_DYNAMIC_LIB=1.
