// qttxf - a Qt-based application to generate GLUT txf files from
// system fonts
//
// Copyright (C) 2009, Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _QTTXF_MAINWINDOW_H_
#define _QTTXF_MAINWINDOW_H_

#include <QMainWindow>
#include <QString>

class QComboBox;
class QFontComboBox;
class QScrollArea;
class QLabel;
class QFont;
class QDataStream;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow();

    bool buildTxf();
    bool buildTxf(const QFont& font, QDataStream& out, int texWidth, int texHeight);

public slots:
    void findStyles(const QFont& font);
    void findSizes(const QFont& font);
    void updateFont(const QFont& font);
    void updateSize(const QString& sizeString);
    void updateStyle(const QString& styleName);
    void saveFont();

private:
    QScrollArea *m_scrollArea;
    QLabel* m_imageWidget;
    QFontComboBox* m_fontCombo;
    QComboBox* m_sizeCombo;
    QComboBox* m_styleCombo;
    QAction* m_saveAction;
    QFont m_currentFont;
    QByteArray m_fontData;
};

#endif // _QTTXF_MAINWINDOW_H_

