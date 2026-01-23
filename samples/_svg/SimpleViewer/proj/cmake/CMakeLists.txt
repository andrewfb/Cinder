cmake_minimum_required( VERSION 3.16 FATAL_ERROR )
set( CMAKE_VERBOSE_MAKEFILE ON )

project( svg-SimpleViewer )

get_filename_component( CINDER_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../../../.." ABSOLUTE )
get_filename_component( APP_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../" ABSOLUTE )

include( "${CINDER_PATH}/proj/cmake/modules/cinderMakeApp.cmake" )

# Cairo sources and includes (shared across platforms)
set( CAIRO_SOURCES
    ${CINDER_PATH}/blocks/Cairo/src/Cairo.cpp
)

set( CAIRO_INCLUDES
    ${CINDER_PATH}/blocks/Cairo/include
)

if( CMAKE_SYSTEM_NAME STREQUAL "Linux" )
    # Add our custom FindCairo module to the module path
    list(APPEND CMAKE_MODULE_PATH "${CINDER_PATH}/proj/cmake/modules")

    # Find system Cairo
    find_package( Cairo REQUIRED )

    list( APPEND CAIRO_INCLUDES ${CAIRO_INCLUDE_DIRS} )

    ci_make_app(
        SOURCES     ${APP_PATH}/src/SimpleViewerApp.cpp ${CAIRO_SOURCES}
        INCLUDES    ${CAIRO_INCLUDES}
        LIBRARIES   ${CAIRO_LIBRARIES}
        CINDER_PATH ${CINDER_PATH}
    )

    # Link Cairo libraries
    target_link_libraries( ${PROJECT_NAME} PRIVATE ${CAIRO_LIBRARIES} )
    target_include_directories( ${PROJECT_NAME} PRIVATE ${CAIRO_INCLUDE_DIRS} )

elseif( CMAKE_SYSTEM_NAME STREQUAL "Darwin" )
    # macOS: use prebuilt static libraries from the Cairo block
    set( CAIRO_LIB_PATH ${CINDER_PATH}/blocks/Cairo/lib/macosx )

    # Add platform-specific Cairo headers
    list( APPEND CAIRO_INCLUDES ${CINDER_PATH}/blocks/Cairo/include/macosx )

    ci_make_app(
        SOURCES     ${APP_PATH}/src/SimpleViewerApp.cpp ${CAIRO_SOURCES}
        INCLUDES    ${CAIRO_INCLUDES}
        CINDER_PATH ${CINDER_PATH}
    )

    # Link Cairo static libraries
    target_link_libraries( ${PROJECT_NAME} PRIVATE
        ${CAIRO_LIB_PATH}/libcairo.a
        ${CAIRO_LIB_PATH}/libpixman-1.a
        ${CAIRO_LIB_PATH}/libpng16.a
    )

    # Retina support
    add_custom_command( TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND plutil -replace NSHighResolutionCapable -bool true
                "$<TARGET_BUNDLE_DIR:${PROJECT_NAME}>/Contents/Info.plist"
    )

else()
    # Windows or other: try the BLOCKS approach
    ci_make_app(
        SOURCES     ${APP_PATH}/src/SimpleViewerApp.cpp
        BLOCKS      Cairo
        CINDER_PATH ${CINDER_PATH}
    )
endif()
