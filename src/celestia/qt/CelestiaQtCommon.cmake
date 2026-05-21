set(QT_NO_CREATE_VERSIONLESS_FUNCTIONS ON)
set(QT_NO_CREATE_VERSIONLESS_TARGETS ON)

function(GetQtSources _var_name)
  set(REL_QT_SOURCES
    qtappwin.cpp
    qtappwin.h
    qtbookmark.cpp
    qtbookmark.h
    qtcelestiaactions.cpp
    qtcelestiaactions.h
    qtcelestialbrowser.cpp
    qtcelestialbrowser.h
    qtcolorswatchwidget.cpp
    qtcolorswatchwidget.h
    qtcommandline.cpp
    qtcommandline.h
    qtdateutil.cpp
    qtdateutil.h
    qtdeepskybrowser.cpp
    qtdeepskybrowser.h
    qtdraghandler.cpp
    qtdraghandler.h
    qteventfinder.cpp
    qteventfinder.h
    qtgettext.h
    qtglwidget.cpp
    qtglwidget.h
    qtgotoobjectdialog.cpp
    qtgotoobjectdialog.h
    qtinfopanel.cpp
    qtinfopanel.h
    qtmain.cpp
    qtpathutil.h
    qtpreferencesdialog.cpp
    qtpreferencesdialog.h
    qtselectionpopup.cpp
    qtselectionpopup.h
    qtsettimedialog.cpp
    qtsettimedialog.h
    qtsolarsystembrowser.cpp
    qtsolarsystembrowser.h
    qttimetoolbar.cpp
    qttimetoolbar.h
    qttourguide.cpp
    qttourguide.h
    xbel.cpp
    xbel.h
  )

  if(WIN32)
    list(APPEND REL_QT_SOURCES celestia.rc)
  endif()

  foreach(SRC_FILE IN LISTS REL_QT_SOURCES)
    list(APPEND OutputSources "../qt/${SRC_FILE}")
  endforeach()

  set(${_var_name} ${OutputSources} PARENT_SCOPE)
endfunction()
