# main/CMakeLists.txt
set(srcs "main.c" "app_video.c" "app_lcd.c" "ai_infer.cpp")

idf_component_register(
  SRCS ${srcs}
  INCLUDE_DIRS "."
  REQUIRES esp-dl
  EMBED_FILES "model/pvc_classifier.espdl"
)

# (po želji)
# set(CMAKE_CXX_STANDARD 17)
# set(CMAKE_CXX_STANDARD_REQUIRED ON)

#target_link_libraries(${COMPONENT_LIB}
#    "-Wl,--whole-archive" "-Wl,--format=binary"
#    ${CMAKE_CURRENT_SOURCE_DIR}/model/pvc_classifier.espdl
#    "-Wl,--no-whole-archive"
#)
