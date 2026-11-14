cmake_minimum_required(VERSION 3.20)
project(Sonic3AIRiOS LANGUAGES C CXX OBJC OBJCXX)
if(NOT CMAKE_SYSTEM_NAME STREQUAL "iOS")
  message(FATAL_ERROR "Configure with -DCMAKE_SYSTEM_NAME=iOS -DCMAKE_OSX_SYSROOT=iphoneos")
endif()
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
get_filename_component(ROOT "${CMAKE_CURRENT_LIST_DIR}/../../../.." ABSOLUTE)
set(EXT "${ROOT}/framework/external")
set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON CACHE BOOL "" FORCE)
set(SDL_TEST OFF CACHE BOOL "" FORCE)
set(SDL_TESTS OFF CACHE BOOL "" FORCE)
set(SDL_DISABLE_INSTALL ON CACHE BOOL "" FORCE)
add_subdirectory("${EXT}/sdl/SDL2" SDL)

file(GLOB OGG_SOURCES "${EXT}/ogg-vorbis/libogg/src/*.c" "${EXT}/ogg-vorbis/libvorbis/lib/*.c")
add_library(oggvorbis STATIC ${OGG_SOURCES})
target_include_directories(oggvorbis PUBLIC "${EXT}/ogg-vorbis/libogg/include" "${EXT}/ogg-vorbis/libvorbis/include" PRIVATE "${EXT}/ogg-vorbis/libvorbis/lib")
add_library(minizip STATIC "${EXT}/zlib/zlib/contrib/minizip/ioapi.c" "${EXT}/zlib/zlib/contrib/minizip/unzip.c" "${EXT}/zlib/zlib/contrib/minizip/zip.c")
target_include_directories(minizip PUBLIC "${EXT}/zlib/zlib/contrib/minizip")
target_link_libraries(minizip PUBLIC z)

# Same libraries as the Android build; all dependencies are built for the iOS SDK.
include_directories("${ROOT}/librmx/source" "${ROOT}/Oxygen/lemonscript/source" "${ROOT}/Oxygen/oxygenengine/source" "${ROOT}/Oxygen/sonic3air/source" "${ROOT}/Oxygen/sonic3air/source/external")
add_compile_definitions(NO_DISCORD=1 GL_SILENCE_DEPRECATION=1 GLES_SILENCE_DEPRECATION=1 GL_MIN=GL_MIN_EXT GL_MAX=GL_MAX_EXT)
file(GLOB IMGUI_SOURCES "${EXT}/imgui/imgui/*.cpp")
add_library(imgui STATIC ${IMGUI_SOURCES}
  "${EXT}/imgui/imgui/backends/imgui_impl_opengl3.cpp"
  "${EXT}/imgui/imgui/backends/imgui_impl_sdl2.cpp")
target_include_directories(imgui PUBLIC "${EXT}/imgui/imgui")
target_compile_definitions(imgui PRIVATE IMGUI_IMPL_OPENGL_ES2)
target_link_libraries(imgui PUBLIC SDL2-static "-framework OpenGLES")
file(GLOB_RECURSE BASE "${ROOT}/librmx/source/rmxbase/*.cpp")
file(GLOB_RECURSE MEDIA "${ROOT}/librmx/source/rmxmedia/*.cpp")
file(GLOB VORBIS "${ROOT}/librmx/source/rmxext_oggvorbis/*.cpp")
file(GLOB_RECURSE LEMON "${ROOT}/Oxygen/lemonscript/source/lemon/*.cpp")
file(GLOB_RECURSE NET "${ROOT}/Oxygen/oxygenengine/source/oxygen_netcore/*.cpp")
file(GLOB_RECURSE ENGINE "${ROOT}/Oxygen/oxygenengine/source/oxygen/*.cpp")
file(GLOB_RECURSE GAME "${ROOT}/Oxygen/sonic3air/source/sonic3air/*.cpp")
list(FILTER GAME EXCLUDE REGEX "/main\\.cpp$")
add_library(rmxbase STATIC ${BASE})
target_link_libraries(rmxbase PUBLIC z)
add_library(rmxmedia STATIC ${MEDIA})
target_link_libraries(rmxmedia PUBLIC rmxbase SDL2-static "-framework OpenGLES")
add_library(rmxext_oggvorbis STATIC ${VORBIS})
target_link_libraries(rmxext_oggvorbis PUBLIC rmxmedia oggvorbis)
add_library(lemonscript STATIC ${LEMON})
target_link_libraries(lemonscript PUBLIC rmxbase)
add_library(oxygen_netcore STATIC ${NET})
target_link_libraries(oxygen_netcore PUBLIC rmxbase)
add_library(oxygen STATIC ${ENGINE})
target_link_libraries(oxygen PUBLIC minizip rmxmedia rmxext_oggvorbis lemonscript oxygen_netcore imgui)
add_executable(Sonic3AIR MACOSX_BUNDLE ${GAME} main.mm)
target_compile_options(Sonic3AIR PRIVATE "$<$<COMPILE_LANGUAGE:OBJCXX>:-fobjc-arc>")
target_link_libraries(Sonic3AIR PRIVATE SDL2main oxygen "-framework UIKit" "-framework Foundation")
set_target_properties(Sonic3AIR PROPERTIES
  MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_LIST_DIR}/Info.plist.in"
  MACOSX_BUNDLE_GUI_IDENTIFIER "org.sonic3air.native-ios"
  MACOSX_BUNDLE_BUNDLE_NAME "Sonic 3 AIR"
  MACOSX_BUNDLE_SHORT_VERSION_STRING "0.2.5"
  MACOSX_BUNDLE_BUNDLE_VERSION "11"
  XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1,2"
  XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED "NO"
  XCODE_ATTRIBUTE_ENABLE_BITCODE "NO")
set(ICON_SOURCE "${ROOT}/Oxygen/sonic3air/build/_xcode/sonic3air_iosApp/Assets.xcassets/AppIcon.appiconset")
add_custom_command(TARGET Sonic3AIR POST_BUILD
  COMMAND "${CMAKE_COMMAND}" -E copy "${ICON_SOURCE}/120.png" "$<TARGET_BUNDLE_DIR:Sonic3AIR>/AppIcon60x60@2x.png"
  COMMAND "${CMAKE_COMMAND}" -E copy "${ICON_SOURCE}/180.png" "$<TARGET_BUNDLE_DIR:Sonic3AIR>/AppIcon60x60@3x.png"
  COMMAND "${CMAKE_COMMAND}" -E copy "${ICON_SOURCE}/76.png" "$<TARGET_BUNDLE_DIR:Sonic3AIR>/AppIcon76x76.png"
  COMMAND "${CMAKE_COMMAND}" -E copy "${ICON_SOURCE}/152.png" "$<TARGET_BUNDLE_DIR:Sonic3AIR>/AppIcon76x76@2x.png"
  COMMAND "${CMAKE_COMMAND}" -E copy "${ICON_SOURCE}/167.png" "$<TARGET_BUNDLE_DIR:Sonic3AIR>/AppIcon83.5x83.5@2x.png"
  VERBATIM)
# Bundle source assets so CI needs neither an original game ROM nor a desktop game run.
# Scripts compile on device using the existing LemonScript interpreter (no JIT).
add_custom_command(TARGET Sonic3AIR POST_BUILD
  COMMAND "${CMAKE_COMMAND}" -E make_directory "$<TARGET_BUNDLE_DIR:Sonic3AIR>/content/sonic3air"
  COMMAND "${CMAKE_COMMAND}" -E copy_directory "${ROOT}/Oxygen/sonic3air/data" "$<TARGET_BUNDLE_DIR:Sonic3AIR>/content/sonic3air/data"
  COMMAND "${CMAKE_COMMAND}" -E copy_directory "${ROOT}/Oxygen/sonic3air/scripts" "$<TARGET_BUNDLE_DIR:Sonic3AIR>/content/sonic3air/scripts"
  COMMAND "${CMAKE_COMMAND}" -E copy_directory "${ROOT}/Oxygen/oxygenengine/data" "$<TARGET_BUNDLE_DIR:Sonic3AIR>/content/oxygenengine/data"
  COMMAND "${CMAKE_COMMAND}" -E copy "${CMAKE_CURRENT_LIST_DIR}/config.json" "$<TARGET_BUNDLE_DIR:Sonic3AIR>/content/sonic3air/config.json"
  VERBATIM)
