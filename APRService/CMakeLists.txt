set(APRSERVICE_TOCALL           "APRS")

set(APRSERVICE_SOFTWARE_NAME    "APRService")
set(APRSERVICE_SOFTWARE_VERSION "0.1")

set(APRSERVICE_RX_BUFFER_SIZE   1024)

project(APRService)
add_library(APRService STATIC APRService.cpp)
target_compile_definitions(APRService PUBLIC -DAPRSERVICE_TOCALL="${APRSERVICE_TOCALL}")
target_compile_definitions(APRService PUBLIC -DAPRSERVICE_SOFTWARE_NAME="${APRSERVICE_SOFTWARE_NAME}")
target_compile_definitions(APRService PUBLIC -DAPRSERVICE_SOFTWARE_VERSION="${APRSERVICE_SOFTWARE_VERSION}")
target_compile_definitions(APRService PUBLIC -DAPRSERVICE_RX_BUFFER_SIZE=${APRSERVICE_RX_BUFFER_SIZE})
target_include_directories(APRService PUBLIC ${CMAKE_CURRENT_LIST_DIR})

if(UNIX)
	target_link_libraries(APRService PUBLIC )
	target_compile_definitions(APRService PUBLIC -DAPRSERVICE_UNIX=1)
elseif(WIN32)
	target_link_libraries(APRService PUBLIC Ws2_32)
	target_compile_definitions(APRService PUBLIC -DAPRSERVICE_WIN32=1)
endif()
