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

boost and spdlog may be installed through Linux package manager, or through vcpkg (for both Windows and Linux), then made detectable for meson by providing pkg_config file (for Windows, pkg-config-lite works fine with meson). For Linux, vcpkg-generated pkg_config files usually work, but for Windows, hand-written pkg_config files may be needed.
