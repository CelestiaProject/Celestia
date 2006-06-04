/***************************************************************************
                          celsplashscreen.cpp  -  description
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
#include <qapplication.h>
#include <qpainter.h>
#include <qrect.h>
#include <klocale.h>
#include <kglobal.h>
#include <kglobalsettings.h>
#include <celsplashscreen.h>
#include <X11/Xlib.h>

CelSplashScreen::CelSplashScreen(const QString& filename, QWidget* _parent):
    QWidget(0, 0, WStyle_Customize | WX11BypassWM), 
    parent(_parent)
{
    setPixmap(filename);
    show();
}

void CelSplashScreen::mousePressEvent( QMouseEvent * )
{
    hide();
}

void CelSplashScreen::repaint()
{
    drawContents();
    QWidget::repaint();
    QApplication::flush();
}

void CelSplashScreen::drawContents()
{
    QPixmap textPix = pixmap;
    QPainter painter( &textPix, this );
    drawContents( &painter );
    setErasePixmap( textPix );
}

void CelSplashScreen::drawContents(QPainter *painter) {
    status.draw(painter);
}

void CelSplashScreen::update(const string& _message) {
    status.setContent(_message.c_str());
    repaint();
}

void CelSplashScreen::finish( QWidget *mainWin )
{
    if ( mainWin ) {
	extern void qt_wait_for_window_manager( QWidget *mainWin );
	qt_wait_for_window_manager( mainWin );
    }
    close();
}

void CelSplashScreen::setPixmap( const QString &filename )
{
    QPixmap _pixmap(filename);
    resize( _pixmap.size() );

    //  Set default values for status and version fields
    status.getRect().setX(20);
    status.getRect().setY(height() - 40);
    status.getRect().setWidth(width() - 200);
    status.getRect().setHeight(20);
    status.setFlags(Qt::AlignLeft | Qt::AlignTop);

    version.getRect().setX(width() - 180);
    version.getRect().setY(height() - 40);
    version.getRect().setWidth(150);
    version.getRect().setHeight(20);
    version.setFlags(Qt::AlignRight | Qt::AlignTop);
    version.setContent(VERSION);

    KFileMetaInfo info(filename);
    KFileMetaInfoGroup comments = info.group("Comment");

    if (comments.isValid()) {
        status.set("status", comments);
        version.set("version", comments);
        int i = 0;
        char extraName[10];
        sprintf(extraName, "extra%02d", i);
        while (i< 100 && comments.item(QString(extraName) + "_insert_before").isValid()) {
            TextItem extra;
            extra.set(extraName, comments);
            extraText.push_back(extra);
            i++;
            sprintf(extraName, "extra%02d", i);
        }
    }

    QRect desk = KGlobalSettings::splashScreenDesktopGeometry();
    setGeometry( ( desk.width() / 2 ) - ( width() / 2 ) + desk.left(),
       ( desk.height() / 2 ) - ( height() / 2 ) + desk.top(),
         width(), height() );
    if (_pixmap.hasAlphaChannel()) {
        QPixmap bg = QPixmap::grabWindow( qt_xrootwin(), x(), y(), width(), height() );
        QPainter painter(&bg);
        painter.drawPixmap(0, 0, _pixmap);
        pixmap = bg;
    } else {
        pixmap = _pixmap;
    }
    
    QPainter painter( &pixmap, this );
    version.draw(&painter);
    for(std::vector<TextItem>::const_iterator i = extraText.begin(); i != extraText.end();  ++i)
        (*i).draw(&painter);

    repaint();
}

TextItem::TextItem():
    disable(false),
    color(QColor("white")),
    font(QFont("Arial")),
    showBox(false)
 {
    font.setPixelSize(11);
}

void TextItem::draw(QPainter *painter) const {
    if (disable) return;
    painter->setPen(color);
    painter->setFont(font);
    painter->drawText(rect, flags, insertBefore + content);
    if (showBox) painter->drawRect(rect);
}

void TextItem::setColor(const QString& rgb) {
    bool okr, okg, okb;
    int r, g, b;
    r = rgb.mid(0, 2).toInt(&okr, 16);
    g = rgb.mid(2, 2).toInt(&okg, 16);
    b = rgb.mid(4, 2).toInt(&okb, 16);
    if (okr && okg && okb) color.setRgb(r, g, b);
}

void TextItem::set(const QString& prefix, const KFileMetaInfoGroup& comments) {
    bool ok;
    int intVal;
    intVal = comments.value(prefix + "_disable").toInt(&ok);
    if (ok) disable = intVal;
    intVal = comments.value(prefix + "_x").toInt(&ok);
    if (ok) rect.setX(intVal);
    intVal = comments.value(prefix + "_y").toInt(&ok);
    if (ok) rect.setY(intVal);
    intVal = comments.value(prefix + "_width").toInt(&ok);
    if (ok) rect.setWidth(intVal);
    intVal = comments.value(prefix+ "_height").toInt(&ok);
    if (ok) rect.setHeight(intVal);
    QString sVal = comments.value(prefix + "_halign").toString();
    int _flags = -1;
    if (sVal == "left") {
        _flags = Qt::AlignLeft;
    } else if (sVal == "center") {
        _flags = Qt::AlignHCenter;
    } else if (sVal == "right") {
        _flags = Qt::AlignRight;
    }
    sVal = comments.value(prefix + "_valign").toString();
    if (sVal == "top") {
        if (_flags < 0) _flags = Qt::AlignTop;
        else _flags |= Qt::AlignTop;
    } else if (sVal == "center") {
        if (_flags < 0) _flags = Qt::AlignVCenter;
        else _flags |= Qt::AlignVCenter;
    } else if (sVal == "bottom") {
        if (_flags < 0) _flags = Qt::AlignBottom;
        else _flags |= Qt::AlignBottom;
    }
    if (_flags >= 0) flags = _flags;
    sVal = comments.value(prefix + "_font_family").toString();
    if (sVal != "") font.setFamily(sVal);
    intVal = comments.value(prefix + "_font_size").toInt(&ok);
    if (ok) font.setPixelSize(intVal);
    intVal = comments.value(prefix + "_font_is_bold").toInt(&ok);
    if (ok) font.setBold(intVal);
    sVal = comments.value(prefix + "_color").toString();
    if (sVal != "") setColor(sVal);
    intVal = comments.value(prefix + "_show_box").toInt(&ok);
    if (ok) showBox = intVal;
    sVal = comments.value(prefix + "_insert_before").toString();
    if (sVal != "") insertBefore = sVal;
}
