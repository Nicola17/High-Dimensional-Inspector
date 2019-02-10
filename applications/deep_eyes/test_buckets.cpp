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

#include "hdi/utils/cout_log.h"
#include "hdi/utils/log_helper_functions.h"
#include "hdi/dimensionality_reduction/tsne.h"
#include "hdi/dimensionality_reduction/sparse_tsne_user_def_probabilities.h"
#include <qimage.h>
#include <QApplication>
#include "hdi/visualization/scatterplot_canvas_qobj.h"
#include "hdi/visualization/scatterplot_drawer_fixed_color.h"
#include "hdi/visualization/scatterplot_drawer_scalar_attribute.h"
#include <iostream>
#include <fstream>
#include "hdi/data/panel_data.h"
#include "hdi/data/empty_data.h"
#include "hdi/data/image_data.h"
#include "hdi/data/pixel_data.h"
#include "hdi/data/text_data.h"
#include "hdi/visualization/multiple_image_view_qobj.h"
#include "hdi/utils/visual_utils.h"
#include <QDir>
#include "hdi/data/embedding.h"
#include "hdi/visualization/controller_embedding_selection_qobj.h"
#include "hdi/dimensionality_reduction/hierarchical_sne.h"
#include "hdi/dimensionality_reduction/sparse_tsne_user_def_probabilities.h"
#include "hdi/visualization/multiple_heatmaps_view_qobj.h"
#include "hdi/analytics/multiscale_embedder_single_view_qobj.h"
#include "hdi/analytics/multiscale_embedder_system_qobj.h"
#include "hdi/utils/graph_algorithms.h"
#include "hdi/utils/math_utils.h"
#include "hdi/utils/dataset_utils.h"
#include "hdi/dimensionality_reduction/evaluation.h"
#include "hdi/dimensionality_reduction/hd_joint_probability_generator.h"
#include "hdi/visualization/scatterplot_drawer_labels.h"
#include "hdi/visualization/heatmap_view_qobj.h"

int main(int argc, char *argv[]){
	try{
		typedef float scalar_type;
		QApplication app(argc, argv);

		hdi::utils::CoutLog log;

///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////
        if(argc != 5){
			hdi::utils::secureLog(&log,"Not enough input parameters...");
			return 1;
		}

        std::map<unsigned int, QColor> palette;
        palette[0] = qRgb(150,0,0);
        palette[1] = qRgb(0,150,255);

        hdi::analytics::MultiscaleEmbedderSystem::panel_data_type panel_data;
        std::vector<unsigned int> labels;
        hdi::utils::IO::loadMitoticFigures(panel_data,labels,argv[1],argv[2],std::atoi(argv[3]),std::atoi(argv[4]));

///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////

        const int num_buckets = 2;
        const int num_dims = panel_data.numDimensions();
        const int num_dps = panel_data.numDataPoints();

        std::vector<scalar_type> max_values;
        hdi::data::getMaxPerDimension(panel_data,max_values);

        hdi::utils::secureLogVectorWithStats(&log,"Max",max_values);
        std::vector<scalar_type> buckets;


        std::map<std::vector<unsigned int>,unsigned int> space_final_frontier;

        for(int i = 0; i < num_dps; ++i){
        //for(int i = 0; i < 15; ++i){
            std::vector<unsigned int> features(num_dims,0);
            for(int d = 0; d < num_dims; ++d){
                features[d] = panel_data.dataAt(i,d)/(max_values[d]/num_buckets+0.001);
            }
            ++space_final_frontier[features];
            //++space_final_frontier[features];
            //hdi::utils::secureLogVectorWithStats(&log,"F",features);
        }

        std::vector<int> stats;
        for(auto& s: space_final_frontier){
            //hdi::utils::secureLogValue(&log,"#elems",s.second);
            stats.push_back(s.second);
        }
        hdi::utils::secureLogVectorStats(&log,"S",stats);

	}
	catch(std::logic_error& ex){ std::cout << "Logic error: " << ex.what();}
	catch(std::runtime_error& ex){ std::cout << "Runtime error: " << ex.what();}
	catch(...){ std::cout << "An unknown error occurred";}
}
