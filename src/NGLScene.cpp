#include <QMouseEvent>
#include <QGuiApplication>
#include <QCameraInfo>
#include "NGLScene.h"
#include <ngl/Camera.h>
#include <ngl/Light.h>
#include <ngl/Material.h>
#include <ngl/NGLInit.h>
#include <ngl/VAOPrimitives.h>
#include <ngl/ShaderLib.h>
#include <QList>
#include <QGLWidget>
#include <QColorDialog>
#include "Form.h"

//----------------------------------------------------------------------------------------------------------------------
/// @brief the increment for x/y translation with mouse movement
//----------------------------------------------------------------------------------------------------------------------
const static float INCREMENT=0.01f;
//----------------------------------------------------------------------------------------------------------------------
/// @brief the increment for the wheel zoom
//----------------------------------------------------------------------------------------------------------------------
const static float ZOOM=0.1f;

NGLScene::NGLScene()
{
  // re-size the widget to that of the parent (in that case the GLFrame passed in on construction)
  m_rotate=false;
  // mouse rotation values set to 0
  m_spinXFace=0.0f;
  m_spinYFace=0.0f;
  setTitle("Qt5 Simple NGL Demo");
  m_imageCapture=nullptr;
  initCamera();
  m_drawNormals=false;
  Form *f=new Form(this);
  f->show();
//  connect(f->ui->s_bothButton,SIGNAL(clicked()),this,SLOT(setBoth())
  //connect(f->ui.)
  m_materialColour.set(1.0f,1.0f,1.0f,1.0f);
}

void NGLScene::initCamera()
{
  if (QCameraInfo::availableCameras().count() > 0)
  {
    std::cout<<"found #"<<QCameraInfo::availableCameras().count()<<"\n";
  }

  QList<QCameraInfo> cameras = QCameraInfo::availableCameras();
  foreach (const QCameraInfo &cameraInfo, cameras)
  {
    std::cout<<"Device "<<cameraInfo.deviceName().toStdString()<<"\n";
    auto name = cameraInfo.deviceName();
    std::cout<<name.toStdString()<<"\n";
      if ( name == "0x14100000046d0825" || name == "0x1410000006038612")
      {
        m_camera= new QCamera(cameraInfo);
        m_camera->start();
        //m_camera.get()->setViewfinder()
        m_imageCapture= new QCameraImageCapture(m_camera);
        connect(m_imageCapture, SIGNAL(imageCaptured(int,QImage)), this, SLOT(processCapturedImage(int,QImage)));

      }
  }
}



NGLScene::~NGLScene()
{
  std::cout<<"Shutting down NGL, removing VAO's and Shaders\n";
}

void NGLScene::resizeGL(QResizeEvent *_event)
{
  m_width=_event->size().width()*devicePixelRatio();
  m_height=_event->size().height()*devicePixelRatio();
  // now set the camera size values as the screen size has changed
  m_cam.setShape(45.0f,(float)width()/height(),0.05f,350.0f);
}

void NGLScene::resizeGL(int _w , int _h)
{
  m_cam.setShape(45.0f,(float)_w/_h,0.05f,350.0f);
  m_width=_w*devicePixelRatio();
  m_height=_h*devicePixelRatio();
}

void NGLScene::initializeGL()
{
  // we must call that first before any other GL commands to load and link the
  // gl commands from the lib, if that is not done program will crash
  ngl::NGLInit::instance();
  glClearColor(0,0,0,1);
  // enable depth testing for drawing
  glEnable(GL_DEPTH_TEST);
  // enable multisampling for smoother drawing
  glEnable(GL_MULTISAMPLE);

  ngl::ShaderLib *shader=ngl::ShaderLib::instance();
  bool loaded=shader->loadFromJson("shaders/shaders.json");
  if(!loaded)
  {
    std::cerr<<"problem loading shaders\n";
    exit(EXIT_FAILURE);
  }

  (*shader)["nglDiffuseShader"]->use();
  shader->setRegisteredUniform("lightPos",ngl::Vec3(0,2,3));
  shader->setRegisteredUniform("Colour",1.0f,1.0f,1.0f,1.0f);
  shader->setRegisteredUniform("lightDiffuse",1.0f,1.0f,1.0f,1.0f);

  // the shader will use the currently active material and light0 so set them
  (*shader)["Phong"]->use();
  shader->printRegisteredUniforms("Phong");
  ngl::Material m;
  m.setAmbient(ngl::Colour(0,0,0));
  m.setDiffuse(ngl::Colour(0.9,0.9,0.9));
  m.setSpecular(ngl::Colour(1.0,1.0,1.0));
  m.setSpecularExponent(120.0f);
  // load our material values to the shader into the structure material (see Vertex shader)
  m.loadToShader("material");
  // Now we will create a basic Camera from the graphics library
  // This is a static camera so it only needs to be set once
  // First create Values for the camera position
  ngl::Vec3 from(0.0f,0.0f,1.5f);
  ngl::Vec3 to(0.0f,0.0f,0.0f);
  ngl::Vec3 up(0.0f,1.0f,0.0f);
  // now load to our new camera
  m_cam.set(from,to,up);
  // set the shape using FOV 45 Aspect Ratio based on Width and Height
  // The final two are near and far clipping planes of 0.5 and 10
  m_cam.setShape(45.0f,(float)720.0/576.0f,0.05f,350.0f);
  shader->setUniform("viewerPos",m_cam.getEye().toVec3());
  // now create our light that is done after the camera so we can pass the
  // transpose of the projection matrix to the light to do correct eye space
  // transformations
  ngl::Mat4 iv=m_cam.getViewMatrix();
  iv.transpose();
  ngl::Light light(ngl::Vec3(0,2,3),ngl::Colour(1,1,1,1),ngl::Colour(1,1,1,1),ngl::LightModes::POINTLIGHT );
  light.setTransform(iv);
  // load these values to the shader as well
  light.loadToShader("light");

  shader->use("Normal");
  // now pass the modelView and projection values to the shader
  shader->setShaderParam1f("normalSize",0.1f);
  shader->setShaderParam4f("vertNormalColour",1.0f,1.0f,0.0f,1.0f);
  shader->setShaderParam4f("faceNormalColour",1.0f,0.0f,0.0f,1.0f);

  shader->setShaderParam1i("drawFaceNormals",false);
  shader->setShaderParam1i("drawVertexNormals",true);

  (*shader)["Fire"]->use();
  m.setAmbient(ngl::Colour(0,0,0));
  m.setDiffuse(ngl::Colour(1.0,1.0,1.0));
  m.setSpecular(ngl::Colour(1.0,1.0,1.0));
  m.setSpecularExponent(120.0f);
  // load our material values to the shader into the structure material (see Vertex shader)
  m.loadToShader("material");
  // Now we will create a basic Camera from the graphics library
  // This is a static camera so it only needs to be set once
  // First create Values for the camera position
  shader->setUniform("viewerPos",m_cam.getEye().toVec3());
  shader->setRegisteredUniform("repeat",0.05f);

  // now create our light that is done after the camera so we can pass the
  // transpose of the projection matrix to the light to do correct eye space
  // transformations
  light.setTransform(iv);
  // load these values to the shader as well
  light.loadToShader("light");

  shader->use("nglColourShader");
  shader->setRegisteredUniform("Colour",1.0f,1.0f,1.0f,1.0f);
  startTimer(20);
  m_camTimer = new QTimer(this);
  connect(m_camTimer, SIGNAL(timeout()), this, SLOT(captureImage()));
  m_camTimer->start();
glGenTextures(1,&m_textureName);
m_screenQuad.reset(  new ScreenQuad("Texture"));

}


void NGLScene::loadMatricesToShader()
{
  ngl::ShaderLib *shader=ngl::ShaderLib::instance();
  ngl::Mat4 MV;
  ngl::Mat4 MVP;
  ngl::Mat3 normalMatrix;
  ngl::Mat4 M;
  M=m_mouseGlobalTX;
  MV=  M*m_cam.getViewMatrix();
  MVP= M*m_cam.getVPMatrix();
  normalMatrix=MV;
  normalMatrix.inverse();
  shader->setShaderParamFromMat4("MV",MV);
  shader->setShaderParamFromMat4("MVP",MVP);
  shader->setShaderParamFromMat3("normalMatrix",normalMatrix);
  shader->setShaderParamFromMat4("M",M);
}

void NGLScene::paintGL()
{
  if(m_viewMode == DrawMode::BLANK)
  {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    return;
  }
  // Rotation based on the mouse position for our global transform
  ngl::Mat4 rotX;
  ngl::Mat4 rotY;
  // create the rotation matrices
  rotX.rotateX(m_spinXFace);
  rotY.rotateY(m_spinYFace);
  // multiply the rotations
  ngl::Mat4 scale;
  scale.scale(1.0,0.8,1.0);
  m_mouseGlobalTX=rotY*rotX*scale;
  // add the translations
  m_mouseGlobalTX.m_m[3][0] = m_modelPos.m_x;
  m_mouseGlobalTX.m_m[3][1] = m_modelPos.m_y;
  m_mouseGlobalTX.m_m[3][2] = m_modelPos.m_z;
  ngl::ShaderLib *shader=ngl::ShaderLib::instance();

  if(m_viewMode==DrawMode::BOTH)
  {

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    // draw camera
    glViewport(0,0,m_width/2,m_height);
    // get the VBO instance and draw the built in teapot
    glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
    glBindTexture(GL_TEXTURE_2D,m_textureName);
    m_screenQuad->draw();


    glViewport(m_width/2,0,m_width/2,m_height);
    // clear the screen and depth buffer
    ngl::VAOPrimitives *prim=ngl::VAOPrimitives::instance();
    if(m_wireframe)
      glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);

      // grab an instance of the shader manager
    if(m_shaderMode == ShaderMode::PHONG)
      shader->use("Phong");
    else if(m_shaderMode== ShaderMode::FIRE)
      shader->use("Fire");
    else if(m_shaderMode== ShaderMode::COLOUR)
      shader->use("nglColourShader");


    // draw
    loadMatricesToShader();
    prim->draw("teapot");
    if(m_drawNormals)
    {
      (*shader)["Normal"]->use();
      ngl::Mat4 MV;
      ngl::Mat4 MVP;

      MV=m_mouseGlobalTX* m_cam.getViewMatrix();
      MVP=MV*m_cam.getProjectionMatrix();
      shader->setShaderParamFromMat4("MVP",MVP);
      shader->setShaderParam1f("normalSize",0.1);

      prim->draw("teapot");
    }
  }
  else if(m_viewMode==DrawMode::CAMERA)
  {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    // draw camera
    glViewport(0,0,m_width,m_height);
    // get the VBO instance and draw the built in teapot
    glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
    glBindTexture(GL_TEXTURE_2D,m_textureName);
    m_screenQuad->draw();
  }
  else if(m_viewMode==DrawMode::CGI)
  {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glViewport(0,0,m_width,m_height);
    // clear the screen and depth buffer
    ngl::VAOPrimitives *prim=ngl::VAOPrimitives::instance();
    if(m_wireframe)
      glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);

      // grab an instance of the shader manager
    if(m_shaderMode == ShaderMode::PHONG)
      shader->use("Phong");
    else if(m_shaderMode== ShaderMode::FIRE)
      shader->use("Fire");
    else if(m_shaderMode== ShaderMode::COLOUR)
      shader->use("nglColourShader");
    else if(m_shaderMode== ShaderMode::DIFFUSE)
      shader->use("nglDiffuseShader");

    // draw
    loadMatricesToShader();
    prim->draw("teapot");
    if(m_drawNormals)
    {
      (*shader)["Normal"]->use();
      ngl::Mat4 MV;
      ngl::Mat4 MVP;

      MV=m_mouseGlobalTX* m_cam.getViewMatrix();
      MVP=MV*m_cam.getProjectionMatrix();
      shader->setShaderParamFromMat4("MVP",MVP);
      shader->setShaderParam1f("normalSize",0.1);

      prim->draw("teapot");
    }

  }
}

void NGLScene::processCapturedImage(int requestId, const QImage& img)
{
    Q_UNUSED(requestId);
    QImage image = QGLWidget::convertToGLFormat(img);
    glBindTexture(GL_TEXTURE_2D,m_textureName);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
    int width=image.width();
    int height=image.height();

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height,
                 0, GL_RGBA, GL_UNSIGNED_BYTE, image.bits());
    glGenerateMipmap(GL_TEXTURE_2D); //  Allocate the mipmaps


}



//----------------------------------------------------------------------------------------------------------------------
void NGLScene::mouseMoveEvent (QMouseEvent * _event)
{
  // note the method buttons() is the button state when event was called
  // that is different from button() which is used to check which button was
  // pressed when the mousePress/Release event is generated
  if(m_rotate && _event->buttons() == Qt::LeftButton)
  {
    int diffx=_event->x()-m_origX;
    int diffy=_event->y()-m_origY;
    m_spinXFace += (float) 0.5f * diffy;
    m_spinYFace += (float) 0.5f * diffx;
    m_origX = _event->x();
    m_origY = _event->y();
    update();

  }
        // right mouse translate code
  else if(m_translate && _event->buttons() == Qt::RightButton)
  {
    int diffX = (int)(_event->x() - m_origXPos);
    int diffY = (int)(_event->y() - m_origYPos);
    m_origXPos=_event->x();
    m_origYPos=_event->y();
    m_modelPos.m_x += INCREMENT * diffX;
    m_modelPos.m_y -= INCREMENT * diffY;
    update();

   }
}


//----------------------------------------------------------------------------------------------------------------------
void NGLScene::mousePressEvent ( QMouseEvent * _event)
{
  // that method is called when the mouse button is pressed in this case we
  // store the value where the maouse was clicked (x,y) and set the Rotate flag to true
  if(_event->button() == Qt::LeftButton)
  {
    m_origX = _event->x();
    m_origY = _event->y();
    m_rotate =true;
  }
  // right mouse translate mode
  else if(_event->button() == Qt::RightButton)
  {
    m_origXPos = _event->x();
    m_origYPos = _event->y();
    m_translate=true;
  }

}

//----------------------------------------------------------------------------------------------------------------------
void NGLScene::mouseReleaseEvent ( QMouseEvent * _event )
{
  // that event is called when the mouse button is released
  // we then set Rotate to false
  if (_event->button() == Qt::LeftButton)
  {
    m_rotate=false;
  }
        // right mouse translate mode
  if (_event->button() == Qt::RightButton)
  {
    m_translate=false;
  }
}

//----------------------------------------------------------------------------------------------------------------------
void NGLScene::wheelEvent(QWheelEvent *_event)
{

	// check the diff of the wheel position (0 means no change)
	if(_event->delta() > 0)
	{
		m_modelPos.m_z+=ZOOM;
	}
	else if(_event->delta() <0 )
	{
		m_modelPos.m_z-=ZOOM;
	}
	update();
}
//----------------------------------------------------------------------------------------------------------------------

void NGLScene::keyPressEvent(QKeyEvent *_event)
{
  static float repeat=0.1f;
  ngl::ShaderLib *shader=ngl::ShaderLib::instance();

  // that method is called every time the main window recives a key event.
  // we then switch on the key value and set the camera in the GLWindow
  switch (_event->key())
  {
  // escape key to quit
  case Qt::Key_Escape : QGuiApplication::exit(EXIT_SUCCESS); break;
  // turn on wirframe rendering
  case Qt::Key_W : m_wireframe=true; break;
  // turn off wire frame
  case Qt::Key_S : m_wireframe=false; break;
  // show full screen
  case Qt::Key_F : showFullScreen(); break;
  // show windowed
  case Qt::Key_N : showNormal(); break;
  case Qt::Key_1 : m_drawNormals ^=true; break;
  case Qt::Key_2 : m_shaderMode=ShaderMode::PHONG; break;
  case Qt::Key_3 : m_shaderMode=ShaderMode::FIRE; break;
  case Qt::Key_5 :
    repeat-=0.01;
    ngl::ShaderLib::instance()->setRegisteredUniform("repeat",repeat);
  break;
  case Qt::Key_6 :
    repeat+=0.01;
    ngl::ShaderLib::instance()->setRegisteredUniform("repeat",repeat);
  break;

  case Qt::Key_R :
    std::cout<<"setting light to red\n";
    shader->use("Phong");
    shader->setUniform("light.diffuse",1.0f,0.0f,0.0f,1.0f);
    break;

  case Qt::Key_G :
    std::cout<<"setting light to red\n";
    shader->use("Phong");
    shader->setUniform("light.diffuse",0.0f,1.0f,0.0f,1.0f);
    break;
  case Qt::Key_B :
    std::cout<<"setting light to red\n";
    shader->use("Phong");
    shader->setUniform("light.diffuse",0.0f,0.0f,1.0f,1.0f);
    break;
  case Qt::Key_Space :
    std::cout<<"setting light to red\n";
    shader->use("Phong");
    shader->setUniform("light.diffuse",1.0f,1.0f,1.0f,1.0f);
    break;

  default : break;
  }
    update();
}

void NGLScene::setColour(int _l)
{
  ngl::ShaderLib *shader=ngl::ShaderLib::instance();
  shader->use("Phong");
  switch (_l)
  {
  case 0 :
    shader->setUniform("light.diffuse",1.0f,1.0f,1.0f,1.0f);
  break;
  case 1 :
    shader->setUniform("light.diffuse",1.0f,0.0f,0.0f,1.0f);
  break;
  case 2 :
    shader->setUniform("light.diffuse",0.0f,1.0f,0.0f,1.0f);
  break;
  case 3 :
    shader->setUniform("light.diffuse",0.0f,0.0f,1.0f,1.0f);
  break;


  }

  update();
}

void NGLScene::timerEvent(QTimerEvent *_event)
{
  if(m_shaderMode == ShaderMode::FIRE)
  {
  static float t=0.0f;
  ngl::ShaderLib *shader=ngl::ShaderLib::instance();
  shader->use("Fire");
  shader->setRegisteredUniform("time",t);
  t+=0.01;
  }
  update();
}

void NGLScene::setShaderMode(int _m)
{
  switch (_m)
  {
    case 0 : m_shaderMode=ShaderMode::PHONG; break;
    case 1 : m_shaderMode=ShaderMode::COLOUR; break;
    case 2 : m_shaderMode=ShaderMode::DIFFUSE; break;
    case 3 : m_shaderMode=ShaderMode::FIRE; break;

  }
}

void NGLScene::setMaterialColour()
{
  QColor colour = QColorDialog::getColor();
  // we now check to see if a colour was set or if the cancel was pressed
  // using the isValid method
  if (colour.isValid())
  {
    // if it was we get the colour values using the ..F() function and call the
    // SetColour method in the GLWindow
    m_materialColour.set(colour.redF(),colour.greenF(),colour.blueF(),1.0f);
    ngl::ShaderLib *shader = ngl::ShaderLib::instance();
    shader->use("nglColourShader");
    shader->setRegisteredUniform("Colour",m_materialColour);
    shader->use("nglDiffuseShader");
    shader->setRegisteredUniform("Colour",m_materialColour);
    shader->use("Phong");
    shader->setUniform("material.diffuse",m_materialColour);
  }
}

void NGLScene::setLightColour()
{
  QColor colour = QColorDialog::getColor();
  // we now check to see if a colour was set or if the cancel was pressed
  // using the isValid method
  if (colour.isValid())
  {
    // if it was we get the colour values using the ..F() function and call the
    // SetColour method in the GLWindow
    ngl::ShaderLib *shader = ngl::ShaderLib::instance();
    shader->use("nglDiffuseShader");
    shader->setRegisteredUniform("lightDiffuse",colour.redF(),colour.greenF(),colour.blueF(),1.0f);
    shader->use("Phong");
    shader->setUniform("light.diffuse",colour.redF(),colour.greenF(),colour.blueF(),1.0f);
  }
}

void NGLScene::captureImage()
{
  if(m_imageCapture!=nullptr)
    m_imageCapture->capture();
  update();
}
