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

#ifndef TEXT_VIEW_H
#define TEXT_VIEW_H


#include <qdialog.h>
#include <qcolor.h>
#include <memory>
#include "hdi/data/abstract_data.h"
#include "hdi/visualization/abstract_view.h"

namespace Ui{
  class TextView;
}
namespace hdi{
  namespace viz{

    //! View for data::TextData
    /*!
      View for data::TextData
      \author Nicola Pezzotti
    */
    class TextView : public QWidget, public AbstractView{
      Q_OBJECT
    public:
      TextView(QWidget* parent = nullptr);
      virtual ~TextView(){}

      virtual QWidget* widgetPtr(){ return this; }
      virtual const QWidget* widgetPtr()const{ return this; }

      virtual void updateView();

    public slots:
      //! Communicate to the widget that the data changed
      void onSelectionChanged();

    private slots:
      //! Do a selection in the view
      void onSelectTexts();
      //! Unselect data in the view
      void onUnselectTexts();
      //! Export selected texts
      void onExportTexts()const;
      //! Import a selection
      void onImportTextSelection();

    signals:
      void sgnSelectionChanged();

    private:
      Ui::TextView* _ui;
    };
  }
}

#endif
