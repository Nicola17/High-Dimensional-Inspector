#include <QtGui>
#include <QtOpenGL>

#include "hdi/visualization/scatterplot_canvas_qobj.h"
#include "hdi/utils/log_helper_functions.h"
#include "opengl_helpers.h"
#include "hdi/utils/assert_by_exception.h"

namespace hdi{
  namespace viz{

     ScatterplotCanvas::ScatterplotCanvas(QWidget *parent, QGLWidget *shareWidget)
       : QGLWidget(parent, shareWidget),
       _logger(nullptr),
       _verbose(false),
       _bg_top_color(qRgb(255,255,255)),
       _bg_bottom_color(qRgb(255,255,255)),
       _selection_color(qRgb(50,50,50)),
       _embedding_top_right_corner(1,1),
       _embedding_bottom_left_corner(0,0),
       _selection_in_progress(false),
       _far_val(1),
       _near_val(-1)
     {
       //Q_INIT_RESOURCE(scatterplot_canvas_qobj);
     }

     ScatterplotCanvas::~ScatterplotCanvas(){
     }
    
     void ScatterplotCanvas::setBackgroundColors(const QColor &top, const QColor &bottom){
      _bg_top_color = top;
      _bg_bottom_color = bottom;
      updateGL();
     }
     void ScatterplotCanvas::getBackgroundColors(QColor &top, QColor &bottom)const{
      top  = _bg_top_color;
      bottom = _bg_bottom_color;
     }


    void ScatterplotCanvas::initializeGL(){
      glEnable(GL_DEPTH_TEST);
      glEnable(GL_CULL_FACE);
      glEnable(GL_TEXTURE_2D);

      glEnable(GL_POINT_SMOOTH);

      glEnable (GL_BLEND);
      glEnable(GL_MULTISAMPLE);
      glEnable(GL_PROGRAM_POINT_SIZE);
      glPointSize(2.5);
      /*
      GLenum err= glGetError();
      printf("OpenGL error: %d", err);
      while ((err = glGetError()) != GL_NO_ERROR) {
        printf("OpenGL error: %d", err);
      }
      */
    }

    void ScatterplotCanvas::paintGL(){
      // A bit overcomplicated... revise it
      utils::secureLog(_logger, "paintGl", _verbose);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
      
      glClearColor(0.2,0.2,0.2,1);

      
      auto camera_position = (_embedding_bottom_left_corner + _embedding_top_right_corner)*0.5;
      auto size = embeddingSize();
      glFrontFace(GL_CW);
      glMatrixMode(GL_PROJECTION);
      glLoadIdentity();
      glScalef(1/size.x()*2,1/size.y()*2,1);
      glTranslatef(-camera_position.x(),-camera_position.y(),0);
      glMatrixMode(GL_MODELVIEW);
      
      {//Background
        ScopedCapabilityEnabler depth_test_helper(GL_DEPTH_TEST);
        ScopedCapabilityDisabler texture_helper(GL_TEXTURE_2D);
        ScopedCapabilityEnabler blend_helper(GL_BLEND);
        glEnable(GL_POINT_SMOOTH);
        glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        const scalar_type background_z_coord(_near_val + (_far_val-_near_val)*.99);
                
        glBegin(GL_TRIANGLE_STRIP);  
          glColor3f  (_bg_bottom_color.redF(),_bg_bottom_color.greenF(),_bg_bottom_color.blueF());
          glVertex3f  (_embedding_bottom_left_corner.x(), _embedding_bottom_left_corner.y(), background_z_coord); //vertex 1
          glColor3f  (_bg_top_color.redF(),_bg_top_color.greenF(),_bg_top_color.blueF());
          glVertex3f  (_embedding_bottom_left_corner.x(), _embedding_top_right_corner.y(), background_z_coord); //vertex 2
          glColor3f  (_bg_bottom_color.redF(),_bg_bottom_color.greenF(),_bg_bottom_color.blueF());
          glVertex3f  (_embedding_top_right_corner.x(), _embedding_bottom_left_corner.y(), background_z_coord); //vertex 3
          glColor3f  (_bg_top_color.redF(),_bg_top_color.greenF(),_bg_top_color.blueF());
          glVertex3f  (_embedding_top_right_corner.x(), _embedding_top_right_corner.y(), background_z_coord); //vertex 4
        glEnd();
        
      }
/*
      {//Icon
        GLuint textureID;

        glGenTextures(1, &textureID); // Obtain an id for the texture
        glBindTexture(GL_TEXTURE_2D, textureID); // Set as the current texture

        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);


        //QImage im(240,120,QImage::Format_ARGB32);
        //for(int j = 0; j < 120;++j)
         //   for(int i = 0; i < 120;++i)
           //   im.setPixel(i,j,qRgb(i,j,0));

        QImage im(":/logo/cgv_logo.png");

        QImage tex = QGLWidget::convertToGLFormat(im);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex.width(), tex.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, tex.bits());

        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);

        glDisable(GL_TEXTURE_2D);


        ScopedCapabilityEnabler depth_test_helper(GL_DEPTH_TEST);
        ScopedCapabilityEnabler texture_helper(GL_TEXTURE_2D);
        ScopedCapabilityEnabler blend_helper(GL_BLEND);
        glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        const scalar_type background_z_coord(_near_val + (_far_val-_near_val)*.97);
        glBindTexture(GL_TEXTURE_2D, textureID);


        point_type bl = _embedding_bottom_left_corner;
        point_type tr = _embedding_top_right_corner;

        point_type logo_size(height()*0.2/im.height()*im.width(),height()*0.2);
        point_type pixel_size(1./width(),1./height());
        tr = (bl+(tr-bl)*logo_size*pixel_size);

        glBegin(GL_TRIANGLE_STRIP);
          glTexCoord2f(0,0);
          glVertex3f  (bl.x(), bl.y(), background_z_coord); //vertex 1
          glTexCoord2f(0,1);
          glVertex3f  (bl.x(), tr.y(), background_z_coord); //vertex 2
          glTexCoord2f(1,0);
          glVertex3f  (tr.x(), bl.y(), background_z_coord); //vertex 3
          glTexCoord2f(1,1);
          glVertex3f  (tr.x(), tr.y(), background_z_coord); //vertex 4
        glEnd();

      }
*/

      for(auto drawer: _drawers ){
        assert(drawer!=nullptr);
        drawer->draw(_embedding_bottom_left_corner,_embedding_top_right_corner);
      }
      
    //Selection
      if(_selection_in_progress){
        ScopedCapabilityEnabler blend_helper(GL_BLEND);
        glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        QVector2D bl_norm(  (std::min<float>(_selection_first_point.x(),_selection_second_point.x())/width()),
                (1.-std::max<float>(_selection_first_point.y(),_selection_second_point.y())/height())
            );
        QVector2D tr_norm(  (std::max<float>(_selection_first_point.x(),_selection_second_point.x())/width()),
                (1.-std::min<float>(_selection_first_point.y(),_selection_second_point.y())/height())
            );

        QVector2D embedding_size(_embedding_top_right_corner-_embedding_bottom_left_corner);
        QVector2D bl(_embedding_bottom_left_corner + embedding_size * bl_norm);
        QVector2D tr(_embedding_bottom_left_corner + embedding_size * tr_norm);
        
        const scalar_type selection_z_coord(_near_val + (_far_val-_near_val)*.01);
        glLineWidth(2.5);
        glColor4f(_selection_color.redF(), _selection_color.greenF(), _selection_color.blueF(), 0.45);
        glBegin(GL_LINE_STRIP);
          glVertex3f(bl.x(), bl.y(), selection_z_coord);
          glVertex3f(tr.x(), bl.y(), selection_z_coord);
          glVertex3f(tr.x(), tr.y(), selection_z_coord);
          glVertex3f(bl.x(), tr.y(), selection_z_coord);
          glVertex3f(bl.x(), bl.y(), selection_z_coord);
        glEnd();
    
        glColor4f(_selection_color.redF(), _selection_color.greenF(), _selection_color.blueF(), 0.25);
        glBegin(GL_TRIANGLE_STRIP);
          glVertex3f(bl.x(), bl.y(), selection_z_coord);
          glVertex3f(bl.x(), tr.y(), selection_z_coord);
          glVertex3f(tr.x(), bl.y(), selection_z_coord);
          glVertex3f(tr.x(), tr.y(), selection_z_coord);
        glEnd();

      }




     }

     void ScatterplotCanvas::resizeGL(int width, int height)
     {
       glViewport(0,0,width,height);
     }

     void ScatterplotCanvas::mousePressEvent(QMouseEvent *event)
     {
      utils::secureLog(_logger,"Mouse clicked...",_verbose);
      if(event->button() == Qt::LeftButton){
        _selection_first_point = event->pos();
        _selection_second_point = event->pos();
        _selection_in_progress = true;

        emit sgnLeftButtonPressed(getMousePositionInSpace(event->pos()));
      }
      if(event->button() == Qt::RightButton){
        emit sgnRightButtonPressed(getMousePositionInSpace(event->pos()));
      }
      if(event->button() == Qt::MiddleButton){
        updateGL();
      }
     }

     void ScatterplotCanvas::mouseMoveEvent(QMouseEvent *event)
     {
       if(_selection_in_progress){
        _selection_second_point = event->pos();
        updateGL();
       }
       emit sgnMouseMove(getMousePositionInSpace(event->pos()));

       if((event->buttons()&Qt::MidButton) == Qt::MidButton){
         QPoint position = event->pos();
         QVector2D lens_center(1.*position.x()/width(),1.*(height()-position.y())/height());
         updateGL();
       }
     }

    void ScatterplotCanvas::wheelEvent ( QWheelEvent * event ){
      int numDegrees = event->delta() / 8;
      updateGL();
    }

     void ScatterplotCanvas::mouseReleaseEvent(QMouseEvent *event){
      if(event->button() == Qt::LeftButton){
        _selection_second_point = event->pos();
        _selection_in_progress = false;
        updateGL();

        auto p0 = getMousePositionInSpace(_selection_first_point);
        auto p1 = getMousePositionInSpace(_selection_second_point);
        emit sgnSelection(point_type(std::min(p0.x(),p1.x()),std::min(p0.y(),p1.y())),
                  point_type(std::max(p0.x(),p1.x()),std::max(p0.y(),p1.y()))
                  );
        emit sgnLeftButtonReleased(getMousePositionInSpace(event->pos()));
      }
      if(event->button() == Qt::RightButton){
        emit sgnRightButtonReleased(getMousePositionInSpace(event->pos()));
      }
      if(event->button() == Qt::MiddleButton){
        updateGL();
      }

      emit clicked();
     }

    void ScatterplotCanvas::mouseDoubleClickEvent(QMouseEvent* event){
      if(event->button() == Qt::LeftButton){
        emit sgnLeftDoubleClick(getMousePositionInSpace(event->pos()));
      }
      if(event->button() == Qt::RightButton){
        emit sgnRightDoubleClick(getMousePositionInSpace(event->pos()));
      }
    }

    QVector2D ScatterplotCanvas::getMousePositionInSpace(QPoint pnt)const{
      const point_type tr_bl(_embedding_top_right_corner-_embedding_bottom_left_corner);
      return QVector2D(_embedding_bottom_left_corner.x()+float(pnt.x())/width()*tr_bl.x(),
               _embedding_bottom_left_corner.y()+float(height()-pnt.y())/height()*tr_bl.y());
    }

    void ScatterplotCanvas::keyPressEvent(QKeyEvent* e){
      emit sgnKeyPressed(e->key());
    }

    void ScatterplotCanvas::saveToFile(std::string filename){
      QImage image = grabFrameBuffer();
      image.save(QString::fromStdString(filename));
    }
  }
}
