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

#include "hdi/analytics/waow_vis_single_view_embedder_qobj.h"
#include "hdi/utils/log_helper_functions.h"
#include "hdi/data/text_data.h"
#include <iostream>
#include "hdi/visualization/embedding_lines_drawer.h"
#include "hdi/visualization/embedding_bundled_lines_drawer.h"
#include "hdi/visualization/scatterplot_drawer_two_scalar_attributes.h"
#include <chrono>
#include <thread>

namespace hdi{
  namespace analytics{

    WAOWVisSingleViewEmbedder::WAOWVisSingleViewEmbedder():
      _logger(nullptr),
      _initialized(false),
      _canvas(new hdi::viz::ScatterplotCanvas),
      _selection_controller_A_1D(new hdi::viz::ControllerSelectionEmbedding),
      _selection_controller_A_2D(new hdi::viz::ControllerSelectionEmbedding),
      _selection_controller_B_1D(new hdi::viz::ControllerSelectionEmbedding),
      _selection_controller_B_2D(new hdi::viz::ControllerSelectionEmbedding),
      _visualization_mode("Weights"),
      _lines_alpha(0.05),
      _emph_selection(1)
    {
      connect(_canvas.get(),&viz::ScatterplotCanvas::sgnKeyPressed,this,&WAOWVisSingleViewEmbedder::onKeyPressedOnCanvas);
      //Only one is needed as the controllers works independently from this class
      connect(_selection_controller_A_1D.get(),&viz::ControllerSelectionEmbedding::sgnSelection,this,&WAOWVisSingleViewEmbedder::onSelection);
      connect(_selection_controller_B_1D.get(),&viz::ControllerSelectionEmbedding::sgnSelection,this,&WAOWVisSingleViewEmbedder::onSelection);
      _canvas->resize(1500,400);
    }

    void WAOWVisSingleViewEmbedder::initialize(const sparse_scalar_matrix_type& fmc_A,
                       const sparse_scalar_matrix_type& fmc_B,
                       id_type my_id)
    {

      checkAndThrowLogic(!_initialized, "WAOWVisSingleViewEmbedder is already initialized");
      checkAndThrowLogic(fmc_A.size() == _panel_data_A.numDataPoints(), "fmc_A.size() == _panel_data_A.numDataPoints()");
      checkAndThrowLogic(fmc_B.size() == _panel_data_B.numDataPoints(), "fmc_B.size() == _panel_data_B.numDataPoints()");


      utils::secureLogValue(_logger,"A Size",fmc_A.size());
      utils::secureLogValue(_logger,"B Size",fmc_B.size());

      //Initializing A
      utils::secureLog(_logger,"Initializing embedder for sets A");
      {
        tsne_type::Parameters params;

        double theta = 0;
        if(fmc_A.size() < 1000){
          theta = 0;
          params._exaggeration_factor = 1.5;
        }else if(fmc_A.size() < 15000){
          theta = (fmc_A.size()-1000.)/(15000.-1000.)*0.5;
          params._exaggeration_factor = 1.5+(fmc_A.size()-1000.)/(15000.-1000.)*8.5;
        }else{
          theta = 0.5;
          params._exaggeration_factor = 10;
        }
        params._remove_exaggeration_iter = 170;
        utils::secureLog(_logger,"\ttheta",theta);
        utils::secureLog(_logger,"\texaggeration",params._exaggeration_factor);

        _tSNE_A_1D.setTheta(theta);
        _tSNE_A_2D.setTheta(theta);

        params._embedding_dimensionality = 2;
        _tSNE_A_2D.initialize(fmc_A,&_embedding_A_2D,params);
        params._embedding_dimensionality = 1;
        _tSNE_A_1D.initialize(fmc_A,&_embedding_A_1D,params);

      }

      //Initializing B
      utils::secureLog(_logger,"Initializing embedder for sets B");
      {
        tsne_type::Parameters params;
        params._eta = 50;

        double theta = 0;
        if(fmc_B.size() < 1000){
          theta = 0;
          params._exaggeration_factor = 1.1;
        }else if(fmc_B.size() < 15000){
          theta = (fmc_B.size()-1000.)/(15000.-1000.)*0.5;
          params._exaggeration_factor = 1.5+(fmc_B.size()-1000.)/(15000.-1000.)*8.5;
        }else{
          theta = 0.5;
          params._exaggeration_factor = 10;
        }
        params._remove_exaggeration_iter = 170;
        utils::secureLog(_logger,"\ttheta",theta);
        utils::secureLog(_logger,"\texaggeration",params._exaggeration_factor);

        _tSNE_B_1D.setTheta(theta);
        _tSNE_B_2D.setTheta(theta);

        params._embedding_dimensionality = 2;
        _tSNE_B_2D.initialize(fmc_B,&_embedding_B_2D,params);
        params._embedding_dimensionality = 1;
        _tSNE_B_1D.initialize(fmc_B,&_embedding_B_1D,params);
      }

      _embedding_A_2D_view.resize(2,_embedding_A_2D.numDataPoints());
      _embedding_A_1D_view.resize(2,_embedding_A_2D.numDataPoints());
      _embedding_B_2D_view.resize(2,_embedding_B_2D.numDataPoints());
      _embedding_B_1D_view.resize(2,_embedding_B_2D.numDataPoints());

      _selection_controller_A_2D->setActors(&_panel_data_A,&_embedding_A_2D_view,_canvas.get());
      _selection_controller_A_1D->setActors(&_panel_data_A,&_embedding_A_1D_view,_canvas.get());
      _selection_controller_B_2D->setActors(&_panel_data_B,&_embedding_B_2D_view,_canvas.get());
      _selection_controller_B_1D->setActors(&_panel_data_B,&_embedding_B_1D_view,_canvas.get());

      _selection_controller_A_2D->initialize();
      _selection_controller_A_1D->initialize();
      _selection_controller_B_2D->initialize();
      _selection_controller_B_1D->initialize();

      {//limits
        std::vector<scalar_type> limits;
        auto tr = QVector2D(3,1);
        auto bl = QVector2D(0,0);
        _canvas->setTopRightCoordinates(tr);
        _canvas->setBottomLeftCoordinates(bl);
      }
      _canvas->setBackgroundColors(qRgb(255,255,255),qRgb(255,255,255));
      _canvas->setSelectionColor(qRgb(200,200,200));
      _canvas->show();

      _limits_A_2D = std::vector<scalar_type>{0.1,0.9,0.1,0.9};
      _limits_A_1D = std::vector<scalar_type>{1,1.2,0.1,0.9};
      _limits_B_2D = std::vector<scalar_type>{2.1,2.9,0.1,0.9};
      _limits_B_1D = std::vector<scalar_type>{1.7,1.75,0.1,0.9};
      _limits_B_Text = std::vector<scalar_type>{1.65,2.0,0.08,0.92};

      {// initializing the equalizers
        _eq_A_1D_to_2D.initialize(&_embedding_A_1D,&_embedding_A_2D);
        _eq_A_2D_to_1D.initialize(&_embedding_A_2D,&_embedding_A_1D);

        _eq_B_1D_to_2D.initialize(&_embedding_B_1D,&_embedding_B_2D);
        _eq_B_2D_to_1D.initialize(&_embedding_B_2D,&_embedding_B_1D);

        _eq_A_1D_to_B_1D.initialize(&_embedding_A_1D,&_embedding_B_1D, &_connections_BA);
        _eq_A_1D_to_B_2D.initialize(&_embedding_A_1D,&_embedding_B_2D, &_connections_BA);
        _eq_B_1D_to_A_1D.initialize(&_embedding_B_1D,&_embedding_A_1D, &_connections_AB);
        _eq_B_1D_to_A_2D.initialize(&_embedding_B_1D,&_embedding_A_2D, &_connections_AB);
      }

      hdi::data::copyAndRemap2D2D     (_embedding_A_2D,_embedding_A_2D_view,_limits_A_2D,true);
      hdi::data::copyAndRemap1D2DVertical (_embedding_A_1D,_embedding_A_1D_view,_limits_A_1D);
      hdi::data::copyAndRemap2D2D     (_embedding_B_2D,_embedding_B_2D_view,_limits_B_2D,true);
      hdi::data::copyAndRemap1D2DVertical (_embedding_B_1D,_embedding_B_1D_view,_limits_B_1D);

      _my_id = my_id;
    }

    void WAOWVisSingleViewEmbedder::doAnIteration(){
      const auto iteration_A = _tSNE_A_1D.iteration();
      const auto iteration_B = _tSNE_B_1D.iteration();
      if(_iterate && iteration_A < 1500){//QUICKPAPER
        _tSNE_A_1D.doAnIteration();
        _tSNE_A_2D.doAnIteration();

        if(iteration_A < 500){
          double mult = 1-double(iteration_A)/500;
          if(_intra_set_eq){
            _eq_A_1D_to_2D.doAnIteration(mult);
            _eq_A_2D_to_1D.doAnIteration(mult);
          }
          if(_inter_set_eq){
            _eq_B_1D_to_A_1D.doAnIteration(mult);
            _eq_B_1D_to_A_2D.doAnIteration(mult);
          }
        }

      }
      if(_iterate && iteration_B < 1500){//QUICKPAPER
        _tSNE_B_1D.doAnIteration();
        _tSNE_B_2D.doAnIteration();

        if(iteration_B < 500){
          double mult = 1-double(iteration_B)/500;
          if(_intra_set_eq){
            _eq_B_1D_to_2D.doAnIteration(mult);
            _eq_B_2D_to_1D.doAnIteration(mult);
          }
          if(_inter_set_eq){
            _eq_A_1D_to_B_1D.doAnIteration(mult);
            _eq_A_1D_to_B_2D.doAnIteration(mult);
          }
        }

      }
      //else{
      //  std::this_thread::sleep_for(std::chrono::milliseconds(10));


      hdi::data::copyAndRemap2D2D     (_embedding_A_2D,_embedding_A_2D_view,_limits_A_2D,true);
      hdi::data::copyAndRemap1D2DVertical (_embedding_A_1D,_embedding_A_1D_view,_limits_A_1D);
      hdi::data::copyAndRemap2D2D     (_embedding_B_2D,_embedding_B_2D_view,_limits_B_2D,true);
      hdi::data::copyAndRemap1D2DVertical (_embedding_B_1D,_embedding_B_1D_view,_limits_B_1D);
      //updateTextB();
      _canvas->updateGL();

    }

    void WAOWVisSingleViewEmbedder::updateConnections(bool force){
      //utils::secureLog(_logger,"Updating connections");

      if(_drawers[_visualization_mode].find("Lines_A_B")!=_drawers[_visualization_mode].end()){//A
        auto line_drawer = reinterpret_cast<hdi::viz::EmbeddingBundledLinesDrawer*>(_drawers[_visualization_mode]["Lines_A_B"].get());
        /*
        if(!force && _always_show_lines && line_drawer->lines().size()) {
          return;
        }
        */

        std::vector<uint32_t> selection;
        if(_always_show_lines){
          selection.resize(_panel_data_A.numDataPoints());
          std::iota(selection.begin(),selection.end(),0);
        }else{
          getSelectionA(selection);
        }

        float max_connection = 0;
        for(auto s: selection){
          for(auto link: _connections_AB[s]){
            if(link.second > max_connection){
              max_connection = link.second;
            }
          }
        }

        std::fill(_selection_B.begin(),_selection_B.end(),0);
        std::vector<std::pair<uint32_t,uint32_t>> lines;
        std::vector<float> alpha;
        uint32_t total = 0;
        for(auto s: selection){
          for(auto link: _connections_AB[s]){
            double alpha_v = _lines_alpha*link.second/max_connection;
            if(alpha_v > 0.01){
              lines.push_back(std::pair<uint32_t,uint32_t>(s,link.first));
              alpha.push_back(alpha_v);

            }
              _selection_B[link.first] += link.second;
              ++total;
          }
        }

        if(total == 0 || _always_show_lines){
          std::fill(_selection_B.begin(),_selection_B.end(),0);
        }else{
          for(int i = 0; i < _selection_B.size(); ++i){
            _selection_B[i] = _incoming_connections_B[i]!=0?_selection_B[i]/_incoming_connections_B[i]:0;
            _selection_B[i] *= _emph_selection;
          }
        }

        //MEH
        assert(line_drawer);
        line_drawer->setLines(lines,alpha);


        {
          auto drawer = reinterpret_cast<hdi::viz::ScatterplotDrawerTwoScalarAttributes*>(_drawers[_visualization_mode]["B_2D"].get());
          if(drawer){
            drawer->setLimitsColor(0,1);
          }
        }
        {
          auto drawer = reinterpret_cast<hdi::viz::ScatterplotDrawerTwoScalarAttributes*>(_drawers[_visualization_mode]["B_1D"].get());
          if(drawer){
            drawer->setLimitsColor(0,1);
          }
        }
      }
      _canvas->updateGL();
    }
    void WAOWVisSingleViewEmbedder::getSelectionA(std::vector<uint32_t>& selection)const{
      uint32_t n_dp = _panel_data_A.numDataPoints();
      const auto& flags = _panel_data_A.getFlagsDataPoints();

      selection.clear();
      selection.reserve(n_dp);
      for(int i = 0; i < n_dp; ++i){
        if((flags[i]&panel_data_type::Selected) == panel_data_type::Selected){
          selection.push_back(i);
        }
      }
    }
    void WAOWVisSingleViewEmbedder::getSelectionB(std::vector<uint32_t>& selection)const{
      uint32_t n_dp = _panel_data_B.numDataPoints();
      const auto& flags = _panel_data_B.getFlagsDataPoints();

      selection.clear();
      selection.reserve(n_dp);
      for(int i = 0; i < n_dp; ++i){
        if((flags[i]&panel_data_type::Selected) == panel_data_type::Selected){
          selection.push_back(i);
        }
      }
    }

    void WAOWVisSingleViewEmbedder::setVisualizationMode(const std::string& visualization_mode){
      _visualization_mode = visualization_mode;
      onUpdateCanvas();
    }

    void WAOWVisSingleViewEmbedder::addDrawer  (const std::string& mode,
                      const std::string& name,
                      std::shared_ptr<hdi::viz::AbstractScatterplotDrawer> drawer)
    {
      drawer->initialize(_canvas->context());
      _drawers[mode][name] = drawer;
      onUpdateCanvas();
    }
    void WAOWVisSingleViewEmbedder::addDataView (const std::string& mode,
                      const std::string& name,
                      std::shared_ptr<hdi::viz::AbstractView> dataView)
    {
      _data_views[mode][name] = dataView;
      if(dataView->panelData() == &_panel_data_A){
        _selection_controller_A_1D->addView(dataView.get());
        _selection_controller_A_2D->addView(dataView.get());
      }else if(dataView->panelData() == &_panel_data_B){
        _selection_controller_B_1D->addView(dataView.get());
        _selection_controller_B_2D->addView(dataView.get());
      }else{
        throw std::logic_error("WAOWVisSingleViewEmbedder::addDataView: view is not linked to the right panel data");
      }
      onUpdateCanvas();
    }


    void WAOWVisSingleViewEmbedder::onKeyPressedOnCanvas(int key){
      utils::secureLogValue(_logger,"Key pressed on canvas",key);
      if(key == Qt::Key_N){
        emit sgnCreateNewView(_my_id);
      }
    }

    void WAOWVisSingleViewEmbedder::onUpdateCanvas(){
      _canvas->removeAllDrawers();
      //TODO remove data views
      for(auto& drawer: _drawers[_visualization_mode]){
        _canvas->addDrawer(drawer.second.get());
      }
      for(auto& data_view: _data_views[_visualization_mode]){
        //TODO
      }
      _canvas->updateGL();
    }

    void WAOWVisSingleViewEmbedder::saveImageToFile(std::string prefix){
      std::string filename(QString("%1_V%2.png").arg(prefix.c_str()).arg(_my_id).toStdString());
      _canvas->saveToFile(filename);
    }

  }
}
