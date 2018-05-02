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

#ifndef MULTISCALE_EMBEDDER_SINGLE_VIEW_H
#define MULTISCALE_EMBEDDER_SINGLE_VIEW_H

#include <QObject>
#include <memory>
#include <vector>
#include <unordered_map>
#include <map>
#include "hdi/dimensionality_reduction/sparse_tsne_user_def_probabilities.h"
#include "hdi/data/panel_data.h"
#include "hdi/utils/abstract_log.h"
#include "hdi/visualization/abstract_view.h"
#include "hdi/visualization/abstract_scatterplot_drawer.h"
#include "hdi/visualization/controller_embedding_selection_qobj.h"
#include "hdi/visualization/scatterplot_drawer_user_defined_colors.h"
#include "hdi/dimensionality_reduction/wtsne.h"
#include "hdi/data/map_mem_eff.h"

namespace hdi{
  namespace analytics{

    class MultiscaleEmbedderSingleView: public QObject{
      Q_OBJECT
    public:
      typedef float scalar_type;
      typedef hdi::data::MapMemEff<uint32_t,scalar_type> map_type;
      typedef std::vector<map_type> sparse_scalar_matrix_type;

      typedef std::tuple<unsigned int, unsigned int> id_type;
      typedef hdi::dr::SparseTSNEUserDefProbabilities<scalar_type,sparse_scalar_matrix_type> tsne_type;
      //typedef hdi::dr::WeightedTSNE<scalar_type> tsne_type;
      typedef hdi::data::Embedding<scalar_type> embedding_type;
      typedef hdi::data::PanelData<scalar_type> panel_data_type;

      enum class VisualizationModes{UserDefined=0, Influence=1, Selection=2};

      MultiscaleEmbedderSingleView();
      virtual ~MultiscaleEmbedderSingleView(){}
      void initialize(sparse_scalar_matrix_type& sparse_matrix, id_type my_id, tsne_type::Parameters params = tsne_type::Parameters());
      void doAnIteration();

      //! Return the current log
      utils::AbstractLog* logger()const{return _logger;}
      //! Set a pointer to an existing log
      void setLogger(utils::AbstractLog* logger){_logger = logger; _selection_controller->setLogger(logger);_tSNE.setLogger(logger);}

      embedding_type& getEmbedding(){return _embedding;}
      const embedding_type& getEmbedding()const{return _embedding;}

      panel_data_type& getPanelData(){return _panel_data;}
      const panel_data_type& getPanelData()const{return _panel_data;}

      void addView(std::shared_ptr<hdi::viz::AbstractView> view);
      void addUserDefinedDrawer(std::shared_ptr<hdi::viz::AbstractScatterplotDrawer> drawer);
      void addAreaOfInfluenceDrawer(std::shared_ptr<hdi::viz::AbstractScatterplotDrawer> drawer);
      void addSelectionDrawer(std::shared_ptr<hdi::viz::AbstractScatterplotDrawer> drawer);

      void removeAllUserDefinedDrawers(){_drawers.clear(); onUpdateViewer();}
      void removeAllAoIDrawers(){_influence_drawers.clear(); onUpdateViewer();}
      void removeAllSelectionDrawers(){_selection_drawers.clear(); onUpdateViewer();}

      const id_type& getId(){return _my_id;}

      QWidget* getCanvas(){return _viewer.get();}

      void saveImageToFile(std::string prefix);

    public slots:
      void onActivateUserDefinedMode();
      void onActivateSelectionMode();
      void onActivateInfluencedMode();
      void onUpdateViewer();

    private slots:
      void onSelection(){emit sgnSelection(_my_id); emit sgnSelectionPtr(this);}
      void onKeyPressedOnCanvas(int key);

    signals:
      void sgnNewAnalysisTriggered(id_type id);
      void sgnActivateUserDefinedMode(id_type id);
      void sgnActivateSelectionMode(id_type id);
      void sgnActivateInfluenceMode(id_type id);
      void sgnPropagateSelection(id_type id);
      void sgnClusterizeSelection(id_type id);
      void sgnExport(id_type id);
      void sgnKeyPressedOnCanvas(id_type id, int key);
      void sgnSelection(id_type id);
      void sgnSelectionPtr(MultiscaleEmbedderSingleView* ptr);

    private:
      //unique pointers to members to avoid qt limitation (no copy contructors) bleah
      bool _initialized;
      std::string _name;
      utils::AbstractLog* _logger;

      std::unique_ptr<hdi::viz::ControllerSelectionEmbedding> _selection_controller;

      tsne_type _tSNE;
      embedding_type _embedding;
      panel_data_type _panel_data;
      std::vector<std::shared_ptr<hdi::viz::AbstractView>> _views;
      std::vector<std::shared_ptr<hdi::viz::AbstractScatterplotDrawer>> _drawers;
      std::vector<std::shared_ptr<hdi::viz::AbstractScatterplotDrawer>> _influence_drawers;
      std::vector<std::shared_ptr<hdi::viz::AbstractScatterplotDrawer>> _selection_drawers;
      std::unique_ptr<hdi::viz::ScatterplotCanvas> _viewer;

      id_type _my_id;

      VisualizationModes _visualization_mode;
    };
  }
}

#endif
