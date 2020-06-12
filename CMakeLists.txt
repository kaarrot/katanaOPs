cmake_minimum_required (VERSION 3.2)
project(KatanaExampleOps)

set(KATANA_ROOT "$ENV{HOME}/Katana3.5v2" CACHE PATH "Path to Katana")
list(INSERT CMAKE_MODULE_PATH 0 "${KATANA_ROOT}/plugins/Src/cmake")

set(CMAKE_CXX_STANDARD 11)

find_package(Katana PATHS "${KATANA_ROOT}/plugin_apis/cmake" REQUIRED)

if (NOT MSVC)
    add_compile_options(-Wall)
endif ()

# Ensure we don't compile DSOs with undefined symbols.
if (CMAKE_SYSTEM_NAME MATCHES Linux)
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--no-undefined")
    set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -Wl,--no-undefined")
endif ()

if (MSVC)
    # Prevent Boost auto-linking.
    add_definitions(-DBOOST_ALL_NO_LIB)
endif ()

# Find dependencies.

# TODO - would be nice if only this one was required
# find_package(Katana REQUIRED PATH ${KATANA_ROOT}/plugin_apis/cmake)

# find_package(Boost 1.55 COMPONENTS filesystem regex system thread)
# find_package(Boost 1.55 COMPONENTS filesystem regex system thread HINTS ${KATANA_ROOT/bin} ${KATANA_ROOT/external/FnBoost})


# find_package(HDF5 COMPONENTS C HL)
find_package(OpenEXR)
# find_package(Threads REQUIRED)
# find_package(ZLIB)
# find_package(Alembic) # Alembic comes last, as it requires all of the above.

# find_package(TBB)
# find_package(TinyXML)

#-------------------------------------------------------------------------------

# if (ALEMBIC_FOUND AND Boost_FOUND AND HDF5_FOUND AND OPENEXR_FOUND
#         AND ZLIB_FOUND AND TBB_FOUND)
#     add_subdirectory(AlembicIn)
# else ()
#     message("Not compiling AlembicIn Op as not all dependencies were found.")
# endif ()

# if (Boost_FOUND AND TINYXML_FOUND AND TBB_FOUND)
#     add_subdirectory(AttributeFile_In)
# else ()
#     message("Not compiling AttributeFile_In Op as not all dependencies were found.")
# endif ()

# if (OPENEXR_FOUND)
#     add_subdirectory(RigidBind)
#     add_subdirectory(SkinEnvelope)
# else ()
#     message("Not compiling RigidBind or SkinEnvelope Ops as OpenEXR was not found.")
# endif ()

# if (Boost_FOUND)
#     add_subdirectory(ScenegraphXml)
# else ()
#     message("Not compiling ScenegraphXml as not all dependencies were found.")
# endif ()


add_library(HelloWorldOp MODULE HelloWorldOp.cpp)

target_link_libraries(HelloWorldOp
    PRIVATE
    Katana::FnAttribute
    Katana::FnGeolibOpPlugin
)

set_target_properties(HelloWorldOp PROPERTIES PREFIX "")

install(TARGETS HelloWorldOp DESTINATION Ops)


### CubeMaker
add_library(CubeMaker MODULE CubeMaker.cpp)

target_link_libraries(CubeMaker
    PRIVATE
    Katana::FnAttribute
    Katana::FnGeolibOpPlugin
    Katana::FnGeolibServices
    Katana::pystring
)

set_target_properties(CubeMaker PROPERTIES PREFIX Ops) # or "" ?
install(FILES RegisterCubeMakerNode.py DESTINATION Plugins)
install(TARGETS CubeMaker DESTINATION Ops)



