/*
 *
 * Copyright (c) 2014, Nicola Pezzotti (Delft University of Technology)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *    This product includes software developed by the Delft University of Technology.
 * 4. Neither the name of the Delft University of Technology nor the names of
 *    its contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
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

#ifndef HSNE_ANALYTICS_SYSTEM_H
#define HSNE_ANALYTICS_SYSTEM_H

#include <QObject>
#include "hsne_analytics_interfaces_qobj.h"
#include "hdi/dimensionality_reduction/hierarchical_sne.h"
#include "hdi/utils/abstract_log.h"
#include "hdi/data/abstract_panel_data.h"
#include "hdi/data/map_mem_eff.h"
#include <cstdlib>

namespace hdi{
    namespace analytics{


        class HSNEAnalyticsSystem: public QObject{
            Q_OBJECT
        public:
            typedef float scalar_type;
            typedef uint32_t unsigned_int_type;
            typedef std::vector<hdi::data::MapMemEff<unsigned_int_type,scalar_type>> sparse_scalar_matrix_type;
            typedef hdi::data::AbstractPanelData    panel_data_type;
            typedef AbstractHSNEEmbedderFactory     embedder_factory_type;
            typedef AbstractHSNEEmbedder            embedder_type;
            typedef embedder_type::id_type          embedder_id_type;
            typedef hdi::dr::HierarchicalSNE<scalar_type,sparse_scalar_matrix_type> hsne_type;
            typedef std::vector<std::vector<std::shared_ptr<embedder_type>>> multiscale_analysis_type;

        public:
            HSNEAnalyticsSystem();

            //! Initialize the system with a given number of scales and on a dense high-dim space
            void initialize(unsigned int num_scales, scalar_type* data, hsne_type::Parameters hsne_params = hsne_type::Parameters());
            //! Load the hierarchical representation from file
            void initializeFromFile(std::string filename);

            //! Return the current log
            utils::AbstractLog* logger()const{return _logger;}
            //! Set a pointer to an existing log
            void setLogger(utils::AbstractLog* logger){_logger = logger;}
            //! Return the current factory
            embedder_factory_type* factory()const{return _factory;}
            //! Set a pointer to an existing factory
            void setFactory(embedder_factory_type* factory){_factory = factory;}
            //! Return the current panel data
            panel_data_type* panelData()const{return _panel_data;}
            //! Set a pointer to an existing panel data
            void setpanelData(panel_data_type* panel_data){_panel_data = panel_data;}

            //! Create the embedder for the highest scale in the hierarchy
            void createTopLevelEmbedder();

            //! Compute a gradient descent itereation on every embedder
            void doAnIterateOnAllEmbedder();


        private:
            void connectEmbedder(embedder_type* embedder);

        public slots:
            void onNewEmbedder(){}
            void onDeleteEmbedder(){}

        private:
            hsne_type                   _hSNE;
            utils::AbstractLog*         _logger;
            embedder_factory_type*      _factory;
            panel_data_type*            _panel_data;
            multiscale_analysis_type    _multiscale_analysis;
            std::vector<uint32_t>       _analysis_counter; //used to assign an id to the analysis
            bool _initialized;

            /*
        public:
            typedef float scalar_type;
            typedef std::vector<std::unordered_map<uint32_t,scalar_type>> sparse_scalar_matrix_type;
            typedef std::tuple<uint32_t, uint32_t> embedder_id_type;
            typedef hdi::data::PanelData<scalar_type> panel_data_type;
            typedef MultiscaleEmbedderSingleView embedder_type;
            typedef hdi::dr::HierarchicalSNE<scalar_type,sparse_scalar_matrix_type> hsne_type;

        public:
            class Analysis{
            public:
                Analysis():
                    _embedder(new embedder_type())
                {}

                void resetStochasticSelection(){for(auto& s: _selection) s = 0;}

            public:
                std::shared_ptr<embedder_type> _embedder; //unique_ptr is not working on windows!
                std::vector<unsigned int> _scale_idxes; //indices of the selection in the scale
                std::vector<scalar_type> _landmark_weight; //copy of the influence of the selection
                std::vector<scalar_type> _selection; //linked selection
            };
            typedef Analysis analysis_type;
            typedef std::vector<std::vector<analysis_type>> multiscale_analysis_type;

            //! Abstract class for initializer of a hdi::analytics::MultiscaleEmbedderSingleView (views and scatterplots)
            class AbstractInterfaceInitializer{
            public:
                typedef HSNEAnalyticsSystem::embedder_type embedder_type;
                typedef HSNEAnalyticsSystem::scalar_type scalar_type;
            public:
                virtual void initializeStandardVisualization(embedder_type* embedder, const std::vector<unsigned int>& idxes_to_orig_data)=0;
                virtual void initializeInfluenceVisualization(embedder_type* embedder, const std::vector<unsigned int>& idxes_to_orig_data, scalar_type* influence)=0;
                virtual void initializeSelectionVisualization(embedder_type* embedder, const std::vector<unsigned int>& idxes_to_orig_data, scalar_type* selection)=0;

                virtual void updateSelection(embedder_type* embedder, scalar_type* selection)=0;

                virtual void dataPointSelectionChanged(const std::vector<scalar_type>& selection)=0;
            };

        public slots:
            void onNewAnalysisTriggered(embedder_id_type id);
            void onActivateUserDefinedMode(embedder_id_type id);
            void onActivateSelectionMode(embedder_id_type id);
            void onActivateInfluenceMode(embedder_id_type id);
            void onPropagateSelection(embedder_id_type id);
            void onClusterizeSelection(embedder_id_type id);
            void onLinkSelectionToDataPoints(embedder_id_type id);

        public:
            HSNEAnalyticsSystem();

            void initialize(unsigned int num_scales, hsne_type::Parameters rw_params = hsne_type::Parameters());
            void initializeFromFile(std::string filename);
            void createTopLevelEmbedder();
            void createFullScaleEmbedder(unsigned int scale);
            void doAnIterateOnAllEmbedder();

            //! Return the number of scales
            unsigned int numScales(){return _multiscale_analysis.size();}

            panel_data_type& getPanelData(){return _panel_data;}
            const panel_data_type& getPanelData()const{return _panel_data;}
            //! Return the current log
            utils::AbstractLog* logger()const{return _logger;}
            //! Set a pointer to an existing log
            void setLogger(utils::AbstractLog* logger){_logger = logger;}

            AbstractInterfaceInitializer* interfaceInitializer()const{return _interface_initializer;}
            void setInterfaceInitializer(AbstractInterfaceInitializer* interface_initializer){_interface_initializer = interface_initializer;}

            const hsne_type& hSNE()const{return _hSNE;}

        private:
            void connectEmbedder(embedder_type* embedder);
            void getScaleAndAnalysisId(embedder_id_type id, unsigned int& scale_id, unsigned int& analysis_id);
            void getSelectionInTheAnalysis(const analysis_type& analysis, std::vector<unsigned int>& selection)const;
            void getSelectionInTheScale(const analysis_type& analysis, std::vector<unsigned int>& selection)const;
            void visualizeTheFlow();


        private:
            hsne_type   _hSNE;

            multiscale_analysis_type        _multiscale_analysis;
            panel_data_type                 _panel_data;
            std::vector<unsigned int>       _analysis_counter; //used to assign an id to the analysis

            std::vector<std::vector<std::unordered_set<unsigned int> > > _clusters;

            utils::AbstractLog* _logger;
            AbstractInterfaceInitializer* _interface_initializer;

            bool _selection_linked_to_data_points;
            */
        };
    }
}

#endif
