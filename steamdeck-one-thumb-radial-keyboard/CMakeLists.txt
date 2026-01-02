cmake_minimum_required(VERSION 3.16)
project(radialkb LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick Test)

add_executable(radialkb-ui
    src/ui/main.cpp
)

target_link_libraries(radialkb-ui PRIVATE Qt6::Core Qt6::Gui Qt6::Qml Qt6::Quick)

target_compile_definitions(radialkb-ui PRIVATE RADIALKB_QML_DIR="${CMAKE_CURRENT_SOURCE_DIR}/src/ui/qml")

add_executable(radialkb-engine
    src/engine/EngineMain.cpp
    src/engine/InputRouter.cpp
    src/engine/RadialLayout.cpp
    src/engine/StateMachine.cpp
    src/engine/GestureRecognizer.cpp
    src/engine/CommitBridge.cpp
    src/engine/Haptics.cpp
    src/engine/Logging.cpp
)

target_link_libraries(radialkb-engine PRIVATE Qt6::Core)

add_executable(engine_tests
    tests/engine_tests.cpp
    src/engine/RadialLayout.cpp
    src/engine/StateMachine.cpp
    src/engine/GestureRecognizer.cpp
    src/engine/Logging.cpp
)

target_link_libraries(engine_tests PRIVATE Qt6::Core Qt6::Test)

install(TARGETS radialkb-ui radialkb-engine RUNTIME DESTINATION bin)
