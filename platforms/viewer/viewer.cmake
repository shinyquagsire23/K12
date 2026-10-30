# The model viewer, k1view / k2view (viewer_host.cpp): the game's executable with its intro, menus and
# game loop swapped for one model in a room. Desktop only. Each desktop platform calls
# k2p_add_viewer(<the game executable>) after configuring it, and the viewer is that executable --
# its sources (main.cpp, the Windows icon and crash handler), compile settings and links -- plus:
#   k2view_engine   viewer_engine.cpp, built as the engine is (its headers, flags and dialect)
#   the panel       viewer_host.cpp and Dear ImGui (third_party/imgui: the SDL2 and OpenGL2 backends)
set(K2P_VIEWER_DIR "${CMAKE_CURRENT_LIST_DIR}")

function(k2p_add_viewer _game)
  set(_view "${K2P_GAME}view")

  add_library(k2view_engine STATIC "${K2P_VIEWER_DIR}/viewer_engine.cpp")
  set_target_properties(k2view_engine PROPERTIES CXX_STANDARD ${K2P_ENGINE_STD} CXX_EXTENSIONS ON
                                                 K2_NO_EXCEPTIONS ON)
  target_include_directories(k2view_engine PRIVATE "${K2P_DIR}/compat/include" ${K2_INCLUDE_DIRS})
  target_compile_options(k2view_engine PRIVATE ${K2_HOST_FLAGS})
  target_compile_definitions(k2view_engine PRIVATE ${K2_ENGINE_DEFS})

  set(_imgui "${K2P_DIR}/third_party/imgui")
  get_target_property(_srcs ${_game} SOURCES)
  add_executable(${_view} ${_srcs}
    "${K2P_VIEWER_DIR}/viewer_host.cpp"
    "${_imgui}/imgui.cpp" "${_imgui}/imgui_draw.cpp" "${_imgui}/imgui_tables.cpp"
    "${_imgui}/imgui_widgets.cpp"
    "${_imgui}/backends/imgui_impl_sdl2.cpp" "${_imgui}/backends/imgui_impl_opengl2.cpp")
  foreach(_p COMPILE_DEFINITIONS COMPILE_OPTIONS INCLUDE_DIRECTORIES LINK_OPTIONS LINK_DEPENDS
             WIN32_EXECUTABLE RUNTIME_OUTPUT_DIRECTORY)
    get_target_property(_v ${_game} ${_p})
    if(_v)
      set_property(TARGET ${_view} PROPERTY ${_p} "${_v}")
    endif()
  endforeach()
  get_target_property(_deps ${_game} MANUALLY_ADDED_DEPENDENCIES)
  if(_deps)
    add_dependencies(${_view} ${_deps})
  endif()
  get_target_property(_libs ${_game} LINK_LIBRARIES)
  target_link_libraries(${_view} PRIVATE k2view_engine ${_libs})
  target_include_directories(${_view} PRIVATE "${_imgui}")
  target_compile_definitions(${_view} PRIVATE ${K2P_GAME_DEFS})
  # <filesystem> for the override folder; main.cpp and gamedirs.cpp are fine with it too.
  set_target_properties(${_view} PROPERTIES CXX_STANDARD 17)
  if(APPLE)
    set_target_properties(${_view} PROPERTIES XCODE_ATTRIBUTE_MACOSX_DEPLOYMENT_TARGET 10.15)
  endif()
endfunction()
