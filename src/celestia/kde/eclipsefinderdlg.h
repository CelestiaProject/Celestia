#ifndef ECLIPSEFINDERDLG_H
#define ECLIPSEFINDERDLG_H
#include "eclipsefinderdlgbase.uic.h"
#include <qpoint.h>

class QListViewItem;

class CelestiaCore;

class EclipseFinderDlg : public EclipseFinderDlgBase
{
    Q_OBJECT

public:
    EclipseFinderDlg( QWidget* parent = 0, CelestiaCore *appCore = 0 );
    ~EclipseFinderDlg();

public slots:
    void search();
    void gotoEclipse(QListViewItem* item, const QPoint& p, int col);

    CelestiaCore* appCore;
};

#endif // ECLIPSEFINDERDLG_H
