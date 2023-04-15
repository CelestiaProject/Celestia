set(QT_NO_CREATE_VERSIONLESS_FUNCTIONS ON)
set(QT_NO_CREATE_VERSIONLESS_TARGETS ON)

function(GetQtSources UseWayland)
  set(REL_QT_SOURCES
    qtappwin.cpp
    qtbookmark.cpp
    qtcelestialbrowser.cpp
    qtcelestiaactions.cpp
    qtcolorswatchwidget.cpp
    qtcommandline.cpp
    qtdeepskybrowser.cpp
    qtdraghandler.cpp
    qteventfinder.cpp
    qtglwidget.cpp
    qtgotoobjectdialog.cpp
    qtinfopanel.cpp
    qtmain.cpp
    qtpreferencesdialog.cpp
    qtselectionpopup.cpp
    qtsettimedialog.cpp
    qtsolarsystembrowser.cpp
    qttimetoolbar.cpp
    xbel.cpp
  )

  set(REL_QT_HEADERS
    qtappwin.h
    qtbookmark.h
    qtcelestialbrowser.h
    qtcelestiaactions.h
    qtcolorswatchwidget.h
    qtcommandline.h
    qtdeepskybrowser.h
    qtdraghandler.h
    qteventfinder.h
    qtgettext.h
    qtgotoobjectdialog.h
    qtglwidget.h
    qtinfopanel.h
    qtpreferencesdialog.h
    qtselectionpopup.h
    qtsettimedialog.h
    qtsolarsystembrowser.h
    qttimetoolbar.h
    xbel.h
  )

  if(UseWayland)
    list(APPEND REL_QT_SOURCES qtwaylanddraghandler.cpp)
    list(APPEND REL_QT_HEADERS qtwaylanddraghandler.h)
  endif()

  if(WIN32)
    set(REL_RES celestia.rc)
  endif()

  foreach(SRC_FILE IN LISTS REL_QT_SOURCES)
    list(APPEND OutputSources "../qt/${SRC_FILE}")
  endforeach()

  set(QT_SOURCES ${OutputSources} PARENT_SCOPE)

  foreach(SRC_FILE IN LISTS REL_QT_HEADERS)
    list(APPEND OutputHeaders "../qt/${SRC_FILE}")
  endforeach()

  set(QT_HEADERS ${OutputHeaders} PARENT_SCOPE)

  foreach(SRC_FILE IN LISTS REL_RES)
    list(APPEND OutputRes "../qt/${SRC_FILE}")
  endforeach()

  set(RES ${OutputRes} PARENT_SCOPE)
endfunction()
