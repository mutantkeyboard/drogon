# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

#
# - Try to find Facebook zstd library
# This will define
# ZSTD_FOUND
# ZSTD_INCLUDE_DIR
# ZSTD_LIBRARY
# ZSTD_lib (imported target for Drogon compatibility)
#

find_path(ZSTD_INCLUDE_DIR NAMES zstd.h)

find_library(ZSTD_LIBRARY_DEBUG NAMES zstdd zstd_staticd)
find_library(ZSTD_LIBRARY_RELEASE NAMES zstd zstd_static)

include(SelectLibraryConfigurations)
SELECT_LIBRARY_CONFIGURATIONS(ZSTD)

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(
    ZSTD DEFAULT_MSG
    ZSTD_LIBRARY ZSTD_INCLUDE_DIR
)

if (ZSTD_FOUND)
    message(STATUS "Found Zstd: ${ZSTD_LIBRARY}")
    # Define ZSTD_lib as an imported target for Drogon
    if (NOT TARGET ZSTD_lib)
        add_library(ZSTD_lib UNKNOWN IMPORTED)
        set_target_properties(ZSTD_lib PROPERTIES
            IMPORTED_LOCATION "${ZSTD_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${ZSTD_INCLUDE_DIR}")
    endif()
endif()

mark_as_advanced(ZSTD_INCLUDE_DIR ZSTD_LIBRARY)