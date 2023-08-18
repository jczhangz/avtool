

# This library is compiled as a DYNAMIC library by default, unless
# FOO_LIBRARY_STATIC is set to TRUE.
if (NOT DEFINED AV_LIBRARY_STATIC)
    set(AV_LIBRARY_STATIC FALSE)
endif()

# List all library sources
set(AV_LIBRARY_SOURCES
        av_library.h
        av_library.cpp
        )

include_directories(include)
link_directories(lib)

# Create static or shared library target.
if (AV_LIBRARY_STATIC)
    # Static
    add_library(av_library STATIC ${FOO_LIBRARY_SOURCES})

    # Define FOO_LIBRARY_STATIC when building and using this library.
    target_compile_definitions(av_library
            PUBLIC
            AV_LIBRARY_STATIC
            )

else()
    # Dynamic
    add_library(av_library SHARED ${AV_LIBRARY_SOURCES})
    # Make all non-exported symbols are hidden by default (similar to Windows)
    set_target_properties(av_library
            PROPERTIES
            C_VISIBILITY_PRESET hidden
            CXX_VISIBILITY_PRESET hidden
            )
endif()

# Treat the exported symbols as exported (and not imported) when building the library
target_compile_definitions(av_library
        PRIVATE
        AV_LIBRARY_BUILD
        )

# Export the library's public include path to dependent targets
target_include_directories(av_library
        INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}
        )


target_link_libraries(av_library PRIVATE
        swscale swresample avcodec avutil avdevice avfilter avformat
        )