from conan import ConanFile


class QPConanFile(ConanFile):
    name = "qp"
    homepage = "https://github.com/cdleonard/qp"
    description = """\
QP is a header-only library containing debug macros. It is
not intended for permanent inclusion into a project but rather only for
debugging highly specific issues.
"""

    requires = ("libcheck/0.15.2",)

    generators = "cmake"
