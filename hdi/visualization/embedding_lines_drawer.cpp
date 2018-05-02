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

#include "hdi/visualization/embedding_lines_drawer.h"
#include <QOpenGLFunctions>
#include "opengl_helpers.h"
#include "hdi/utils/assert_by_exception.h"

#define GLSL(version, shader)  "#version " #version "\n" #shader

namespace hdi{
  namespace viz{

    EmbeddingLinesDrawer::EmbeddingLinesDrawer():
      _color(qRgb(0,0,0)),
      _initialized(false),
      _alpha(0.5),
      _z_coord(0)
    {
      //initialize();

    }

    void EmbeddingLinesDrawer::initialize(QGLContext* context){
      _initialized = true;
    }

    void EmbeddingLinesDrawer::draw(const point_type& bl, const point_type& tr){
      checkAndThrowLogic(_initialized,"Shader must be initilized");
      ScopedCapabilityEnabler blend_helper(GL_BLEND);
      ScopedCapabilityEnabler depth_test_helper(GL_DEPTH_TEST);
      ScopedCapabilityEnabler point_smooth_helper(GL_POINT_SMOOTH);
      ScopedCapabilityEnabler multisample_helper(GL_MULTISAMPLE);
      ScopedCapabilityEnabler point_size_helper(GL_PROGRAM_POINT_SIZE);
      glDepthMask(GL_FALSE);

      for(int i = 0; i < _lines.size(); ++i){
        auto& line = _lines[i];
        glBegin(GL_LINES);
          glColor4f  (_color.redF(),_color.greenF(),_color.blueF(),_alpha_per_line.size()?_alpha_per_line[i]:_alpha);
          glVertex3f  (_embedding_src[line.first*2], _embedding_src[line.first*2+1], _z_coord); //vertex 1
          glVertex3f  (_embedding_dst[line.second*2], _embedding_dst[line.second*2+1], _z_coord); //vertex 2
        glEnd();
      }
    }

    void EmbeddingLinesDrawer::setData(const scalar_type* embedding_src, const scalar_type* embedding_dst){
      _embedding_src = embedding_src;
      _embedding_dst = embedding_dst;
    }

    void EmbeddingLinesDrawer::setLines(const std::vector<std::pair<uint32_t,uint32_t>>& lines){
      _lines = lines;
      _alpha_per_line.clear();
    }
    void EmbeddingLinesDrawer::setLines(const std::vector<std::pair<uint32_t,uint32_t>>& lines, const std::vector<float>& alpha_per_line){
      assert(lines.size() == alpha_per_line.size());
      _lines = lines;
      _alpha_per_line = alpha_per_line;
    }

  }
}
