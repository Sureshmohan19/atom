# Bazel WORKSPACE file - not Python!

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

# Use a specific version of rules_cc that's compatible with Bazel 7.4.1
http_archive(
    name = "rules_cc",
    urls = ["https://github.com/bazelbuild/rules_cc/releases/download/0.0.9/rules_cc-0.0.9.tar.gz"],
    sha256 = "2037875b9a4456dce4a79d112a8ae885bbc4aad968e6587dca6e64f3a0900cdf",
    strip_prefix = "rules_cc-0.0.9",
)

new_local_repository(
    name = "python_314",
    path = "/opt/homebrew/opt/python@3.14/Frameworks/Python.framework/Versions/3.14",
    build_file_content = """
load("@rules_cc//cc:defs.bzl", "cc_library")

cc_library(
    name = "python",
    hdrs = glob(["include/python3.14/**/*.h"]),
    includes = ["include/python3.14"],
    visibility = ["//visibility:public"],
)
""",
)

new_local_repository(
    name = "numpy_headers",
    path = "/Users/aakrititanwar/Desktop/atom/.venv/lib/python3.14/site-packages/numpy/_core/include",
    build_file_content = """
load("@rules_cc//cc:defs.bzl", "cc_library")

cc_library(
    name = "numpy",
    hdrs = glob(["**/*.h"]),
    includes = ["."],
    visibility = ["//visibility:public"],
)
""",
)
