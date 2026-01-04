cmake_minimum_required(VERSION 3.16)
project(radialkb LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick Test Network DBus)

# Optional libinput for touchscreen input (enabled via RADIALKB_INPUT=touch)
find_package(PkgConfig)
if(PkgConfig_FOUND)
    pkg_check_modules(LIBINPUT libinput)
endif()

add_executable(radialkb-ui
    src/ui/main.cpp
)

target_link_libraries(radialkb-ui PRIVATE Qt6::Core Qt6::Gui Qt6::Qml Qt6::Quick Qt6::Network)
target_link_libraries(radialkb-ui PRIVATE Qt6::DBus)

target_compile_definitions(radialkb-ui PRIVATE RADIALKB_QML_DIR="${CMAKE_CURRENT_SOURCE_DIR}/src/ui/qml")

add_executable(radialkb-engine
    src/engine/EngineMain.cpp
    src/engine/InputRouter.cpp
    src/engine/swipe/SwipePath.cpp
    src/engine/RadialLayout.cpp
    src/engine/StateMachine.cpp
    src/engine/GestureRecognizer.cpp
    src/engine/CommitBridge.cpp
    src/engine/UInputKeyboard.cpp
    src/engine/Haptics.cpp
    src/engine/Logging.cpp
)

target_link_libraries(radialkb-engine PRIVATE Qt6::Core Qt6::Network)

# Conditionally add libinput touchscreen backend
if(LIBINPUT_FOUND)
    target_sources(radialkb-engine PRIVATE
        src/engine/TouchInputLibinput.cpp
    )
    target_include_directories(radialkb-engine PRIVATE ${LIBINPUT_INCLUDE_DIRS})
    target_link_libraries(radialkb-engine PRIVATE ${LIBINPUT_LIBRARIES})
    target_compile_definitions(radialkb-engine PRIVATE RADIALKB_HAS_LIBINPUT)
    message(STATUS "libinput found - touchscreen input enabled")
else()
    message(STATUS "libinput not found - touchscreen input disabled")
endif()

add_executable(radialkbctl
    src/ui/radialkbctl.cpp
)

target_link_libraries(radialkbctl PRIVATE Qt6::Core Qt6::DBus)

add_executable(engine_tests
    tests/engine_tests.cpp
    src/engine/swipe/SwipePath.cpp
    src/engine/RadialLayout.cpp
    src/engine/StateMachine.cpp
    src/engine/GestureRecognizer.cpp
    src/engine/Logging.cpp
)

target_link_libraries(engine_tests PRIVATE Qt6::Core Qt6::Test)

install(TARGETS radialkb-ui radialkb-engine radialkbctl RUNTIME DESTINATION bin)
