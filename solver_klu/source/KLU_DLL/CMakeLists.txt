cmake_minimum_required(VERSION 3.6)
project(solver_KLU)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

find_package(KLU REQUIRED)

add_library(${PROJECT_NAME} SHARED
        KLU_DLL.h
        KLU_DLL.cpp
        )

if (WIN32)
    set(KLU_LIB_PREFIX "")
else ()
    set(KLU_LIB_PREFIX "lib_")
endif ()
set_target_properties(${PROJECT_NAME}
        PROPERTIES PREFIX "${KLU_LIB_PREFIX}"
        )

target_include_directories(${PROJECT_NAME} PRIVATE ${KLU_INCLUDES})
target_link_libraries(${PROJECT_NAME} PRIVATE ${KLU_LIBRARIES})

set(FILE_PERMISSIONS PERMISSIONS
        OWNER_EXECUTE OWNER_READ OWNER_WRITE
        GROUP_EXECUTE GROUP_READ
        WORLD_EXECUTE WORLD_READ)

install(TARGETS ${PROJECT_NAME}
        ${FILE_PERMISSIONS}
        RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/gridlabd
        LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/gridlabd
        ARCHIVE DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/gridlabd
        COMPONENT library)
