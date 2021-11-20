# QP: Quick Print with a header-only logging library

QP is a header-only library containing debug macros. It is not intended for
permanent inclusion into a project but rather only for debugging highly specific
issues.

Various macros can be defined before `#include "qp.h"` which adapt the library
to the target environment, for example to use the target project's own log
output function.

## Features

Built-in functionality:

* Display func(line): header
* Optional custom timestamp header
* Rate limiting (per-location)
* Micro-profiling certain areas
* Helpers to format various network-related structures.
* Automatic detection of "kernel/userspace" environment.

## Installation

Symlink `qp.h` to somewhere in your include path and `#include "qp.h"`

## Documentation

Doxygen HTML is published on [gitlab pages](https://cdleonard.gitlab.io/qp/doxygen-html/qp_8h.html) and
also [gitlab artifacts](https://gitlab.com/cdleonard/qp/-/jobs/artifacts/main/file/html/qp_8h.html?job=docs)

## Selftest

Run `make check`.

This requires [libcheck](https://libcheck.github.io/check/) which can be installed
via `apt install check`.

Test coverage is low.
