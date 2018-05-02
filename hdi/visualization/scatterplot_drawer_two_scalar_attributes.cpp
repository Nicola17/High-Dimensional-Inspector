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

#include "hdi/visualization/scatterplot_drawer_two_scalar_attributes.h"
#include <QOpenGLFunctions>
#include "opengl_helpers.h"
#include "hdi/utils/assert_by_exception.h"

#define GLSL(version, shader)  "#version " #version "\n" #shader

namespace hdi{
  namespace viz{

    ScatterplotDrawerTwoScalarAttributes::ScatterplotDrawerTwoScalarAttributes():
      _program(nullptr),
      _vshader(nullptr),
      _fshader(nullptr),
      _selection_color(qRgb(255,150,0)),
      _initialized(false),
      _point_size(20),
      _min_point_size(2),
      _alpha(.7),
      _z_coord(0),
      _z_coord_selection(-0.5),
      _min_val_size(0),
      _max_val_size(1),
      _min_val_color(0),
      _max_val_color(1)
    {
      //initialize();

    }

    void ScatterplotDrawerTwoScalarAttributes::initialize(QGLContext* context){
      {//Points color
        const char *vsrc = GLSL(130,
          in highp vec4 pos_attribute;        
          in highp float scalar_attribute_size;
          in highp float scalar_attribute_color;
          in highp float flag_attribute;
          uniform highp mat4 matrix_uniform;        
          uniform highp float alpha_uniform;
          uniform highp float z_coord_uniform;
          uniform highp float min_size_uniform;
          uniform highp float max_size_uniform;
          uniform highp float min_color_uniform;
          uniform highp float max_color_uniform;
          uniform highp float pnt_size_uniform;
          uniform highp float min_pnt_size_uniform;
          uniform highp vec4 color0_uniform;
          uniform highp vec4 color1_uniform;
          uniform highp vec4 color2_uniform;
          uniform highp vec4 color3_uniform;
          uniform highp vec4 selection_color_uniform;
          out lowp vec4 col;            

          void main() {
            float v_size = (scalar_attribute_size-min_size_uniform)/(max_size_uniform-min_size_uniform+0.0001);
            if(v_size > 1){ v_size = 1; }
            if(v_size < 0){ v_size = 0; }

            float v_color = (scalar_attribute_color-min_color_uniform)/(max_color_uniform-min_color_uniform+0.0001);
            if(v_color > 1){ v_color = 1; }
            if(v_color < 0){ v_color = 0; }

            if((int(0.01+flag_attribute)%2) == 1){
              col = selection_color_uniform;
            }else{
              if(v_color < 0){
                col = color0_uniform;
              /*}else if(v_color < 0.25){
                col = color0_uniform;
              }else if(v_color < 0.5){
                col = color1_uniform;
              }else if(v_color < 0.75){
                col = color2_uniform;
                */
              }else if(v_color < 0.70){
                float v = v_color/0.70;
                col = color3_uniform * v + color0_uniform * (1.00-v);
              }else{
                col = color3_uniform;
              }
            }
            col.a = alpha_uniform;

            gl_Position = matrix_uniform * pos_attribute;
            gl_Position.z = z_coord_uniform;
            gl_PointSize = min_pnt_size_uniform + v_size * (pnt_size_uniform-min_pnt_size_uniform);
          }                    
        );
        const char *fsrc = GLSL(130,
          in lowp vec4 col;            
          void main() {                
             gl_FragColor = col;
          }                      
        );

        _vshader = std::unique_ptr<QGLShader>(new QGLShader(QGLShader::Vertex));
        _fshader = std::unique_ptr<QGLShader>(new QGLShader(QGLShader::Fragment));

        _vshader->compileSourceCode(vsrc);
        _fshader->compileSourceCode(fsrc);


        _program = std::unique_ptr<QGLShaderProgram>(new QGLShaderProgram());
        _program->addShader(_vshader.get());
        _program->addShader(_fshader.get());
        _program->link();

        _coords_attribute     = _program->attributeLocation("pos_attribute");
        _flags_attribute    = _program->attributeLocation("flag_attribute");
        _scalar_attribute_size  = _program->attributeLocation("scalar_attribute_size");
        _scalar_attribute_color = _program->attributeLocation("scalar_attribute_color");

        _z_coord_uniform  = _program->uniformLocation("z_coord_uniform");
        _alpha_uniform    = _program->uniformLocation("alpha_uniform");
        _matrix_uniform    = _program->uniformLocation("matrix_uniform");
        _min_size_uniform   = _program->uniformLocation("min_size_uniform");
        _max_size_uniform   = _program->uniformLocation("max_size_uniform");
        _min_color_uniform  = _program->uniformLocation("min_color_uniform");
        _max_color_uniform  = _program->uniformLocation("max_color_uniform");
        _pnt_size_uniform   = _program->uniformLocation("pnt_size_uniform");
        _color0_uniform   = _program->uniformLocation("color0_uniform");
        _color1_uniform   = _program->uniformLocation("color1_uniform");
        _color2_uniform   = _program->uniformLocation("color2_uniform");
        _color3_uniform   = _program->uniformLocation("color3_uniform");
        _selection_color_uniform   = _program->uniformLocation("selection_color_uniform");
        _min_pnt_size_uniform   = _program->uniformLocation("min_pnt_size_uniform");

        _program->release();
      }

      _initialized = true;
    }

    void ScatterplotDrawerTwoScalarAttributes::draw(const point_type& bl, const point_type& tr){
      checkAndThrowLogic(_initialized,"Shader must be initilized");
      ScopedCapabilityEnabler blend_helper(GL_BLEND);
      ScopedCapabilityEnabler depth_test_helper(GL_DEPTH_TEST);
      ScopedCapabilityEnabler point_smooth_helper(GL_POINT_SMOOTH);
      ScopedCapabilityEnabler multisample_helper(GL_MULTISAMPLE);
      ScopedCapabilityEnabler point_size_helper(GL_PROGRAM_POINT_SIZE);
      glDepthMask(GL_FALSE);

      {
        glPointSize(_point_size);
        _program->bind();
          QMatrix4x4 matrix;
          matrix.ortho(bl.x(), tr.x(), bl.y(), tr.y(), 1, -1);

          _program->setUniformValue(_min_size_uniform, _min_val_size);
          _program->setUniformValue(_max_size_uniform, _max_val_size);
          _program->setUniformValue(_min_color_uniform, _min_val_color);
          _program->setUniformValue(_max_color_uniform, _max_val_color);
          _program->setUniformValue(_pnt_size_uniform, _point_size);
          _program->setUniformValue(_min_pnt_size_uniform, _min_point_size);
          _program->setUniformValue(_matrix_uniform, matrix);
          _program->setUniformValue(_alpha_uniform, _alpha);
          _program->setUniformValue(_z_coord_uniform, _z_coord);
          _program->setUniformValue(_color0_uniform, _colors[0]);
          _program->setUniformValue(_color1_uniform, _colors[1]);
          _program->setUniformValue(_color2_uniform, _colors[2]);
          _program->setUniformValue(_color3_uniform, _colors[3]);
          _program->setUniformValue(_selection_color_uniform, _selection_color);

          QOpenGLFunctions glFuncs(QOpenGLContext::currentContext());
          glFuncs.glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE);
          _program->enableAttributeArray(_coords_attribute);
          glFuncs.glVertexAttribPointer(_coords_attribute, 2, GL_FLOAT, GL_FALSE, 0, _embedding);

          _program->enableAttributeArray(_scalar_attribute_size);
          glFuncs.glVertexAttribPointer(_scalar_attribute_size, 1, GL_FLOAT, GL_FALSE, 0, _attribute_size);

          _program->enableAttributeArray(_scalar_attribute_color);
          glFuncs.glVertexAttribPointer(_scalar_attribute_color, 1, GL_FLOAT, GL_FALSE, 0, _attribute_color);

          _program->enableAttributeArray(_flags_attribute);
          glFuncs.glVertexAttribPointer(_flags_attribute, 1, GL_UNSIGNED_INT, GL_FALSE, 0, _flags);

          glDrawArrays(GL_POINTS, 0, _num_points);
        _program->release();
      }

      glDepthMask(GL_TRUE);
    }

    void ScatterplotDrawerTwoScalarAttributes::setData(const scalar_type* embedding, const scalar_type* attribute_size, const scalar_type* attribute_color, const flag_type* flags, int num_points){
      _embedding      = embedding;
      _attribute_size   = attribute_size;
      _attribute_color  = attribute_color;
      _flags        = flags;
      _num_points     = num_points;
    }

    void ScatterplotDrawerTwoScalarAttributes::updateLimitsFromData(){
      {
        _min_val_size = std::numeric_limits<scalar_type>::max();
        _max_val_size = -std::numeric_limits<scalar_type>::max();
        scalar_type avg = 0; //QUICKPAPER
        for(int i = 0; i < _num_points; ++i){
          _min_val_size = std::min(_attribute_size[i],_min_val_size);
          _max_val_size = std::max(_attribute_size[i],_max_val_size);
        }
      }
      {
        _min_val_color = std::numeric_limits<scalar_type>::max();
        _max_val_color = -std::numeric_limits<scalar_type>::max();
        scalar_type avg = 0; //QUICKPAPER
        for(int i = 0; i < _num_points; ++i){
          _min_val_color = std::min(_attribute_color[i],_min_val_color);
          _max_val_color = std::max(_attribute_color[i],_max_val_color);
        }
        if(_min_val_color == _max_val_color){
          _min_val_color = _max_val_color-1;
        }
      }
    }

  }
}
