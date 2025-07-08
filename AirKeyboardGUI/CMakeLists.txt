
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Windows-specific settings
if(WIN32)
    add_definitions(-DWIN32_LEAN_AND_MEAN)
    add_definitions(-DNOMINMAX)
    add_definitions(-DUNICODE -D_UNICODE)
endif()

add_executable (AirKeyboardGUI "AirKeyboardGUI.cpp" "AirKeyboardGUI.h" "src/base/UIView.h" "globals.h" "src/ui/ReactiveChar.h"  "src/capture/KeyEventPublisher.h"  "src/types.h"  "src/capture/FramePublisher.h" "src/base/Publisher.h" "src/base/Subscriber.h" "src/ui/LiveKeyboardView.h"  "src/ThreadManager.h" "src/logging/KeyEventLogger.h"  "src/base/StreamSubscriber.h" "src/base/BatchSubscriber.h" "src/logging/FrameLogger.h" "src/LoggingTrigger.h" "src/EventBus.h" "config.h" "src/ui/TextProvider.h"  "src/logging/FramePostProcessor.h"   "src/ui/LiveKeyboardView.cpp" "src/capture/FramePublisher.cpp" "src/capture/KeyEventPublisher.cpp" "src/logging/FrameLogger.cpp" "src/logging/FramePostProcessor.cpp" "src/logging/KeyEventLogger.cpp" "src/ui/TextProvider.cpp" "src/ui/TextContainer.cpp" "src/EventBus.cpp" "src/LoggingTrigger.cpp" "src/ThreadManager.cpp")

set_target_properties(AirKeyboardGUI PROPERTIES
    WIN32_EXECUTABLE TRUE
)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET AirKeyboardGUI PROPERTY CXX_STANDARD 20)
endif()

# Windows API libraries
target_link_libraries(${PROJECT_NAME}
    user32
    gdi32
    kernel32
    comctl32
    ole32
    oleaut32
    uuid
    mf
    mfplat
    mfreadwrite
    mfuuid
    strmiids
)

configure_file(app.manifest ${CMAKE_BINARY_DIR}/app.manifest COPYONLY)

configure_file(
    ${CMAKE_CURRENT_LIST_DIR}/scripts/frame_postprocessor.py
    ${CMAKE_BINARY_DIR}/AirKeyboardGUI/frame_postprocessor.py
    COPYONLY)   

configure_file(
    ${CMAKE_CURRENT_LIST_DIR}/scripts/hand_landmarker.task
    ${CMAKE_BINARY_DIR}/AirKeyboardGUI/hand_landmarker.task
    COPYONLY
)
