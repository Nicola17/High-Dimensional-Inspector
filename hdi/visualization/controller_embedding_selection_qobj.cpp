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

#include "hdi/utils/assert_by_exception.h"
#include "hdi/visualization/controller_embedding_selection_qobj.h"
#include "hdi/utils/log_helper_functions.h"
#include "QApplication"

namespace hdi{
  namespace viz{

    ControllerSelectionEmbedding::ControllerSelectionEmbedding():
      _initialized(false),
      _panel_data(nullptr),
      _embedding(nullptr),
      _canvas(nullptr),
      _logger(nullptr)
    {}

    void ControllerSelectionEmbedding::initialize(){
      utils::secureLog(_logger,"Initializing ControllerSelectionEmbedding...");
      checkAndThrowLogic(_panel_data!=nullptr,"ControllerSelectionEmbedding must have a valid panel data!");
      checkAndThrowLogic(_embedding!=nullptr,"ControllerSelectionEmbedding must have a valid embedding!");
      checkAndThrowLogic(_canvas!=nullptr,"ControllerSelectionEmbedding must have a valid canvas!");
      checkAndThrowLogic(!_initialized,"ControllerSelectionEmbedding is already initialized");
      _initialized = true;

      QObject::connect(_canvas,&ScatterplotCanvas::sgnSelection,this,&ControllerSelectionEmbedding::onDoSelection);
      QObject::connect(_canvas,&ScatterplotCanvas::sgnLeftDoubleClick,this,&ControllerSelectionEmbedding::onLeftDoubleClickOnCanvas);

      utils::secureLog(_logger,"done");
    }

    void ControllerSelectionEmbedding::setActors(data::AbstractPanelData* panel_data, data::Embedding<scalar_type>* embedding, ScatterplotCanvas* canvas){
      checkAndThrowLogic(!_initialized,"ControllerSelectionEmbedding is already initialized");
      _panel_data = panel_data;
      _embedding = embedding;
      _canvas = canvas;
    }

    void ControllerSelectionEmbedding::reset(){
      _initialized = false;
      _panel_data = nullptr;
      _embedding = nullptr;
      _canvas = nullptr;
      _linked_views.clear();
    }

    void ControllerSelectionEmbedding::addView(AbstractView* view){
      checkAndThrowLogic(view->panelData()==_panel_data,"The view is not linked to the right panel_data!");
      _linked_views.push_back(view);
    }
    void ControllerSelectionEmbedding::removeView(AbstractView* view){
      std::remove(_linked_views.begin(),_linked_views.end(),view);
    }

    void ControllerSelectionEmbedding::onDoSelection(point_type bl, point_type tr){
      utils::secureLog(_logger,"Selection on a scatterplot canvas...");
      utils::secureLogValue(_logger,"Bottom-Left x",bl.x());
      utils::secureLogValue(_logger,"Bottom-Left y",bl.y());
      utils::secureLogValue(_logger,"Top-Right x",tr.x());
      utils::secureLogValue(_logger,"Top-Right y",tr.x());

      auto key_mod = QApplication::queryKeyboardModifiers();

      bool selection_changed = false;
      auto& flags = _panel_data->getFlagsDataPoints();
      for(int i = 0; i < _panel_data->numDataPoints(); ++i){
        auto x = _embedding->dataAt(i,0);
        auto y = _embedding->dataAt(i,1);
        if(x >= bl.x() && x <= tr.x() && y >= bl.y() && y <= tr.y()){
          if(key_mod == Qt::ShiftModifier){
          flags[i] = flags[i] ^ data::AbstractPanelData::Selected;
          } else if(key_mod == Qt::ControlModifier){
          flags[i] = flags[i] & (!data::AbstractPanelData::Selected);
          } else {
          flags[i] = flags[i] | data::AbstractPanelData::Selected;
          }
          selection_changed = true;
        }
      }
      if(selection_changed){
        for(auto ptr: _linked_views){
          ptr->updateView();
        }
        _canvas->updateGL();
      }
      emit sgnSelection();
    }

    void ControllerSelectionEmbedding::onLeftDoubleClickOnCanvas(point_type bl){
      onUnselectAll();
    }

    void ControllerSelectionEmbedding::onUnselectAll(){
      utils::secureLog(_logger,"Unselect all...");
      auto& flags = _panel_data->getFlagsDataPoints();
      for(auto& v: flags){
        v &= ~data::AbstractPanelData::Selected;
      }
      for(auto ptr: _linked_views){
        ptr->updateView();
      }
      _canvas->updateGL();
      emit sgnUnselection();
    }


  }
}
