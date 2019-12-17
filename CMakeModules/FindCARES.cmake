# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.
#
# This is a slight edited version of FindLIBUV.cmake :)

#[=======================================================================[.rst:
FindCARES
---------

Find c-ares includes and library.

Imported Targets
^^^^^^^^^^^^^^^^

An :ref:`imported target <Imported targets>` named
``CARES::CARES`` is provided if c-ares has been found.

Result Variables
^^^^^^^^^^^^^^^^

This module defines the following variables:

``CARES_FOUND``
  True if c-ares was found, false otherwise.
``CARES_INCLUDE_DIRS``
  Include directories needed to include c-ares headers.
``CARES_LIBRARIES``
  Libraries needed to link to c-ares.
``CARES_VERSION``
  The version of c-ares found.
``CARES_VERSION_MAJOR``
  The major version of c-ares.
``CARES_VERSION_MINOR``
  The minor version of c-ares.
``CARES_VERSION_PATCH``
  The patch version of c-ares.

Cache Variables
^^^^^^^^^^^^^^^

This module uses the following cache variables:

``CARES_LIBRARY``
  The location of the c-ares library file.
``CARES_INCLUDE_DIR``
  The location of the c-ares include directory containing ``ares.h``.

The cache variables should not be used by project code.
They may be set by end users to point at c-ares components.
#]=======================================================================]

set(CARES_NAMES ${CARES_NAMES} cares)

#-----------------------------------------------------------------------------
find_library(CARES_LIBRARY
  NAMES ${CARES_NAMES}
  )
mark_as_advanced(CARES_LIBRARY)

find_path(CARES_INCLUDE_DIR
  NAMES ares.h
  )
mark_as_advanced(CARES_INCLUDE_DIR)

#-----------------------------------------------------------------------------
# Extract version number if possible.
set(_CARES_H_REGEX "#[ \t]*define[ \t]+ARES_VERSION_(MAJOR|MINOR|PATCH)[ \t]+[0-9]+")
if(CARES_INCLUDE_DIR AND EXISTS "${CARES_INCLUDE_DIR}/ares_version.h")
  file(STRINGS "${CARES_INCLUDE_DIR}/ares_version.h" _CARES_H REGEX "${_CARES_H_REGEX}")
else()
  set(_CARES_H "")
endif()
foreach(c MAJOR MINOR PATCH)
  if(_CARES_H MATCHES "#[ \t]*define[ \t]+ARES_VERSION_${c}[ \t]+([0-9]+)")
    set(_CARES_VERSION_${c} "${CMAKE_MATCH_1}")
  else()
    unset(_CARES_VERSION_${c})
  endif()
endforeach()
if(DEFINED _CARES_VERSION_MAJOR AND DEFINED _CARES_VERSION_MINOR)
  set(CARES_VERSION_MAJOR "${_CARES_VERSION_MAJOR}")
  set(CARES_VERSION_MINOR "${_CARES_VERSION_MINOR}")
  set(CARES_VERSION "${CARES_VERSION_MAJOR}.${CARES_VERSION_MINOR}")
  if(DEFINED _CARES_VERSION_PATCH)
    set(CARES_VERSION_PATCH "${_CARES_VERSION_PATCH}")
    set(CARES_VERSION "${CARES_VERSION}.${CARES_VERSION_PATCH}")
  else()
    unset(CARES_VERSION_PATCH)
  endif()
else()
  set(CARES_VERSION_MAJOR "")
  set(CARES_VERSION_MINOR "")
  set(CARES_VERSION_PATCH "")
  set(CARES_VERSION "")
endif()
unset(_CARES_VERSION_MAJOR)
unset(_CARES_VERSION_MINOR)
unset(_CARES_VERSION_PATCH)
unset(_CARES_H_REGEX)
unset(_CARES_H)

#-----------------------------------------------------------------------------
include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(CARES
  FOUND_VAR CARES_FOUND
  REQUIRED_VARS CARES_LIBRARY CARES_INCLUDE_DIR
  VERSION_VAR CARES_VERSION
  )
set(CARES_FOUND ${CARES_FOUND})

#-----------------------------------------------------------------------------
# Provide documented result variables and targets.
if(CARES_FOUND)
  set(CARES_INCLUDE_DIRS ${CARES_INCLUDE_DIR})
  set(CARES_LIBRARIES ${CARES_LIBRARY})
  if(NOT TARGET CARES::CARES)
    add_library(CARES::CARES UNKNOWN IMPORTED)
    set_target_properties(CARES::CARES PROPERTIES
      IMPORTED_LOCATION "${CARES_LIBRARY}"
      INTERFACE_INCLUDE_DIRECTORIES "${CARES_INCLUDE_DIRS}"
      )
  endif()
endif()
