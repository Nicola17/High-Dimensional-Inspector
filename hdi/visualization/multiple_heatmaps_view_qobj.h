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

#ifndef MULTIPLE_HEATMAPS_VIEW_H
#define MULTIPLE_HEATMAPS_VIEW_H

#include "ui_multiple_heatmaps_view_qobj.h"

#include <qdialog.h>
#include <qcolor.h>
#include <memory>
#include "hdi/data/abstract_data.h"
#include "hdi/visualization/abstract_view.h"
#include <QLabel>



namespace hdi{
  namespace viz{

    //! Heatmap viewer for any type of data::AbstractData
    /*!
      View for data::ImageData
      \author Nicola Pezzotti
    */
    class MultipleHeatmapsView : public QWidget, public AbstractView{
      Q_OBJECT
      typedef float scalar_type;
    public:
      MultipleHeatmapsView(QWidget* parent = nullptr);
      virtual ~MultipleHeatmapsView(){}

      virtual QWidget* widgetPtr(){ return this; }
      virtual const QWidget* widgetPtr()const{ return this; }

      virtual void updateView();

      //! An AbstractPanelData shuold be already assigned
      void setAuxData(unsigned int num_dimensions, const scalar_type* data);

    public slots:
      virtual void onSelectionChanged();

    private slots:
      void onNumOfColumnsChanged(int n);
      void onImageHeightChanged(int n);


    private:
      Ui::MultipleHeatmapsView _ui;
      std::vector<std::shared_ptr<QLabel>> _image_labels;

      unsigned int _num_columns;
      unsigned int _img_height;

      unsigned int _num_dimensions;
      const scalar_type* _data;
      scalar_type _data_min,_data_max;

    };

  }
}

#endif
