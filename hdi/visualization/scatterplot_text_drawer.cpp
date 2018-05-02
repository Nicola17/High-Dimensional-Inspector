/*
 *
 * Copyright (c) 2014, Nicola Pezzotti (Delft University of Technology)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright
 *  notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *  notice, this list of conditions and the following disclaimer in the
 *  documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *  must display the following acknowledgement:
 *  This product includes software developed by the Delft University of Technology.
 * 4. Neither the name of the Delft University of Technology nor the names of
 *  its contributors may be used to endorse or promote products derived from
 *  this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY NICOLA PEZZOTTI ''AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL NICOLA PEZZOTTI BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 */

#include "hdi/visualization/scatterplot_text_drawer.h"
#include <QOpenGLFunctions>
#include "opengl_helpers.h"
#include "hdi/utils/assert_by_exception.h"

#define GLSL(version, shader)  "#version " #version "\n" #shader

namespace hdi{
  namespace viz{

    ScatterplotTextDrawer::ScatterplotTextDrawer()
    {

    }

    void ScatterplotTextDrawer::initialize(QGLContext* context){
      _initialized = true;
    }

    void ScatterplotTextDrawer::setText(const std::string& text, QColor color, std::string font){
      _text = text;
      glGenTextures(1, &_textureID); // Obtain an id for the texture
      glBindTexture(GL_TEXTURE_2D, _textureID); // Set as the current texture

      glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
      glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);

      QImage im(text.size()*12,20,QImage::Format_ARGB32);
      for(int j = 0; j < im.height();++j)
         for(int i = 0; i < im.width();++i)
          im.setPixel(i,j,qRgba(255*i/im.width(),255*j/im.height(),250,0));

      if(text.size()){
        QPainter p(&im);
        p.setPen(QPen(color));
        QFont qfont(font.c_str(), 15);
        qfont.setBold(true);
        p.setFont(qfont);
        p.drawText(im.rect(), Qt::AlignLeft|Qt::AlignVCenter, text.c_str());

        _image = QGLWidget::convertToGLFormat(im);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, _image.width(), _image.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, _image.bits());

        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);

        glDisable(GL_TEXTURE_2D);
      }
    }

    void ScatterplotTextDrawer::draw(const point_type& bl, const point_type& tr){
      ScopedCapabilityDisabler depth_test_helper(GL_DEPTH_TEST);
      ScopedCapabilityEnabler texture_helper(GL_TEXTURE_2D);
      ScopedCapabilityEnabler blend_helper(GL_BLEND);
      glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

      const scalar_type background_z_coord(-0.7);
      glBindTexture(GL_TEXTURE_2D, _textureID);


      point_type img_bl = _coords;

      point_type img_tr = img_bl;
      img_tr.setY(img_tr.y()+_height);
      img_tr.setX(img_tr.x()+_height/_image.height()*_image.width());

      glBegin(GL_TRIANGLE_STRIP);
        glColor4f(1, 1, 1,_alpha);
        glTexCoord2f(0,0);
        glVertex3f  (img_bl.x(), img_bl.y(), background_z_coord); //vertex 1
        glTexCoord2f(0,1);
        glVertex3f  (img_bl.x(), img_tr.y(), background_z_coord); //vertex 2
        glTexCoord2f(1,0);
        glVertex3f  (img_tr.x(), img_bl.y(), background_z_coord); //vertex 3
        glTexCoord2f(1,1);
        glVertex3f  (img_tr.x(), img_tr.y(), background_z_coord); //vertex 4
      glEnd();
    }


  }
}
