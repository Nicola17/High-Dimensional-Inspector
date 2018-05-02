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

#ifndef CONTROLLER_SELECTION_EMBEDDING_H
#define CONTROLLER_SELECTION_EMBEDDING_H

#include <QObject>
#include <QVector2D>
#include "hdi/utils/abstract_log.h"
#include "hdi/data/abstract_panel_data.h"
#include "hdi/data/embedding.h"
#include "hdi/visualization/scatterplot_canvas_qobj.h"
#include "hdi/visualization/abstract_view.h"

namespace hdi{
  namespace viz{

    /*!
      Controller that takes care of doing selection in the data::PanelData based on the interaction with the viz::ScatterplotCanvas
      \author Nicola Pezzotti
    */
    class ControllerSelectionEmbedding: public QObject{
      Q_OBJECT
    public:
      typedef float scalar_type;
      typedef QVector2D point_type;

    public:
      ControllerSelectionEmbedding();
      virtual ~ControllerSelectionEmbedding(){}
      bool isInitialized(){return _initialized;}
      void initialize();
      void reset();

      //! Return the current log
      utils::AbstractLog* logger()const{return _logger;}
      //! Set a pointer to an existing log
      void setLogger(utils::AbstractLog* logger){_logger = logger;}
      //! Set the actors controller by this controller
      void setActors(data::AbstractPanelData* panel_data, data::Embedding<scalar_type>* embedding, ScatterplotCanvas* canvas);
      //! Add a view linked to the panel data
      void addView(AbstractView* view);
      //! Remove a view linked to the panel data
      void removeView(AbstractView* view);


    private slots:
      void onDoSelection(point_type bl, point_type tr);
      void onLeftDoubleClickOnCanvas(point_type bl);
      void onUnselectAll();

    signals:
      void sgnSelection();
      void sgnUnselection();

    private:
      data::AbstractPanelData* _panel_data;
      data::Embedding<scalar_type>* _embedding;
      ScatterplotCanvas* _canvas;
      std::vector<AbstractView*> _linked_views;

      bool _initialized;
      utils::AbstractLog* _logger;
    };

  }
}

#endif
