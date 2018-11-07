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

#ifndef SCATTERPLOT_DRAWER_TWO_SCALAR_ATTRIBUTES_H
#define SCATTERPLOT_DRAWER_TWO_SCALAR_ATTRIBUTES_H

#include <stdint.h>
#include <QColor>
#include <QGLShader>
#include <QGLShaderProgram>
#include <memory>
#include "hdi/visualization/abstract_scatterplot_drawer.h"

namespace hdi {
namespace viz {

class ScatterplotDrawerTwoScalarAttributes : public AbstractScatterplotDrawer {
 public:
  ScatterplotDrawerTwoScalarAttributes();
  //! Draw on canvas
  virtual void draw(const point_type& bl, const point_type& tr);
  //! Set the data to be drawn
  void setData(const scalar_type* embedding, const scalar_type* attribute_size, const scalar_type* attribute_color, const flag_type* flags, int num_points);

  virtual void initialize(QGLContext* context);

  void setPointSize(scalar_type point_size) { _point_size = point_size; }
  void setMinPointSize(scalar_type min_point_size) { _min_point_size = min_point_size; }
  void setAlpha(scalar_type alpha) { _alpha = alpha; }
  void setZCoord(scalar_type z_coord) { _z_coord = z_coord; }
  void setZCoordSelection(scalar_type z_coord_selection) { _z_coord_selection = z_coord_selection; }
  void setSelectionColor(color_type selection_color) { _selection_color = selection_color; }

  void setColors(std::array<color_type, 4> colors) { _colors = colors; }

  scalar_type pointSize() const { return _point_size; }
  scalar_type zCoord() const { return _z_coord; }
  scalar_type alpha() const { return _alpha; }
  scalar_type zCoordSelection() const { return _z_coord_selection; }

  void updateLimitsFromData();
  void setLimitsSize(scalar_type min, scalar_type max) {
    _min_val_size = min;
    _max_val_size = max;
  }
  void setLimitsColor(scalar_type min, scalar_type max) {
    _min_val_color = min;
    _max_val_color = max;
  }

 private:
  std::unique_ptr<QGLShaderProgram> _program;
  std::unique_ptr<QGLShader> _vshader;
  std::unique_ptr<QGLShader> _fshader;

  GLuint _coords_attribute;
  GLuint _flags_attribute;
  GLuint _scalar_attribute_size;
  GLuint _scalar_attribute_color;

  GLuint _matrix_uniform;
  GLuint _z_coord_uniform;
  GLuint _alpha_uniform;
  GLuint _min_size_uniform;
  GLuint _max_size_uniform;
  GLuint _min_color_uniform;
  GLuint _max_color_uniform;
  GLuint _pnt_size_uniform;
  GLuint _min_pnt_size_uniform;
  GLuint _color0_uniform;
  GLuint _color1_uniform;
  GLuint _color2_uniform;
  GLuint _color3_uniform;
  GLuint _selection_color_uniform;

  const scalar_type* _embedding;
  const scalar_type* _attribute_size;
  const scalar_type* _attribute_color;
  const flag_type* _flags;

  scalar_type _min_val_size;
  scalar_type _max_val_size;
  scalar_type _min_val_color;
  scalar_type _max_val_color;

  int _num_points;
  bool _initialized;

  color_type _selection_color;
  scalar_type _point_size;
  scalar_type _min_point_size;
  scalar_type _alpha;
  scalar_type _z_coord;
  scalar_type _z_coord_selection;
  std::array<color_type, 4> _colors;
};

}  // namespace viz
}  // namespace hdi

#endif
