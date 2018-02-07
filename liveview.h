#ifndef LIVEVIEW_H
#define LIVEVIEW_H

#include <QWidget>

class LiveViewDisplay : public QWidget
{
    Q_OBJECT

  public:
    explicit LiveViewDisplay(QWidget *parent = nullptr);
    void setImage(QImage *image);

  private:
    QImage *displayedImage;

  protected:
    void paintEvent(QPaintEvent *event);
};

#endif // LIVEVIEW_H
