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

#include "hdi/visualization/multiple_image_view_qobj.h"
#include "hdi/data/image_data.h"
#include "hdi/data/text_data.h"
#include <array>
#include <assert.h>
#include <QSpinBox>

namespace hdi{
  namespace viz{

    MultipleImageView::MultipleImageView(QWidget* parent):
      _num_columns(5),
      _img_height(50),
      _text_data_as_os_path(false)
    {
      _ui.setupUi(this);
      QObject::connect(_ui._num_columns_sbox, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &MultipleImageView::onNumOfColumnsChanged);
      QObject::connect(_ui._img_height_sbox, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &MultipleImageView::onImageHeightChanged);

    }

    void MultipleImageView::onSelectionChanged(){
      updateView();
    }

    void MultipleImageView::updateView(){
      if(_ui._disabled_cbx->isChecked()){
        return;
      }
      assert(_panel_data!=nullptr);
      for(int i = 0; i < _image_labels.size(); ++i){
        _ui._image_grid->removeWidget(_image_labels[i].get());
      }
      _image_labels.clear();
      const auto& data = _panel_data->getDataPoints();
      const auto& flags = _panel_data->getFlagsDataPoints();

      int num_selected(0);
      int num_valid(0);
      for(int i = 0; i < data.size(); ++i){
        auto data_ptr = dynamic_cast<data::ImageData*>(data[i].get());
        if(data_ptr != nullptr){
          ++num_valid;
          if((flags[i] & panel_data_type::Selected) == panel_data_type::Selected){
            auto scaled_img = data_ptr->image().scaledToHeight(_img_height);
            std::shared_ptr<QLabel> image_label(new QLabel());
            image_label->setPixmap(QPixmap::fromImage(scaled_img));
            _image_labels.push_back(image_label);
            ++num_selected;
          }
        }else{
          if(_text_data_as_os_path){
            auto text_data_ptr = dynamic_cast<data::TextData*>(data[i].get());
            if(text_data_ptr != nullptr){
              ++num_valid;
              if((flags[i] & panel_data_type::Selected) == panel_data_type::Selected){
                QImage scaled_img(QString::fromStdString(text_data_ptr->text()));
                scaled_img = scaled_img.scaledToHeight(_img_height);
                std::shared_ptr<QLabel> image_label(new QLabel());
                image_label->setPixmap(QPixmap::fromImage(scaled_img));
                _image_labels.push_back(image_label);
                ++num_selected;
              }
            }
          }
        }

      }

      for(int i = 0; i < _image_labels.size(); ++i){
        _ui._image_grid->addWidget(_image_labels[i].get(),i/_num_columns,i%_num_columns);
      }

      _ui._num_elem_lbl->setText(QString("# Elements: %1").arg(num_valid));
      _ui._num_sel_elem_lbl->setText(QString("# Selected elements: %1").arg(num_selected));
    }

    void MultipleImageView::onNumOfColumnsChanged(int n){
      _num_columns = n;
      updateView();
    }
    void MultipleImageView::onImageHeightChanged(int n){
      _img_height = n;
      updateView();
    }

    void MultipleImageView::setDetailsVisible(bool v){
      _ui._num_sel_elem_lbl->setVisible(v);
      _ui._num_elem_lbl->setVisible(v);

      _ui._height_lbl->setVisible(v);
      _ui._column_lbl->setVisible(v);
      _ui._num_columns_sbox->setVisible(v);
      _ui._img_height_sbox->setVisible(v);
      _ui._disabled_cbx->setVisible(v);
    }



  }
}
