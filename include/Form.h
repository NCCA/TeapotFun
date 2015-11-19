#ifndef FORM_H
#define FORM_H
#include "NGLScene.h"
#include <QWidget>

namespace Ui {
  class Form;
  }

class Form : public QWidget
{
  Q_OBJECT

public:
  explicit Form(NGLScene *scene,QWidget *parent = 0);
  ~Form();

private:
  Ui::Form *ui;
  NGLScene *scene;
};

#endif // FORM_H
