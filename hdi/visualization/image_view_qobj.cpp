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

#include "hdi/visualization/image_view_qobj.h"
#include "hdi/data/image_data.h"
#include <array>
#include <assert.h>

namespace hdi{
  namespace viz{

    ImageView::ImageView(QWidget* parent) :_res_mult(1){
      _ui.setupUi(this);
    }

    void ImageView::setImageSize(int w, int h){
      _image = QImage(w, h, QImage::Format::Format_RGB32);
    }

    void ImageView::onSelectionChanged(){
      updateView();
    }

    void ImageView::updateView(){
      assert(_panel_data!=nullptr);
      for(int j = 0; j < _image.height(); ++j){
        for(int i = 0; i < _image.width(); ++i){
          _image.setPixel(i, j, qRgb(90, 90, 90));
        }
      }

      const auto& data = _panel_data->getDataPoints();
      const auto& flags = _panel_data->getFlagsDataPoints();

      std::vector<std::array<double, 3>> _fp_img(_image.width()*_image.height());
      for(auto& v : _fp_img){
        v[0] = 0;
        v[1] = 0;
        v[2] = 0;
      }


      int num_selected(0);
      int num_valid(0);
      for(int i = 0; i < data.size(); ++i){
        auto data_ptr = dynamic_cast<data::ImageData*>(data[i].get());
        if(data_ptr == nullptr){
          continue;
        }
        if(data_ptr->image().height() != _image.height() || data_ptr->image().width() != _image.width()){
          continue;
        }
        ++num_valid;

        if((flags[i] & panel_data_type::Selected) == panel_data_type::Selected){
          auto& img = data_ptr->image();
          for(int j = 0; j < _image.height(); ++j){
            for(int i = 0; i < _image.width(); ++i){
              auto pixel = img.pixel(i, j);
              _fp_img[j*img.width() + i][0] += qRed(pixel);
              _fp_img[j*img.width() + i][1] += qGreen(pixel);
              _fp_img[j*img.width() + i][2] += qBlue(pixel);
            }
          }
          ++num_selected;
        }
      }

      for(int j = 0; j < _image.height(); ++j){
        for(int i = 0; i < _image.width(); ++i){
          _image.setPixel(i, j, qRgb(_fp_img[j*_image.width() + i][0] / num_selected, _fp_img[j*_image.width() + i][1] / num_selected, _fp_img[j*_image.width() + i][2] / num_selected));
        }
      }

      QPixmap pixmap = QPixmap::fromImage(_image.scaledToWidth(_image.width()*_res_mult));
      _ui._image_lbl->setPixmap(pixmap);
      _ui._num_elem_lbl->setText(QString("# Elements: %1").arg(num_valid));
      _ui._num_sel_elem_lbl->setText(QString("# Selected elements: %1").arg(num_selected));
    }

  }
}
