set(APRSERVICE_TOCALL           "APRS")

set(APRSERVICE_SOFTWARE_NAME    "APRService")
set(APRSERVICE_SOFTWARE_VERSION "0.1")

project(APRService)

add_library(APRService SHARED APRS.cpp APRService.cpp)

if(UNIX)
	target_link_libraries(APRService PUBLIC )
	target_compile_definitions(APRService PUBLIC -DAPRSERVICE_UNIX=1)
elseif(WIN32)
	target_link_libraries(APRService PUBLIC Ws2_32)
	target_compile_definitions(APRService PUBLIC -DAPRSERVICE_WIN32=1)
endif()

target_compile_definitions(APRService PRIVATE -DAPRSERVICE_API=1)
target_compile_definitions(APRService PUBLIC -DAPRSERVICE_TOCALL="${APRSERVICE_TOCALL}")
target_compile_definitions(APRService PUBLIC -DAPRSERVICE_SOFTWARE_NAME="${APRSERVICE_SOFTWARE_NAME}")
target_compile_definitions(APRService PUBLIC -DAPRSERVICE_SOFTWARE_VERSION="${APRSERVICE_SOFTWARE_VERSION}")
target_include_directories(APRService PUBLIC ${CMAKE_CURRENT_LIST_DIR})
set_target_properties(APRService PROPERTIES PREFIX "" OUTPUT_NAME "APRService")
