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

#include "hdi/visualization/pixel_view_qobj.h"
#include "hdi/data/pixel_data.h"
#include <array>
#include <assert.h>

namespace hdi{
  namespace viz{

    PixelView::PixelView(QWidget* parent) :_res_mult(1){
      _ui.setupUi(this);
    }

    void PixelView::setImageSize(int w, int h){
      _image = QImage(w, h, QImage::Format::Format_RGB32);
    }

    void PixelView::onSelectionChanged(){
      updateView();
    }

    void PixelView::updateView(){
      assert(_panel_data!=nullptr);
      for(int j = 0; j < _image.height(); ++j){
        for(int i = 0; i < _image.width(); ++i){
          _image.setPixel(i, j, qRgb(90, 90, 90));
        }
      }

      const auto& data = _panel_data->getDataPoints();
      const auto& flags = _panel_data->getFlagsDataPoints();

      int num_selected(0);
      int num_valid(0);
      for(int i = 0; i < data.size(); ++i){
        auto data_ptr = dynamic_cast<data::PixelData*>(data[i].get());
        if(data_ptr == nullptr){
          continue;
        }
        if(data_ptr->_height != _image.height() || data_ptr->_width != _image.width()){
          continue;
        }
        ++num_valid;

        if((flags[i] & panel_data_type::Selected) == panel_data_type::Selected){
          _image.setPixel(data_ptr->_u,data_ptr->_v,qRgb(255,150,0));
          ++num_selected;
        }
      }

      QPixmap pixmap = QPixmap::fromImage(_image.scaledToWidth(_image.width()*_res_mult));
      _ui._image_lbl->setPixmap(pixmap);
      _ui._num_elem_lbl->setText(QString("# Elements: %1").arg(num_valid));
      _ui._num_sel_elem_lbl->setText(QString("# Selected elements: %1").arg(num_selected));
    }

  }
}
