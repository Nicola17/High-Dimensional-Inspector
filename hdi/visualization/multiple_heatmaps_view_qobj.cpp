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

#include "hdi/visualization/multiple_heatmaps_view_qobj.h"
#include "hdi/data/image_data.h"
#include <array>
#include <assert.h>
#include <QSpinBox>
#include <cmath>
#include "hdi/utils/assert_by_exception.h"

namespace hdi{
  namespace viz{

    MultipleHeatmapsView::MultipleHeatmapsView(QWidget* parent):
      _num_columns(5),
      _img_height(50),
      _num_dimensions(0),
      _data(nullptr)
    {
      _ui.setupUi(this);
      QObject::connect(_ui._num_columns_sbox, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &MultipleHeatmapsView::onNumOfColumnsChanged);
      QObject::connect(_ui._img_height_sbox, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &MultipleHeatmapsView::onImageHeightChanged);

    }

    void MultipleHeatmapsView::onSelectionChanged(){
      updateView();
    }

    void MultipleHeatmapsView::updateView(){
      checkAndThrowLogic(_panel_data!=nullptr,"An AbstractPanelData shuold be already assigned");
      checkAndThrowLogic(_data!=nullptr,"A pointer to data shuold be already assigned");
      checkAndThrowLogic(_num_dimensions!=0,"Invalid dimensionality");
      for(int i = 0; i < _image_labels.size(); ++i){
        _ui._image_grid->removeWidget(_image_labels[i].get());
      }
      _image_labels.clear();
      const auto& data = _panel_data->getDataPoints();
      const auto& flags = _panel_data->getFlagsDataPoints();

      unsigned int pic_size = std::sqrt(_num_dimensions)+1;
      unsigned int num_selected(0);
      unsigned int num_valid(0);
      for(int i = 0; i < data.size(); ++i){
        ++num_valid;
        if((flags[i] & panel_data_type::Selected) == panel_data_type::Selected){
          QImage heatmap(pic_size,pic_size,QImage::Format_RGB32);
          for(int d = 0; d < pic_size*pic_size; ++d){
            heatmap.setPixel(d%pic_size,d/pic_size,qRgb(0,0,50));
          }
          for(int d = 0; d < _num_dimensions; ++d){
            auto v =_data[i*_num_dimensions+d];
            v = (v-_data_min)/(_data_min-_data_max)*255.;
            heatmap.setPixel(d%pic_size,d/pic_size,qRgb(v,v,v));
          }

          std::shared_ptr<QLabel> image_label(new QLabel());
          image_label->setPixmap(QPixmap::fromImage(heatmap.scaledToHeight(_img_height)));
          _image_labels.push_back(image_label);
          ++num_selected;
        }
      }

      for(int i = 0; i < _image_labels.size(); ++i){
        _ui._image_grid->addWidget(_image_labels[i].get(),i/_num_columns,i%_num_columns);
      }

      _ui._num_elem_lbl->setText(QString("# Elements: %1").arg(num_valid));
      _ui._num_sel_elem_lbl->setText(QString("# Selected elements: %1").arg(num_selected));
    }

    void MultipleHeatmapsView::onNumOfColumnsChanged(int n){
      _num_columns = n;
      updateView();
    }
    void MultipleHeatmapsView::onImageHeightChanged(int n){
      _img_height = n;
      updateView();
    }

    void MultipleHeatmapsView::setAuxData(unsigned int num_dimensions, const scalar_type* data){
      checkAndThrowLogic(_panel_data!=nullptr,"An AbstractPanelData shuold be already assigned");
      _num_dimensions = num_dimensions;
      _data = data;

      _data_min = std::numeric_limits<scalar_type>::max();
      _data_max = -std::numeric_limits<scalar_type>::max();
      for(int i = 0; i < _num_dimensions*_panel_data->numDataPoints(); ++i){
        _data_min = std::min(_data_min,_data[i]);
        _data_max = std::max(_data_max,_data[i]);
      }
    }

  }
}
