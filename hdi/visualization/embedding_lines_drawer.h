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

#ifndef EMBEDDING_LINES_DRAWER_H
#define EMBEDDING_LINES_DRAWER_H

#include <QGLShaderProgram>
#include <QGLShader>
#include <QColor>
#include <stdint.h>
#include <memory>
#include "hdi/visualization/abstract_scatterplot_drawer.h"
#include "hdi/data/embedding.h"

namespace hdi{
  namespace viz{

    class EmbeddingLinesDrawer: public AbstractScatterplotDrawer{
    public:
      EmbeddingLinesDrawer();
      //! Draw on canvas
      virtual void draw(const point_type& bl, const point_type& tr);
      //! Set the data to be drawn
      void setData(const scalar_type* embedding_src, const scalar_type* embedding_dst);
      //! Set lines between the embeddings
      void setLines(const std::vector<std::pair<uint32_t,uint32_t>>& lines);
      void setLines(const std::vector<std::pair<uint32_t,uint32_t>>& lines, const std::vector<float>& alpha_per_line);
      const std::vector<std::pair<uint32_t,uint32_t>>& lines()const {return _lines;}

      virtual void initialize(QGLContext* context);

      void setAlpha(scalar_type alpha){_alpha = alpha;}
      void setColor(color_type color){_color = color;}
      
    private:
      const scalar_type* _embedding_src;
      const scalar_type* _embedding_dst;
      std::vector<std::pair<uint32_t,uint32_t>> _lines;
      std::vector<float> _alpha_per_line;

      color_type _color;
      scalar_type _alpha;
      scalar_type _z_coord;

      bool _initialized;

    };

  }
}

#endif
