// qttxf - a Qt-based application to generate GLUT txf files from
// system fonts
//
// Copyright (C) 2009, Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <QtGui>
#include "mainwindow.h"


// TXF format constants
const char* TXF_HEADER_BYTES = "\377txf";
const quint32 TXF_ENDIANNESS_TEST = 0x12345678;


MainWindow::MainWindow() :
    m_scrollArea(NULL),
    m_imageWidget(NULL),
    m_fontCombo(NULL),
    m_sizeCombo(NULL),
    m_styleCombo(NULL),
    m_saveAction(NULL)
{
    QWidget *centralWidget = new QWidget();

    QLabel *fontLabel = new QLabel(tr("Font:"));
    m_fontCombo = new QFontComboBox;
    QLabel *sizeLabel = new QLabel(tr("Size:"));
    m_sizeCombo = new QComboBox;
    QLabel *styleLabel = new QLabel(tr("Style:"));
    m_styleCombo = new QComboBox;

    m_scrollArea = new QScrollArea();
    m_imageWidget = new QLabel();

    m_scrollArea->setWidget(m_imageWidget);

    findStyles(m_fontCombo->currentFont());
    findSizes(m_fontCombo->currentFont());

    connect(m_fontCombo, SIGNAL(currentFontChanged(const QFont &)),
            this, SLOT(findStyles(const QFont &)));
    connect(m_fontCombo, SIGNAL(currentFontChanged(const QFont &)),
            this, SLOT(findSizes(const QFont &)));
    connect(m_fontCombo, SIGNAL(currentFontChanged(const QFont &)),
            this, SLOT(updateFont(const QFont &)));
    connect(m_sizeCombo, SIGNAL(currentIndexChanged(const QString &)),
            this, SLOT(updateSize(const QString &)));
    connect(m_styleCombo, SIGNAL(currentIndexChanged(const QString &)),
            this, SLOT(updateStyle(const QString &)));

    QHBoxLayout *controlsLayout = new QHBoxLayout;
    controlsLayout->addWidget(fontLabel);
    controlsLayout->addWidget(m_fontCombo, 1);
    controlsLayout->addWidget(sizeLabel);
    controlsLayout->addWidget(m_sizeCombo, 1);
    controlsLayout->addWidget(styleLabel);
    controlsLayout->addWidget(m_styleCombo, 1);

    QVBoxLayout *centralLayout = new QVBoxLayout();
    centralLayout->addLayout(controlsLayout);
    centralLayout->addWidget(m_scrollArea, 1);
    centralWidget->setLayout(centralLayout);

    setCentralWidget(centralWidget);
    setWindowTitle("QtTXF");

    QMenuBar* menuBar = new QMenuBar(this);

    QMenu* fileMenu = new QMenu(tr("File"));
    m_saveAction = new QAction(tr("&Save..."), this);
    QAction* quitAction = new QAction(tr("&Quit"), this);
    fileMenu->addAction(m_saveAction);
    fileMenu->addAction(quitAction);
    menuBar->addMenu(fileMenu);

    setMenuBar(menuBar);

    m_saveAction->setShortcut(QKeySequence::Save);
    connect(m_saveAction, SIGNAL(triggered()), this, SLOT(saveFont()));
    quitAction->setShortcut(QKeySequence("Ctrl+Q"));
    connect(quitAction, SIGNAL(triggered()), this, SLOT(close()));

    buildTxf();
}


void MainWindow::findStyles(const QFont &font)
{
    QFontDatabase fontDatabase;
    QString currentItem = m_styleCombo->currentText();
    m_styleCombo->clear();

    QString style;
    foreach (style, fontDatabase.styles(font.family()))
    {
        m_styleCombo->addItem(style);
    }

    int styleIndex = m_styleCombo->findText(currentItem);

    if (styleIndex == -1)
        m_styleCombo->setCurrentIndex(0);
    else
        m_styleCombo->setCurrentIndex(styleIndex);
}


void MainWindow::findSizes(const QFont &font)
{
    QFontDatabase fontDatabase;
    QString currentSize = m_sizeCombo->currentText();
    m_sizeCombo->blockSignals(true);
    m_sizeCombo->clear();

    int size;
    if (fontDatabase.isSmoothlyScalable(font.family(), fontDatabase.styleString(font)))
    {
        foreach(size, QFontDatabase::standardSizes())
        {
            m_sizeCombo->addItem(QVariant(size).toString());
            m_sizeCombo->setEditable(true);
        }
    }
    else
    {
        foreach (size, fontDatabase.smoothSizes(font.family(), fontDatabase.styleString(font)))
        {
            m_sizeCombo->addItem(QVariant(size).toString());
            m_sizeCombo->setEditable(false);
        }
    }

    m_sizeCombo->blockSignals(false);

    int sizeIndex = m_sizeCombo->findText(currentSize);
    if (sizeIndex == -1)
    {
        m_sizeCombo->setCurrentIndex(qMax(0, m_sizeCombo->count() / 3));
    }
    else
    {
        m_sizeCombo->setCurrentIndex(sizeIndex);
    }
}


void
MainWindow::updateFont(const QFont& font)
{
    qDebug() << font.family() << " match: " << font.exactMatch();
    m_currentFont.setFamily(font.family());
    buildTxf();
}


void
MainWindow::updateSize(const QString& sizeString)
{
    m_currentFont.setPointSize(sizeString.toInt());
    buildTxf();
}


void
MainWindow::updateStyle(const QString& /* styleName */)
{
    buildTxf();
}


void
MainWindow::saveFont()
{
    if (!m_fontData.isEmpty())
    {
        QString fileName = QFileDialog::getSaveFileName(this,
                                                        tr("Save Font File"),
                                                        "",
                                                        tr("Texture Fonts (*.txf)"));
        QFile file(fileName);
        if (!file.open(QIODevice::WriteOnly))
        {
            QMessageBox::warning(this, tr("File Error"), tr("Error writing to %1").arg(fileName));
            return;
        }

        QDataStream out(&file);
        out.writeRawData(m_fontData.data(), m_fontData.length());
        file.close();
    }
}


struct BasicGlyphInfo
{
    QChar ch;
    int height;
};

bool operator<(const BasicGlyphInfo& info0, const BasicGlyphInfo& info1)
{
    return info0.height > info1.height;
}


bool
MainWindow::buildTxf()
{
    // Build a txf font from the current system font. Attempt to fit it into a 128x128
    // texture, progressively increasing the texture size until it fits.
    bool fitsInTexture = false;
    int textureWidth = 128;
    int textureHeight = 128;

    while (textureWidth <= 1024 && !fitsInTexture)
    {
        m_fontData.clear();
        QDataStream out(&m_fontData, QIODevice::WriteOnly);
        if (buildTxf(m_currentFont, out, textureWidth, textureHeight))
        {
            fitsInTexture = true;
        }

        if (textureWidth == textureHeight)
            textureWidth *= 2;
        else
            textureHeight *= 2;
    }

    if (!fitsInTexture)
    {
        QMessageBox::warning(this,
                             tr("Font Error"),
                             tr("Font is too large to fit in texture"));
        m_fontData.clear();
    }

    m_saveAction->setEnabled(fitsInTexture);

    return fitsInTexture;
}


QString characterRange(unsigned int firstChar, unsigned int lastChar)
{
    QString s;
    for (unsigned int i = firstChar; i <= lastChar; ++i)
    {
        s += QChar(i);
    }

    return s;
}


bool
MainWindow::buildTxf(const QFont& font, QDataStream& out, int texWidth, int texHeight)
{
    QString charset;

    charset += characterRange(0x0020, 0x007e); // ASCII
    charset += characterRange(0x00a0, 0x00ff); // Latin-1 supplement
    charset += characterRange(0x0100, 0x017f); // Latin Extended-A
    charset += characterRange(0x0391, 0x03ce); // Greek

    QPixmap pixmap(texWidth, texHeight);
    QPainter painter(&pixmap);

    QVector<BasicGlyphInfo> glyphInfoList;

    QFont devFont(font, &pixmap);
    QFontMetrics fm(devFont);
    for (int i = 0; i < charset.length(); i++)
    {
        QChar ch = charset[i];
        if (fm.inFont(ch))
        {
            BasicGlyphInfo info;
            info.ch = ch;
            info.height = fm.boundingRect(ch).height();
            glyphInfoList << info;
        }
    }

    // Sort the glyphs by height so that they pack more compactly
    // into the available space.
    qSort(glyphInfoList);

    if (glyphInfoList.isEmpty())
    {
        return false;
    }

    // Write txf file header
    int maxAscent = 0;
    int maxDescent = 0;
    out.writeRawData(TXF_HEADER_BYTES, 4);
    out << TXF_ENDIANNESS_TEST;
    out << (quint32) 0;
    out << (quint32) texWidth << (quint32) texHeight;
    out << (quint32) maxAscent << (quint32) maxDescent;
    out << (quint32) glyphInfoList.size();

    // Clear the image
    painter.fillRect(0, 0, texWidth, texHeight, Qt::black);

    int rowHeight = glyphInfoList.first().height;
    int x = 1;
    int y = rowHeight;
    int xSpacing = 3;
    int ySpacing = 3;

    painter.setFont(devFont);
    foreach (BasicGlyphInfo info, glyphInfoList)
    {
        QRect bounds = fm.boundingRect(info.ch);
        if (x + bounds.width() >= texWidth)
        {
            y += rowHeight + ySpacing;
            rowHeight = bounds.height();
            x = 1;

            if (y >= texHeight)
            {
                qDebug() << "Not enough room in font glyph texture.";
                return false;
            }
        }

        painter.setPen(Qt::white);
        painter.drawText(x - bounds.left(), y - bounds.bottom(), QString(info.ch));

#if 0
        // Show bounding rectangles for debugging
        painter.setPen(Qt::red);
        QRect glyphRect = bounds;
        glyphRect.translate(x - bounds.left(), y - bounds.bottom());
        painter.drawRect(glyphRect);
#endif

        // Write out the glyph record;
        out << (quint16) info.ch.unicode();
        out << (quint8) (bounds.width() + 2) << (quint8) (bounds.height() + 2);
        out << (qint8) bounds.left() << (qint8) (-bounds.bottom());
        out << (qint8) fm.width(info.ch);
        out << (quint8) 0;   /* unused */
        out << (quint16) (x - 1) << (quint16) (texHeight - y - 2);

        x += bounds.width() + xSpacing;
    }

    // Write out the glyph texture map
    QImage glyphImage = pixmap.toImage();
    for (int iy = 0; iy < texHeight; iy++)
    {
        for (int ix = 0; ix < texWidth; ix++)
        {
            QRgb rgb = glyphImage.pixel(ix, texHeight - iy - 1);
            out << (quint8) qGreen(rgb);
        }
    }

    QLabel* label = new QLabel(m_scrollArea);
    label->setPixmap(pixmap);
    m_scrollArea->setWidget(label);

    return true;
}
