# KeyHole

KeyHole is a suite of libraries and tools used to help provide for introspection
and instrumentation of applications. The current development target is the
support of JVM-based programs via the JVM Tooling Interface (JVM TI). Other
programming environments may be added, should the opportunity arise.

## Requirements

The following software is required to succesfully build and run the provided
source:

* CMake >= 3.31.0
* C++ Compiler with C++23 support
* Git

MacOS 26, Fedora 43, and OpenSUSE Tumbleweed are the primary platforms on which
the codebase is developed and are considered first-class citizens for support.
As function visibility has not been manually provided at the various call sites,
Windows DLLs have not been configured to build correctly.

As far as Java goes, the targeted versions for support include 11+ (Class file
version 55.0).

## Binary Outputs

At this time, there are two primary binaries that are generated in a build. They
consist of the following:

* libkh-classfile.(dylib|so) - Provides the primary functionality for reading and
interacting with JVM class files

* kh-cli - Currently serves as a driver for library functions so that they can be
run on real-world datasets (such as `javac` outputs)

## Building

To build the software, you'll need to perform the following steps:

1. `cmake -DCMAKE_BUILD_TYPE=(Debug|Release) .`, this only needs to occur once
unless very heavy CMake changes are made

2. `make`

## Testing

Once built, tests are handled through both `ctest` and `GoogleTest`. A simple
invocation of `ctest` will trigger the entire test suite.

## Running

At the time of writing `kh-cli` provides two features:

1. A `javap`-like class file examiner, invoked via
`kh-cli inspect <FILENAME>.class`

2. An example serialized file to validate code generation, invoked via
`kh-cli test-class <FILENAME>.class`
