/***************************************************************************
                          celsplashscreen.h  -  description
                             -------------------
    begin                : Tue Jan 03 23:27:30 CET 2006
    copyright            : (C) 2006 by Christophe Teyssier
    email                : chris@teyssier.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <vector>
#include <qpainter.h>
#include <qwidget.h>
#include <qstring.h>
#include <qpixmap.h>
#include <qrect.h>
#include <qcolor.h>
#include <qfont.h>
#include <kfilemetainfo.h>
#include <celestia/celestiacore.h>

class TextItem {
public:
    TextItem();
    QRect& getRect() { return rect; }
    void set(const QString& prefix, const KFileMetaInfoGroup& info);
    void setFlags(int f) { flags = f; };
    void setColor(const QString& rgb);
    void setFont(const QFont& _font) { font = _font; }
    void setContent(const QString& value) { content = value; }

    virtual ~TextItem() {};
    virtual void draw(QPainter* painter) const;

protected:
    bool disable;
    QRect rect;
    int flags;
    QColor color;
    QFont font;
    QString content;
    QString insertBefore;
    bool showBox;
};

class CelSplashScreen:public QWidget, virtual public ProgressNotifier
{
Q_OBJECT

public:
    CelSplashScreen(const QString& filename, QWidget* parent);
    virtual ~CelSplashScreen() {};
    void setPixmap( const QString &filename );
    virtual void update(const string& message);
    void repaint();
    void finish( QWidget *w );

protected:
    virtual void drawContents(QPainter *painter);
    void mousePressEvent( QMouseEvent * );

private:
    void drawContents();
    QPixmap pixmap;
    QWidget* parent;
    TextItem version;
    TextItem status;
    std::vector<TextItem> extraText;
};

