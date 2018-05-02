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

#include "hdi/visualization/anatomical_planes_view_qobj.h"
#include "hdi/data/voxel_data.h"
#include <array>
#include <assert.h>

namespace hdi{
  namespace viz{

    AnatomicalPlanesView::AnatomicalPlanesView(QWidget* parent):_single_image_mode(false){
      _ui.setupUi(this);

      connect(_ui._x_Sld,&QSlider::valueChanged,this,&AnatomicalPlanesView::onPlaneMoved);
      connect(_ui._y_Sld,&QSlider::valueChanged,this,&AnatomicalPlanesView::onPlaneMoved);
      connect(_ui._z_Sld,&QSlider::valueChanged,this,&AnatomicalPlanesView::onPlaneMoved);

      connect(_ui._x_SpBx,static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),this,&AnatomicalPlanesView::onPlaneMoved);
      connect(_ui._y_SpBx,static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),this,&AnatomicalPlanesView::onPlaneMoved);
      connect(_ui._z_SpBx,static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),this,&AnatomicalPlanesView::onPlaneMoved);

      connect(_ui._res_mult_dspbx,&QDoubleSpinBox::editingFinished,this,&AnatomicalPlanesView::onSelectionChanged);
    }

    void AnatomicalPlanesView::onSelectionChanged(){
      updateView();
    }

    void AnatomicalPlanesView::computeMaxVoxelValues(int& x, int& y, int& z)const{
      x = 0;
      y = 0;
      z = 0;

      const auto& data = _panel_data->getDataPoints();
      for(int i = 0; i < data.size(); ++i){
        auto data_ptr = dynamic_cast<data::VoxelData*>(data[i].get());
        if(data_ptr == nullptr){
          continue;
        }
        x = std::max(data_ptr->_x,x);
        y = std::max(data_ptr->_y,y);
        z = std::max(data_ptr->_z,z);
      }

    }

    //! Update limits in the sliders and spin boxes
    void AnatomicalPlanesView::updateLimits(){
      int max_x,max_y,max_z;
      computeMaxVoxelValues(max_x,max_y,max_z);

      _ui._x_Sld->setMaximum(max_x);
      _ui._y_Sld->setMaximum(max_y);
      _ui._z_Sld->setMaximum(max_z);

      _ui._x_SpBx->setMaximum(max_x);
      _ui._y_SpBx->setMaximum(max_y);
      _ui._z_SpBx->setMaximum(max_z);
    }

    void AnatomicalPlanesView::updateView(){
      assert(_panel_data!=nullptr);

      int max_x,max_y,max_z;
      computeMaxVoxelValues(max_x,max_y,max_z);

      _xy = QImage(max_x+1,max_y+1,QImage::Format_ARGB32);
      _xz = QImage(max_x+1,max_z+1,QImage::Format_ARGB32);
      _yz = QImage(max_y+1,max_z+1,QImage::Format_ARGB32);

      for(int j = 0; j < _xy.height(); ++j){
        for(int i = 0; i < _xy.width(); ++i){
          _xy.setPixel(i, j, qRgb(0, 0, 0));
        }
      }
      for(int j = 0; j < _xz.height(); ++j){
        for(int i = 0; i < _xz.width(); ++i){
          _xz.setPixel(i, j, qRgb(0, 0, 0));
        }
      }
      for(int j = 0; j < _yz.height(); ++j){
        for(int i = 0; i < _yz.width(); ++i){
          _yz.setPixel(i, j, qRgb(0, 0, 0));
        }
      }

      QColor selection_color(qRgb(255,150,0));
      const int x_sel = _ui._x_SpBx->value();
      const int y_sel = _ui._y_SpBx->value();
      const int z_sel = _ui._z_SpBx->value();

      const auto& data = _panel_data->getDataPoints();
      const auto& flags = _panel_data->getFlagsDataPoints();

      for(int i = 0; i < data.size(); ++i){
        auto data_ptr = dynamic_cast<data::VoxelData*>(data[i].get());
        if(data_ptr == nullptr){
          continue;
        }
        if((flags[i] & panel_data_type::Selected) == panel_data_type::Selected){
          if(data_ptr->_x == x_sel){
            _yz.setPixel(data_ptr->_y,data_ptr->_z,selection_color.rgb());
          }
          if(data_ptr->_y == y_sel){
            _xz.setPixel(data_ptr->_x,data_ptr->_z,selection_color.rgb());
          }
          if(data_ptr->_z == z_sel){
            _xy.setPixel(data_ptr->_x,data_ptr->_y,selection_color.rgb());
          }
        }
      }

      double res_mult = _ui._res_mult_dspbx->value();
      _ui._axial_image_lbl->setPixmap(QPixmap::fromImage(_xy.scaledToWidth(_xy.width()*res_mult)));
      _ui._coronal_image_lbl->setPixmap(QPixmap::fromImage(_xz.scaledToWidth(_xz.width()*res_mult)));
      _ui._sagittal_image_lbl->setPixmap(QPixmap::fromImage(_yz.scaledToWidth(_yz.width()*res_mult)));

    }


    void AnatomicalPlanesView::updateViewWithSelection(){
      assert(_panel_data!=nullptr);

      int max_x,max_y,max_z;
      computeMaxVoxelValues(max_x,max_y,max_z);

      _xy = QImage(max_x+1,max_y+1,QImage::Format_ARGB32);
      _xz = QImage(max_x+1,max_z+1,QImage::Format_ARGB32);
      _yz = QImage(max_y+1,max_z+1,QImage::Format_ARGB32);

      for(int j = 0; j < _xy.height(); ++j){
        for(int i = 0; i < _xy.width(); ++i){
          _xy.setPixel(i, j, qRgb(0, 0, 0));
        }
      }
      for(int j = 0; j < _xz.height(); ++j){
        for(int i = 0; i < _xz.width(); ++i){
          _xz.setPixel(i, j, qRgb(0, 0, 0));
        }
      }
      for(int j = 0; j < _yz.height(); ++j){
        for(int i = 0; i < _yz.width(); ++i){
          _yz.setPixel(i, j, qRgb(0, 0, 0));
        }
      }

      const int x_sel = _ui._x_SpBx->value();
      const int y_sel = _ui._y_SpBx->value();
      const int z_sel = _ui._z_SpBx->value();

      const auto& data = _panel_data->getDataPoints();
      const auto& flags = _panel_data->getFlagsDataPoints();

      for(int i = 0; i < data.size(); ++i){
        auto data_ptr = dynamic_cast<data::VoxelData*>(data[i].get());
        if(data_ptr == nullptr){
          continue;
        }
          scalar_type intensity = 255*_selection[i];
          if(data_ptr->_x == x_sel){
            _yz.setPixel(data_ptr->_y,data_ptr->_z,qRgb(intensity,intensity,intensity));
          }
          if(data_ptr->_y == y_sel){
            _xz.setPixel(data_ptr->_x,data_ptr->_z,qRgb(intensity,intensity,intensity));
          }
          if(data_ptr->_z == z_sel){
            _xy.setPixel(data_ptr->_x,data_ptr->_y,qRgb(intensity,intensity,intensity));
          }
      }

      double res_mult = _ui._res_mult_dspbx->value();
      _ui._axial_image_lbl->setPixmap(QPixmap::fromImage(_xy.scaledToWidth(_xy.width()*res_mult)));
      _ui._coronal_image_lbl->setPixmap(QPixmap::fromImage(_xz.scaledToWidth(_xz.width()*res_mult)));
      _ui._sagittal_image_lbl->setPixmap(QPixmap::fromImage(_yz.scaledToWidth(_yz.width()*res_mult)));

    }

    void AnatomicalPlanesView::setSingleImageMode(bool v){
      _ui._coronal_image_lbl->setVisible(!v);
      _ui._sagittal_image_lbl->setVisible(!v);

      _ui._x_SpBx->setVisible(!v);
      _ui._y_SpBx->setVisible(!v);
      _ui._z_SpBx->setVisible(!v);

      _ui._x_Sld->setVisible(!v);
      _ui._y_Sld->setVisible(!v);
      _ui._z_Sld->setVisible(!v);

      _ui._x_lbl->setVisible(!v);
      _ui._y_lbl->setVisible(!v);
      _ui._z_lbl->setVisible(!v);
    }

  }
}
