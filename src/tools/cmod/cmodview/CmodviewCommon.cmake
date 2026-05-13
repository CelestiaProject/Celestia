set(QT_NO_CREATE_VERSIONLESS_FUNCTIONS ON)
set(QT_NO_CREATE_VERSIONLESS_TARGETS ON)

function(GetCmodviewSources)
  set(REL_CMODVIEW_SOURCES
    cmodview.cpp
    mainwindow.cpp
    materialwidget.cpp
    modelviewwidget.cpp
  )

  set(REL_CMODVIEW_HEADERS
    mainwindow.h
    materialwidget.h
    modelviewwidget.h
  )

  foreach(SRC_FILE IN LISTS REL_CMODVIEW_SOURCES)
    list(APPEND OutputSources "../cmodview/${SRC_FILE}")
  endforeach()

  set(CMODVIEW_SOURCES ${OutputSources} PARENT_SCOPE)

  foreach(SRC_FILE IN LISTS REL_CMODVIEW_HEADERS)
    list(APPEND OutputHeaders "../cmodview/${SRC_FILE}")
  endforeach()

  set(CMODVIEW_HEADERS ${OutputHeaders} PARENT_SCOPE)
endfunction()
