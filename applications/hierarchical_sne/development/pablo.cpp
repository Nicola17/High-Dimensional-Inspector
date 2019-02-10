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
#include "hdi/dimensionality_reduction/hierarchical_sne.h"
#include "hdi/dimensionality_reduction/sparse_tsne_user_def_probabilities.h"
#include <qimage.h>
#include <QApplication>
#include "hdi/visualization/scatterplot_canvas_qobj.h"
#include "hdi/visualization/scatterplot_drawer_fixed_color.h"
#include <iostream>
#include <fstream>
#include "hdi/data/panel_data.h"
#include "hdi/data/empty_data.h"
#include "hdi/data/image_data.h"
#include "hdi/visualization/image_view_qobj.h"
#include "hdi/visualization/scatterplot_drawer_labels.h"
#include "hdi/visualization/scatterplot_drawer_fixed_color.h"
#include "hdi/utils/visual_utils.h"
#include <QDir>
#include "hdi/analytics/multiscale_embedder_system_qobj.h"
#include "hdi/visualization/multiple_heatmaps_view_qobj.h"
#include "hdi/utils/graph_algorithms.h"
#include "hdi/utils/math_utils.h"
#include "hdi/dimensionality_reduction/sparse_tsne_user_def_probabilities.h"

int main(int argc, char *argv[]){
    try{
        typedef float scalar_type;
        QApplication app(argc, argv);

        hdi::utils::CoutLog log;

///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////
        if(argc < 3){
            hdi::utils::secureLog(&log,"Not enough input parameters...");
            return 1;
        }

        const int num_points(std::atoi(argv[1]));
        std::ifstream file_data(argv[2], std::ios::in|std::ios::binary);
        if (!file_data.is_open()){
            throw std::runtime_error("data file cannot be open");
        }

        float sum = 0;
        hdi::dr::SparseTSNEUserDefProbabilities<float>::sparse_scalar_matrix_type matrix;
        matrix.resize(num_points);
        for(int j = 0; j < num_points; ++j){
            for(int i = 0; i < num_points; ++i){
                float appo;
                file_data.read((char*)&appo,4);
                std::cout << appo << " " ;
                if(appo!=0){
                    matrix[i][j] = appo;
                    matrix[j][i] = appo;
                    sum += appo*2;
                }
            }
            std::cout << "\n" ;
        }

        for(auto& r: matrix)
            for(auto& c: r)
                c.second /= sum;

///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////


        hdi::dr::SparseTSNEUserDefProbabilities<float> tsne;
        hdi::data::Embedding<float> embedding;
        std::vector<uint32_t> flags(num_points,0);
        tsne.initialize(matrix,&embedding);

        hdi::viz::ScatterplotCanvas viewer;
        viewer.setBackgroundColors(qRgb(255,255,255),qRgb(255,255,255));
        viewer.setSelectionColor(qRgb(50,50,50));
        viewer.resize(500,500);
        viewer.show();
        hdi::viz::ScatterplotDrawerFixedColor drawer;
        drawer.initialize(viewer.context());
        drawer.setData(embedding.getContainer().data(), flags.data(), num_points);
        drawer.setAlpha(0.5);
        drawer.setPointSize(5);
        viewer.addDrawer(&drawer);

        while(true){
            tsne.doAnIteration();
            {//limits
                std::vector<scalar_type> limits;
                embedding.computeEmbeddingBBox(limits,0.25);
                auto tr = QVector2D(limits[1],limits[3]);
                auto bl = QVector2D(limits[0],limits[2]);
                viewer.setTopRightCoordinates(tr);
                viewer.setBottomLeftCoordinates(bl);
            }

            viewer.updateGL();
            QApplication::processEvents();
        }
        return app.exec();
    }
    catch(std::logic_error& ex){ std::cout << "Logic error: " << ex.what();}
    catch(std::runtime_error& ex){ std::cout << "Runtime error: " << ex.what();}
    catch(...){ std::cout << "An unknown error occurred";}
}
