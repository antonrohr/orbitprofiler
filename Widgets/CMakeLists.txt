project(Widgets)

add_library(Widgets STATIC)

target_sources(Widgets PRIVATE CustomSortFilterProxyModel.cpp FilterBarTableViewWidget.cpp FilterBarTableViewWidget.ui)
target_sources(Widgets PUBLIC include/Widgets/CustomSortFilterProxyModel.h include/Widgets/FilterBarTableViewWidget.h)

target_include_directories(Widgets PUBLIC include/)
target_link_libraries(Widgets PUBLIC Qt5::Widgets OrbitBase)
set_target_properties(Widgets PROPERTIES AUTOMOC ON)
set_target_properties(Widgets PROPERTIES AUTOUIC ON)
