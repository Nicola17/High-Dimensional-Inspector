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

#ifndef WAOW_VIS_SINGLE_VIEW_EMBEDDER_H
#define WAOW_VIS_SINGLE_VIEW_EMBEDDER_H

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
#include "hdi/dimensionality_reduction/embedding_equalizer.h"
#include "hdi/visualization/scatterplot_text_drawer.h"
#include "hdi/dimensionality_reduction/hierarchical_sne.h"


namespace hdi{
  namespace analytics{

    class WAOWVisSingleViewEmbedder: public QObject{
      Q_OBJECT
    public:
      typedef uint32_t id_type;
      typedef float scalar_type;
      typedef hdi::data::MapMemEff<uint32_t,scalar_type> map_type;
      typedef std::vector<map_type> sparse_scalar_matrix_type;
      typedef hdi::dr::SparseTSNEUserDefProbabilities<scalar_type,sparse_scalar_matrix_type> tsne_type;
      typedef hdi::data::Embedding<scalar_type> embedding_type;
      typedef hdi::data::PanelData<scalar_type> panel_data_type;
      typedef hdi::dr::EmbeddingEqualizer<scalar_type> embedding_equalizer_type;
      typedef hdi::dr::HierarchicalSNE<scalar_type,sparse_scalar_matrix_type> hsne_type;

    public:
      WAOWVisSingleViewEmbedder();
      virtual ~WAOWVisSingleViewEmbedder(){}

      //! Initialize the embedder
      void initialize(const sparse_scalar_matrix_type& fmc_A,
              const sparse_scalar_matrix_type& fmc_B,
              id_type my_id);
      //! compute an iteration of GD
      void doAnIteration();

      //! Return the current log
      utils::AbstractLog* logger()const{return _logger;}
      //! Set a pointer to an existing log
      void setLogger(utils::AbstractLog* logger){_logger = logger;}


          panel_data_type& getPanelDataA()    {return _panel_data_A;}
      const panel_data_type& getPanelDataA()const {return _panel_data_A;}
          panel_data_type& getPanelDataB()    {return _panel_data_B;}
      const panel_data_type& getPanelDataB()const {return _panel_data_B;}

          embedding_type& getEmbeddingA1DView()   {return _embedding_A_1D_view;}
      const embedding_type& getEmbeddingA1DView()const{return _embedding_A_1D_view;}
          embedding_type& getEmbeddingA2DView()   {return _embedding_A_2D_view;}
      const embedding_type& getEmbeddingA2DView()const{return _embedding_A_2D_view;}
          embedding_type& getEmbeddingB1DView()   {return _embedding_B_1D_view;}
      const embedding_type& getEmbeddingB1DView()const{return _embedding_B_1D_view;}
          embedding_type& getEmbeddingB2DView()   {return _embedding_B_2D_view;}
      const embedding_type& getEmbeddingB2DView()const{return _embedding_B_2D_view;}

          embedding_type& getEmbeddingA1D()   {return _embedding_A_1D;}
      const embedding_type& getEmbeddingA1D()const{return _embedding_A_1D;}
          embedding_type& getEmbeddingA2D()   {return _embedding_A_2D;}
      const embedding_type& getEmbeddingA2D()const{return _embedding_A_2D;}
          embedding_type& getEmbeddingB1D()   {return _embedding_B_1D;}
      const embedding_type& getEmbeddingB1D()const{return _embedding_B_1D;}
          embedding_type& getEmbeddingB2D()   {return _embedding_B_2D;}
      const embedding_type& getEmbeddingB2D()const{return _embedding_B_2D;}


      tsne_type& gettSNEA1D(){return _tSNE_A_1D;}
      tsne_type& gettSNEA2D(){return _tSNE_A_2D;}
      tsne_type& gettSNEB1D(){return _tSNE_B_1D;}
      tsne_type& gettSNEB2D(){return _tSNE_B_2D;}
      const tsne_type& gettSNEA1D()const{return _tSNE_A_1D;}
      const tsne_type& gettSNEA2D()const{return _tSNE_A_2D;}
      const tsne_type& gettSNEB1D()const{return _tSNE_B_1D;}
      const tsne_type& gettSNEB2D()const{return _tSNE_B_2D;}

      void getSelectionA(std::vector<uint32_t>& selection)const;
      void getSelectionB(std::vector<uint32_t>& selection)const;

      //! Return id
      id_type getId()const{return _my_id;}
      //! Return cavas (maybe const?)
      QWidget* getCanvas(){return _canvas.get();}

      //! Set the current visualization mode
      void setVisualizationMode(const std::string& visualization_mode);
      //! Add a scatterplot drawer
      void addDrawer  (const std::string& mode, const std::string& name,std::shared_ptr<hdi::viz::AbstractScatterplotDrawer> drawer);
      //! Add a data view
      void addDataView(const std::string& mode, const std::string& name, std::shared_ptr<hdi::viz::AbstractView> dataView);

      void saveImageToFile(std::string prefix);

      void updateConnections(bool force = false);

    public slots:
      //! update canvas with the proper drawers and redraw
      void onUpdateCanvas();

    private slots:
      void onSelection(){emit sgnSelection(_my_id); emit sgnSelectionPtr(this); updateConnections();}
      void onKeyPressedOnCanvas(int key);

    signals:
      void sgnCreateNewView(id_type id);
      void sgnSelection(id_type id);
      void sgnSelectionPtr(WAOWVisSingleViewEmbedder* ptr);

    private:
      //unique pointers to members to avoid qt limitation (no copy constructors) bleah
      bool _initialized;
      std::string _name;
      utils::AbstractLog* _logger;

      std::unique_ptr<hdi::viz::ScatterplotCanvas> _canvas;

      std::unique_ptr<hdi::viz::ControllerSelectionEmbedding> _selection_controller_A_1D;
      std::unique_ptr<hdi::viz::ControllerSelectionEmbedding> _selection_controller_A_2D;
      std::unique_ptr<hdi::viz::ControllerSelectionEmbedding> _selection_controller_B_1D;
      std::unique_ptr<hdi::viz::ControllerSelectionEmbedding> _selection_controller_B_2D;

      tsne_type _tSNE_A_1D;
      tsne_type _tSNE_A_2D;
      tsne_type _tSNE_B_1D;
      tsne_type _tSNE_B_2D;

      embedding_type _embedding_A_1D;
      embedding_type _embedding_A_2D;
      embedding_type _embedding_B_1D;
      embedding_type _embedding_B_2D;

      embedding_equalizer_type _eq_A_1D_to_2D;
      embedding_equalizer_type _eq_B_1D_to_2D;
      embedding_equalizer_type _eq_A_2D_to_1D;
      embedding_equalizer_type _eq_B_2D_to_1D;

      embedding_equalizer_type _eq_A_1D_to_B_1D;
      embedding_equalizer_type _eq_A_1D_to_B_2D;
      embedding_equalizer_type _eq_B_1D_to_A_1D;
      embedding_equalizer_type _eq_B_1D_to_A_2D;

      embedding_type _embedding_A_1D_view;
      embedding_type _embedding_A_2D_view;
      embedding_type _embedding_B_1D_view;
      embedding_type _embedding_B_2D_view;

      panel_data_type _panel_data_A;
      panel_data_type _panel_data_B;

      std::vector<scalar_type>_limits_A_2D;
      std::vector<scalar_type>_limits_A_1D;
      std::vector<scalar_type>_limits_B_2D;
      std::vector<scalar_type>_limits_B_1D;
      std::vector<scalar_type>_limits_B_Text;

      std::map<std::string,std::map<std::string,std::shared_ptr<hdi::viz::AbstractView>>>         _data_views;


    //the separation with WAOWVis is crumbling! (maybe it should, remove view and put everything in here)
    public:
      std::map<std::string,std::map<std::string,std::shared_ptr<hdi::viz::AbstractScatterplotDrawer>>>  _drawers;
      scalar_type _lines_alpha;
      bool _always_show_lines;
      bool _iterate;
      bool _intra_set_eq;
      bool _inter_set_eq;

      double _emph_selection;

      int _scale_A;
      int _scale_B;

      id_type _my_id;
      id_type _parent_id;
      std::string _visualization_mode;

      std::vector<uint32_t>       _scale_idxes_A;     //indices of the selection in the scale for the sets A
      std::vector<uint32_t>       _scale_idxes_B;     //indices of the selection in the scale for the sets B

      std::vector<scalar_type>    _landmark_weights_A;  //copy of the weights of the landmarks for the sets A
      std::vector<scalar_type>    _landmark_weights_B;  //copy of the weights of the landmarks for the sets B

      std::vector<uint32_t>       _parent_selection_A;  //indices of the selection in parent embedding for the sets A (NB: not in the scale)
      std::vector<uint32_t>       _parent_selection_B;  //indices of the selection in parent embedding for the sets B (NB: not in the scale)

      std::vector<scalar_type>    _selection_A;
      std::vector<scalar_type>    _selection_B;

      std::vector<std::unordered_map<uint32_t,uint32_t>> _connections_AB;
      std::vector<std::unordered_map<uint32_t,uint32_t>> _connections_BA;
      std::vector<scalar_type>    _incoming_connections_A;
      std::vector<scalar_type>    _incoming_connections_B;

    };
  }
}

#endif
