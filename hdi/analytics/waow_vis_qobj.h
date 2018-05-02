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

#ifndef WAOW_VIS_H
#define WAOW_VIS_H

#include <QObject>
#include "hdi/analytics/waow_vis_single_view_embedder_qobj.h"
#include "hdi/dimensionality_reduction/hierarchical_sne.h"
#include "hdi/utils/abstract_log.h"
#include <cstdlib>
#include "hdi/visualization/sankey_diagram_qobj.h"
#include "hdi/data/flow_model.h"
#include "hdi/data/map_mem_eff.h"
#include "hdi/data/cluster.h"
#include "hdi/data/histogram.h"
#include "roaring/roaring.hh"

#include <QColor>

namespace hdi{
  namespace analytics{

    class WAOWVis: public QObject{
      Q_OBJECT
    public:
      typedef float scalar_type;
      typedef std::vector<Roaring> sets_type;

      typedef std::vector<hdi::data::MapMemEff<uint32_t,scalar_type>> sparse_scalar_matrix_type;
      typedef uint32_t embedder_id_type;
      typedef hdi::data::PanelData<scalar_type> panel_data_type;
      typedef WAOWVisSingleViewEmbedder embedder_type;
      typedef hdi::dr::HierarchicalSNE<scalar_type,sparse_scalar_matrix_type> hsne_type;

    public:
      typedef std::vector<std::shared_ptr<embedder_type>> multiscale_views_type;


      //! Abstract class for initializer of a hdi::analytics::WAOWVisSingleViewEmbedder (views and scatterplots)
      class AbstractInterfaceInitializer{
      public:
        typedef WAOWVis::embedder_type embedder_type;
        typedef WAOWVis::scalar_type scalar_type;
      public:
        virtual void initialize(std::shared_ptr<embedder_type> embedder)=0;
      };



    public:
      WAOWVis();

      void initialize(std::shared_ptr<sets_type> set_A,
              std::shared_ptr<sets_type> set_B,
              const std::vector<std::shared_ptr<data::AbstractData>>& sets_A_data,
              const std::vector<std::shared_ptr<data::AbstractData>>& sets_B_data,
              hsne_type::Parameters hsne_params_A = hsne_type::Parameters(),
              hsne_type::Parameters hsne_params_B = hsne_type::Parameters());

      void initialize(std::shared_ptr<sets_type> set_A,
              std::shared_ptr<sets_type> set_B,
              const std::vector<std::shared_ptr<data::AbstractData>>& sets_A_data,
              const std::vector<std::shared_ptr<data::AbstractData>>& sets_B_data,
              const std::string& directory
              );

      void save(const std::string& directory)const;

      //! Initialize the Overview
      void createOverview();
      //! Computes a gradient descent iteration on al the embedders in the view
      void doAnIterateOnAllViews();

      //! Return the current log
      utils::AbstractLog* logger()const{return _logger;}
      //! Set a pointer to an existing log
      void setLogger(utils::AbstractLog* logger){_logger = logger;}
      //! Return the current interface initializer
      AbstractInterfaceInitializer* interfaceInitializer()const{return _interface_initializer;}
      //! Set the interface initializer
      void setInterfaceInitializer(AbstractInterfaceInitializer* interface_initializer){_interface_initializer = interface_initializer;}
      //! Return the HSNE hierarchy for sets A
      const hsne_type& hSNE_set_A()const{return _hSNE_set_A;}
      //! Return the HSNE hierarchy for sets B
      const hsne_type& hSNE_set_B()const{return _hSNE_set_B;}
      //! Return the list of views currently active
      const multiscale_views_type& views()const{return _multiscale_views;}
      //! Set the current visualization mode for all the views
      void setVisualizationMode(const std::string& mode);


      const hdi::data::Histogram<scalar_type>& histogramA(){return _histogram_set_A;}
      const hdi::data::Histogram<scalar_type>& histogramB(){return _histogram_set_B;}
      const hdi::data::Histogram<scalar_type>& histogramUniqueA(){return _histogram_unique_set_A;}
      const hdi::data::Histogram<scalar_type>& histogramUniqueB(){return _histogram_unique_set_B;}

      void saveImagesToFile(std::string prefix);
      void getSelectedLandmarksInScale(embedder_id_type id, std::vector<unsigned int>& selection) const;

    public slots:
      //! Create a new view given starting from the View identified by id
      void onCreateNewView(embedder_id_type id);
      void onUpdateViews();

    private:
      //! Preprocess the sets by removing the duplicates and computing the FMC that connects the points
      void preprocessSets(const sets_type& sets,
                std::vector<uint32_t>& unique_pnts,
                std::vector<uint32_t>& num_duplicates,
                sparse_scalar_matrix_type& fmc)const;

      //! Compute the HSNE hierarchy given a FMC
      void computeHSNE(const sparse_scalar_matrix_type& fmc,
               const std::vector<uint32_t>& num_duplicates,
               hsne_type& hsne)const;

      void computeInfluence(const hsne_type& hsne,
                  std::vector<std::vector<Roaring>>& unique_sets_in_landmarks,
                  std::vector<std::vector<uint32_t>>& pnts_to_landmarks)const;

      void computeConnectionsGreedy(embedder_type& embedder)const;
      void computeConnections(embedder_type& embedder)const;

      //! Connects all signals and slots associated with a view
      void connectView(embedder_type* embedder);

      //! load the hsne & co
      void load(const std::string& directory);

    private slots:
      //! Selection performed in one of the embeddings
      void onSelection(embedder_id_type id);

    signals:
      void sgnKeyPressedOnCanvas(embedder_id_type id, int key);

    private:

      std::shared_ptr<const sets_type> _sets_A;
      std::shared_ptr<const sets_type> _sets_B;
      std::vector<std::shared_ptr<data::AbstractData>> _sets_A_data;
      std::vector<std::shared_ptr<data::AbstractData>> _sets_B_data;

      hdi::data::Histogram<scalar_type> _histogram_set_A;
      hdi::data::Histogram<scalar_type> _histogram_set_B;
      hdi::data::Histogram<scalar_type> _histogram_unique_set_A;
      hdi::data::Histogram<scalar_type> _histogram_unique_set_B;

      /////////////////////////////////////////////
      /// Saved on disk
      ////////////////////////////////////////////////
      std::vector<uint32_t> _unique_pnts_A;
      std::vector<uint32_t> _num_duplicates_A;
      std::vector<uint32_t> _unique_pnts_B;
      std::vector<uint32_t> _num_duplicates_B;
      hsne_type   _hSNE_set_A;
      hsne_type   _hSNE_set_B;
      std::vector<std::vector<Roaring>> _pnts_in_landmarks_A;
      std::vector<std::vector<Roaring>> _pnts_in_landmarks_B;
      std::vector<std::vector<uint32_t>> _pnts_to_landmarks_A;
      std::vector<std::vector<uint32_t>> _pnts_to_landmarks_B;
      ////////////////////////////////////////////////



      multiscale_views_type    _multiscale_views;

      utils::AbstractLog* _logger;
      AbstractInterfaceInitializer* _interface_initializer;


      std::string _visualization_mode;

      bool _selection_linked_to_data_points;
      bool _verbose;
    };

//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////  Interface initializer  ///////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////


    class WAOWVisDefaultInterface: public WAOWVis::AbstractInterfaceInitializer{
    public:
      typedef WAOWVis::embedder_type embedder_type;
      typedef WAOWVis::scalar_type scalar_type;
    public:
      virtual void initialize(std::shared_ptr<embedder_type> embedder);
      void initializeDefault(std::shared_ptr<embedder_type> embedder);
      void initializeSimple(std::shared_ptr<embedder_type> embedder);
      void initializeWeights(std::shared_ptr<embedder_type> embedder);
    };


//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////  IO  //////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////

  }
}

#endif
