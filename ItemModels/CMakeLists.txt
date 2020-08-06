project(ItemModels CXX)

add_library(ItemModels STATIC)

target_sources(ItemModels PRIVATE FunctionItemModel.cpp)
target_sources(ItemModels PUBLIC include/ItemModels/FunctionItemModel.h)

target_include_directories(ItemModels PUBLIC include/)
target_link_libraries(ItemModels PUBLIC OrbitBase OrbitProtos OrbitCore
                                        Qt5::Core)