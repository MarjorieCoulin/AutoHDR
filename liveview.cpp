#include "liveview.h"
#include <QPainter>

LiveViewDisplay::LiveViewDisplay(QWidget *parent) : QWidget(parent)
{
    displayedImage = 0;
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
}

void LiveViewDisplay::setImage(QImage *image)
{
    displayedImage = image;
    repaint();
}

void LiveViewDisplay::paintEvent(QPaintEvent *)
{
    if (!displayedImage)
        return;

    QPainter painter(this);
    painter.drawImage(rect(), *displayedImage, displayedImage->rect());
}
