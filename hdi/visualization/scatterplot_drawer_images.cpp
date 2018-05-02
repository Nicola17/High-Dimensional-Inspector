#include "hdi/visualization/scatterplot_drawer_images.h"
#include <QOpenGLFunctions>
#include <qopenglext.h>
#include "opengl_helpers.h"



#define GLSL(version, shader)  "#version " #version "\n" #shader

namespace hdi{
  namespace viz{

    ScatterplotDrawerImages::ScatterplotDrawerImages():
      _embedding(nullptr),
      _weight(nullptr),
      _flags(nullptr),
      _num_points(0),
      _red_factor(0.09),
      _samplig_factor(0.5)
    {}

    void ScatterplotDrawerImages::initialize(QGLContext* context){

      _min_val = std::numeric_limits<scalar_type>::max();
      _max_val = -std::numeric_limits<scalar_type>::max();
      scalar_type avg = 0; //QUICKPAPER
      for(int i = 0; i < _num_points; ++i){
        _min_val = std::min(_weight[i],_min_val);
        _max_val = std::max(_weight[i],_max_val);
        avg += _weight[i];//QUICKPAPER
      }
      avg /= _num_points;
      _min_val = 0;
      _max_val = avg*1.3;

      unsigned int num_visible = 0;
      QOpenGLFunctions glFuncs(QOpenGLContext::currentContext());
      for(int i = 0; i < _num_points; ++i){
        GLuint id_text;
        glEnable(GL_TEXTURE_2D);
        glGenTextures(1,&id_text);
        _textures_id.push_back(id_text);

        glBindTexture(GL_TEXTURE_2D,id_text);
        glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,_images[i].width(),_images[i].height(),0, GL_RGBA, GL_UNSIGNED_BYTE, _images[i].bits());
        glFuncs.glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glDisable(GL_TEXTURE_2D);

        bool vis = (((rand()%1000)/1000.) < _samplig_factor) && (_weight[i] > _max_val*0.95);
        _visible.push_back(vis);
        if(vis)
          ++num_visible;
      }
      printf("ZZZZZZZ: %d",_num_points);
      printf("AAAAAAA: %d",num_visible);
      if(num_visible == 0){
         for(int i = 0; i < _num_points; ++i){
           bool vis = (((rand()%1000)/1000.) < 0.2);
           if(vis)
             ++num_visible;
           _visible[i] = vis;

         }
         printf("BBBBBBB: %d",num_visible);
      }


    }

    void ScatterplotDrawerImages::draw(const point_type& bl, const point_type& tr){
      // KDE computation
      QOpenGLFunctions glFuncs(QOpenGLContext::currentContext());
      {
        ScopedCapabilityDisabler depth_test_helper(GL_DEPTH_TEST);
        ScopedCapabilityEnabler blend_helper(GL_BLEND);
        ScopedCapabilityEnabler point_smooth_helper(GL_POINT_SMOOTH);
        ScopedCapabilityEnabler texture_helper(GL_TEXTURE_2D);

        glDepthMask(GL_FALSE);
        glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

        float size_quad = std::min(tr.x()-bl.x(),tr.y()-bl.y())*_red_factor;

        for(int i = 0; i < _num_points; ++i){
           if(!_visible[i])
             continue;
           glBindTexture(GL_TEXTURE_2D,_textures_id[i]);

           glBegin(GL_QUADS);
              //glColor3f(1,0,0);
              glTexCoord2f( 0.0f, 0.0f );
              glVertex3f(_embedding[i*2]-size_quad, _embedding[i*2+1]+size_quad, 0.0f);        // Top Left
              //glColor3f(1,1,0);
              glTexCoord2f( 1.0f, 0.0f );
              glVertex3f(_embedding[i*2]+size_quad, _embedding[i*2+1]+size_quad, 0.0f);        // Top Right
              //glColor3f(1,0,1);
              glTexCoord2f( 1.0f, 1.0f );
              glVertex3f(_embedding[i*2]+size_quad, _embedding[i*2+1]-size_quad, 0.0f);        // Bottom Right
              //glColor3f(0,1,1);
              glTexCoord2f( 0.0f, 1.0f );
              glVertex3f(_embedding[i*2]-size_quad, _embedding[i*2+1]-size_quad, 0.0f);        // Bottom Left
           glEnd();

        }
        glDepthMask(GL_TRUE);
      }
    }
  }
}
