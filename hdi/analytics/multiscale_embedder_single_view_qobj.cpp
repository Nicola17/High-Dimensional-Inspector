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

#include "hdi/analytics/multiscale_embedder_single_view_qobj.h"
#include "hdi/utils/log_helper_functions.h"

namespace hdi{
  namespace analytics{

    MultiscaleEmbedderSingleView::MultiscaleEmbedderSingleView():
      _logger(nullptr),
      _initialized(false),
      _viewer(new hdi::viz::ScatterplotCanvas),
      _selection_controller(new hdi::viz::ControllerSelectionEmbedding),
      _visualization_mode(VisualizationModes::UserDefined)
    {
      connect(_viewer.get(),&viz::ScatterplotCanvas::sgnKeyPressed,this,&MultiscaleEmbedderSingleView::onKeyPressedOnCanvas);
      connect(_selection_controller.get(),&viz::ControllerSelectionEmbedding::sgnSelection,this,&MultiscaleEmbedderSingleView::onSelection);
      _viewer->resize(800,800);
    }

    void MultiscaleEmbedderSingleView::initialize(sparse_scalar_matrix_type& sparse_matrix, id_type my_id, tsne_type::Parameters params){
      checkAndThrowLogic(!_initialized, "Aggregator must be initialized ");
      checkAndThrowLogic(sparse_matrix.size() == _panel_data.numDataPoints(), "Panel data and sparse matrix size must agree");
      utils::secureLogValue(_logger,"Sparse matrix size",sparse_matrix.size());
      _selection_controller->setLogger(_logger);
      _tSNE.setLogger(_logger);

      for(int i = 0; i < sparse_matrix.size(); ++i){ //QUICK FIX!!!!
        if(sparse_matrix[i].size() < 7){
          //utils::secureLogValue(_logger,"UUUUUUUUUUUUUUUU",sparse_matrix[i].size());
          int to_add = 7 - sparse_matrix[i].size();
          for(int v = 0; v < to_add; ++v){
            int id = rand()%sparse_matrix.size();
            sparse_matrix[i][id] = 1./to_add;
          }

        }
      }
      double theta = 0;
      if(sparse_matrix.size() < 1000){
        theta = 0;
        params._exaggeration_factor = 1.5;
      }else if(sparse_matrix.size() < 15000){
        theta = (sparse_matrix.size()-1000.)/(15000.-1000.)*0.5;
        params._exaggeration_factor = 1.5+(sparse_matrix.size()-1000.)/(15000.-1000.)*8.5;
      }else{
        theta = 0.5;
        params._exaggeration_factor = 10;
      }
      params._remove_exaggeration_iter = 170;

      _tSNE.setTheta(theta);
      utils::secureLogValue(_logger,"theta",theta);
      utils::secureLogValue(_logger,"exg",params._exaggeration_factor);


      _tSNE.initialize(sparse_matrix,&_embedding,params);

      utils::secureLogValue(_logger,"tSNE theta",theta);

      _selection_controller->setActors(&_panel_data,&_embedding,_viewer.get());
      _selection_controller->initialize();

      //_viewer->setBackgroundColors(qRgb(70,70,70),qRgb(30,30,30));
      _viewer->setBackgroundColors(qRgb(255,255,255),qRgb(255,255,255));
      _viewer->setSelectionColor(qRgb(200,200,200));
      _viewer->show();

      _my_id = my_id;
    }

    void MultiscaleEmbedderSingleView::doAnIteration(){
      if(_tSNE.iteration() < 1500){//QUICKPAPER
        _tSNE.doAnIteration();
        {//limits
          std::vector<scalar_type> limits;
          _embedding.computeEmbeddingBBox(limits,0.25);
          auto tr = QVector2D(limits[1],limits[3]);
          auto bl = QVector2D(limits[0],limits[2]);
          _viewer->setTopRightCoordinates(tr);
          _viewer->setBottomLeftCoordinates(bl);
        }
        _viewer->updateGL();
      }
    }

    void MultiscaleEmbedderSingleView::addView(std::shared_ptr<hdi::viz::AbstractView> view){
      _views.push_back(view);
      view->setPanelData(&_panel_data);
      _selection_controller->addView(view.get());
    }
    void MultiscaleEmbedderSingleView::addUserDefinedDrawer(std::shared_ptr<hdi::viz::AbstractScatterplotDrawer> drawer){
      drawer->initialize(_viewer->context());
      _drawers.push_back(drawer);
      onUpdateViewer();
    }
    void MultiscaleEmbedderSingleView::addAreaOfInfluenceDrawer(std::shared_ptr<hdi::viz::AbstractScatterplotDrawer> drawer){
      drawer->initialize(_viewer->context());
      _influence_drawers.push_back(drawer);
      onUpdateViewer();
    }
    void MultiscaleEmbedderSingleView::addSelectionDrawer(std::shared_ptr<hdi::viz::AbstractScatterplotDrawer> drawer){
      drawer->initialize(_viewer->context());
      _selection_drawers.push_back(drawer);
      onUpdateViewer();
    }

    void MultiscaleEmbedderSingleView::onKeyPressedOnCanvas(int key){
      utils::secureLogValue(_logger,"Key pressed on canvas",key);
      if(key == Qt::Key_N){
        emit sgnNewAnalysisTriggered(_my_id);
      }
      if(key == Qt::Key_S){
        emit sgnActivateSelectionMode(_my_id);
      }
      if(key == Qt::Key_U){
        emit sgnActivateUserDefinedMode(_my_id);
      }
      if(key == Qt::Key_I){
        emit sgnActivateInfluenceMode(_my_id);
      }
      if(key == Qt::Key_P){
        emit sgnPropagateSelection(_my_id);
      }
      if(key == Qt::Key_C){
        emit sgnClusterizeSelection(_my_id);
      }
      if(key == Qt::Key_E){
        emit sgnExport(_my_id);
      }
    }

    void MultiscaleEmbedderSingleView::onActivateUserDefinedMode(){
      _visualization_mode = VisualizationModes::UserDefined;
      onUpdateViewer();
    }

    void MultiscaleEmbedderSingleView::onActivateSelectionMode(){
      _visualization_mode = VisualizationModes::Selection;
      onUpdateViewer();
    }

    void MultiscaleEmbedderSingleView::onActivateInfluencedMode(){
      _visualization_mode = VisualizationModes::Influence;
      onUpdateViewer();
    }

    void MultiscaleEmbedderSingleView::onUpdateViewer(){
      _viewer->removeAllDrawers();
      switch (_visualization_mode) {
      case VisualizationModes::UserDefined:
        for(auto& d: _drawers){
          _viewer->addDrawer(d.get());
        }
        break;
      case VisualizationModes::Influence:
        for(auto& d: _influence_drawers){
          _viewer->addDrawer(d.get());
        }
        break;
      case VisualizationModes::Selection:
        for(auto& d: _selection_drawers){
          _viewer->addDrawer(d.get());
        }
        break;
      default:
        break;
      }
      _viewer->updateGL();
    }

    void MultiscaleEmbedderSingleView::saveImageToFile(std::string prefix){
      std::string filename(QString("%1_S%2_A%3.png").arg(prefix.c_str()).arg(std::get<0>(_my_id)).arg(std::get<1>(_my_id)).toStdString());
      _viewer->saveToFile(filename);
    }

  }
}
