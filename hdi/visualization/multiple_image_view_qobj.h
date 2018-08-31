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

#ifndef MULTIPLE_IMAGE_VIEW_H
#define MULTIPLE_IMAGE_VIEW_H

#include "ui_multiple_image_view_qobj.h"

#include <qcolor.h>
#include <qdialog.h>
#include <QLabel>
#include <memory>
#include "hdi/data/abstract_data.h"
#include "hdi/visualization/abstract_view.h"

namespace hdi {
namespace viz {

//! View for data::ImageData
/*!
      View for data::ImageData
      \author Nicola Pezzotti
    */
class MultipleImageView : public QWidget, public AbstractView {
  Q_OBJECT
 public:
  MultipleImageView(QWidget* parent = nullptr);
  virtual ~MultipleImageView() {}

  virtual QWidget* widgetPtr() { return this; }
  virtual const QWidget* widgetPtr() const { return this; }

  virtual void updateView();

  void setDetailsVisible(bool);

 public slots:
  virtual void onSelectionChanged();

 private slots:
  void onNumOfColumnsChanged(int n);
  void onImageHeightChanged(int n);

 private:
  Ui::MultipleImageView _ui;
  std::vector<std::shared_ptr<QLabel>> _image_labels;

  unsigned int _num_columns;
  unsigned int _img_height;

 public:
  bool _text_data_as_os_path;
};

}  // namespace viz
}  // namespace hdi

#endif
