#include "Form.h"
#include "ui_Form.h"
Form::Form(NGLScene *_scene,QWidget *parent) :
  QWidget(parent),
  ui(new Ui::Form)
{
  ui->setupUi(this);
  scene=_scene;
  connect(ui->s_bothButton,SIGNAL(clicked()),scene,SLOT(setBoth()));
  connect(ui->s_blankButton,SIGNAL(clicked()),scene,SLOT(setBlank()));

  connect(ui->s_cgiButton,SIGNAL(clicked()),scene,SLOT(setCGI()));
  connect(ui->s_cameraButton,SIGNAL(clicked()),scene,SLOT(setCamera()));
  connect(ui->s_showNormals,SIGNAL(toggled(bool)),scene,SLOT(setNormals(bool)));

  connect(ui->s_lightMode,SIGNAL(currentIndexChanged(int)),scene,SLOT(setColour(int)));
  connect(ui->s_shaderMode,SIGNAL(currentIndexChanged(int)),scene,SLOT(setShaderMode(int)));
  connect(ui->s_colourButton,SIGNAL(clicked()),scene,SLOT(setMaterialColour()));
  connect(ui->s_lightColour,SIGNAL(clicked()),scene,SLOT(setLightColour()));

  setWindowFlags(Qt::WindowStaysOnTopHint | Qt::WindowTitleHint | Qt::WindowMinMaxButtonsHint);


}

Form::~Form()
{
  delete ui;
}
