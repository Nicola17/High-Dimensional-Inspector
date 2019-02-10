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
#include "hdi/data/pixel_data.h"
#include "hdi/data/image_data.h"
#include "hdi/visualization/image_view_qobj.h"
#include "hdi/visualization/scatterplot_drawer_user_defined_colors.h"
#include "hdi/visualization/scatterplot_drawer_labels.h"
#include "hdi/utils/visual_utils.h"
#include "hdi/utils/graph_algorithms.h"
#include "hdi/data/embedding.h"
#include "hdi/visualization/controller_embedding_selection_qobj.h"
#include <QDir>
#include "hdi/utils/math_utils.h"
#include "hdi/utils/timing_utils.h"
#include "hdi/utils/timers.h"
#include "hdi/utils/scoped_timers.h"
#include "hdi/data/nano_flann.h"
#include "hdi/dimensionality_reduction/evaluation.h"
#include "hdi/utils/dataset_utils.h"

extern "C" {
    #include "pan_dconv.h"
    #include "pan_force.h"
    #include "pan_lamp.h"
    #include "pan_estimate.h"
}

int main(int argc, char *argv[]){
	try{
		typedef float scalar_type;
		QApplication app(argc, argv);

		hdi::utils::CoutLog log;

        hdi::utils::secureLogValue(&log,"PAN DECIMAL SIZE", sizeof(decimal));

///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////
        if(argc != 4){
            hdi::utils::secureLog(&log,"Not enough input parameters...");
            return 1;
        }
        std::vector<QColor> color_per_digit;
        color_per_digit.push_back(qRgb(16,78,139));
        color_per_digit.push_back(qRgb(139,90,43));
        color_per_digit.push_back(qRgb(138,43,226));
        color_per_digit.push_back(qRgb(0,128,0));
        color_per_digit.push_back(qRgb(255,150,0));
        color_per_digit.push_back(qRgb(204,40,40));
        color_per_digit.push_back(qRgb(131,139,131));
        color_per_digit.push_back(qRgb(0,205,0));
        color_per_digit.push_back(qRgb(20,20,20));
        color_per_digit.push_back(qRgb(0, 150, 255));

        hdi::data::PanelData<scalar_type> panel_data;
        std::vector<unsigned int> labels;

        hdi::utils::IO::loadCifar10(panel_data,labels,argv[1],argv[2],std::atoi(argv[3]));
        hdi::utils::secureLog(&log,"Data loaded...");

///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////

        int num_samples = sqrt(panel_data.numDataPoints()) * 3;
        //int num_samples = 411;
        hdi::utils::secureLogValue(&log,"#samples", num_samples);
        int projdim = 2;
        hdi::data::Embedding<scalar_type> embedding;
        embedding.resize(projdim,panel_data.numDataPoints());

        hdi::data::Embedding<scalar_type> embedding_samples;
        embedding_samples.resize(projdim,num_samples);


        struct sampling_struct sampleinfo = {DEFAULT_VALUE};
          sampleinfo.numpoints = panel_data.numDataPoints();
          sampleinfo.highdim = panel_data.numDimensions();
          sampleinfo.numsamples = num_samples;

        unsigned char hasid=0;
        unsigned char hascl=0;
        struct id_struct ids = {DEFAULT_VALUE};
        ids.retrieve = hasid ? GET_ID : CREATE_ID;

        std::vector<std::string> omg;
        for(int i = 0; i < panel_data.numDataPoints(); ++i){
            std::stringstream ss;
            ss << i;
            omg.push_back(ss.str());
        }

        char pippo []= "pippo\n";
        std::vector<char*> ids_values(panel_data.numDataPoints());
        for(int i = 0; i < panel_data.numDataPoints(); ++i){
            ids_values[i] = (char*)(omg[i].c_str());
        }

        /*for(auto& s: ids_values){
            s = pippo;
        }
        */

        struct class_struct classes = {DEFAULT_VALUE};
            if (hascl) {
              classes.retrieve = GET_CLASS;
              classes.values = (char**) malloc (panel_data.numDataPoints() * sizeof(char*));
              classes.enumeration = (char**) calloc (maxclass_get(), sizeof(char*));
            }
    double time(0);
    char **sampleid = (char**) malloc (num_samples*sizeof(char*));

    {
        hdi::utils::ScopedTimer<double>timer(time);
        hdi::utils::secureLog(&log,"Sampling...");
        std::vector<scalar_type> sampled_data(num_samples*panel_data.numDimensions(),0);
        sampling_execute(panel_data.getData().data(), ids_values.data(), &sampleinfo, sampled_data.data(), sampleid);

        hdi::utils::secureLog(&log,"IDMap...");
        struct idmap_struct idmapinfo = {DEFAULT_VALUE};
          idmapinfo.numpoints = num_samples;
          idmapinfo.highdim = panel_data.numDimensions();
          idmapinfo.projdim = projdim;
          idmapinfo.numiterations = 100;
        idmap_execute(sampled_data.data(), sampleid, &idmapinfo, embedding_samples.getContainer().data());


        hdi::utils::secureLog(&log,"LAMP...");
        struct lamp_struct lampinfo = {DEFAULT_VALUE};
          lampinfo.numpoints = panel_data.numDataPoints();
          lampinfo.numsamples = num_samples;
          lampinfo.highdim = panel_data.numDimensions();
          lampinfo.projdim = projdim;
        lamp_execute(panel_data.getData().data(), sampled_data.data(), embedding_samples.getContainer().data(), &lampinfo, embedding.getContainer().data());
        hdi::utils::secureLog(&log,"DONE!");
     }
    hdi::utils::secureLogValue(&log,"Time",time);

/*
        std::vector<scalar_type> precision;
        std::vector<scalar_type> recall;
        hdi::computePrecisionRecall(panel_data,embedding,20,precision,recall);
        for(int i = 0; i < precision.size(); ++i){
            std::cout << recall[i] << "\t" << precision[i] << std::endl;
        }

        hdi::utils::secureLogValue(&log,"time",time);

		hdi::viz::ScatterplotCanvas viewer;
        viewer.setBackgroundColors(qRgb(255,255,255),qRgb(255,255,255));
        viewer.resize(900,900);
		viewer.show();

        std::vector<uint32_t> flags(num_pics,0);
        std::vector<float> embedding_colors_for_viz(num_pics*3,0);

        for(int i = 0; i < num_pics; ++i){
            int label = labels[i];
			auto color = color_per_digit[label];
			embedding_colors_for_viz[i*3+0] = color.redF();
			embedding_colors_for_viz[i*3+1] = color.greenF();
			embedding_colors_for_viz[i*3+2] = color.blueF();
		}

		hdi::viz::ScatterplotDrawerUsedDefinedColors drawer;
		drawer.initialize(viewer.context());
        drawer.setData(embedding.getContainer().data(), embedding_colors_for_viz.data(), flags.data(), num_pics);
        //drawer.setAlpha(0.2);
		drawer.setPointSize(5);
		viewer.addDrawer(&drawer);

        {//limits
            std::vector<scalar_type> limits;
            embedding.computeEmbeddingBBox(limits,0.25);
            auto tr = QVector2D(limits[1],limits[3]);
            auto bl = QVector2D(limits[0],limits[2]);
            viewer.setTopRightCoordinates(tr);
            viewer.setBottomLeftCoordinates(bl);
        }

        viewer.updateGL();
        viewer.show();
*/

        std::map<unsigned int, QColor> palette;
        palette[0] = qRgb(16,78,139);
        palette[1] = qRgb(139,90,43);
        palette[2] = qRgb(138,43,226);
        palette[3] = qRgb(0,128,0);
        palette[4] = qRgb(255,150,0);
        palette[5] = qRgb(204,40,40);
        palette[6] = qRgb(131,139,131);
        palette[7] = qRgb(0,205,0);
        palette[8] = qRgb(20,20,20);
        palette[9] = qRgb(0, 150, 255);

        std::vector<uint32_t> flags(embedding.numDataPoints(),0);
        hdi::viz::ScatterplotCanvas viewer_full;
        hdi::viz::ScatterplotDrawerLabels drawer_labels;
        {
            viewer_full.setBackgroundColors(qRgb(255,255,255),qRgb(255,255,255));
            viewer_full.resize(600,600);
            viewer_full.show();
            std::vector<scalar_type> limits;
            embedding.computeEmbeddingBBox(limits,0.2);
            auto tr = QVector2D(limits[1],limits[3]);
            auto bl = QVector2D(limits[0],limits[2]);
            viewer_full.setTopRightCoordinates(tr);
            viewer_full.setBottomLeftCoordinates(bl);

            drawer_labels.initialize(viewer_full.context());
            drawer_labels.setData(embedding.getContainer().data(),flags.data(),labels.data(),palette,embedding.numDataPoints());
            drawer_labels.setPointSize(5);
            viewer_full.addDrawer(&drawer_labels);

            viewer_full.updateGL();
            viewer_full.show();
            QApplication::processEvents();
        }

        hdi::viz::ScatterplotCanvas viewer_fix_color;
        hdi::viz::ScatterplotDrawerFixedColor drawer_fixed;
        {
            viewer_fix_color.setBackgroundColors(qRgb(255,255,255),qRgb(255,255,255));
            viewer_fix_color.resize(600,600);
            viewer_fix_color.show();
            std::vector<scalar_type> limits;
            embedding.computeEmbeddingBBox(limits,0.2);
            auto tr = QVector2D(limits[1],limits[3]);
            auto bl = QVector2D(limits[0],limits[2]);
            viewer_fix_color.setTopRightCoordinates(tr);
            viewer_fix_color.setBottomLeftCoordinates(bl);

            drawer_fixed.initialize(viewer_fix_color.context());
            drawer_fixed.setData(embedding.getContainer().data(),flags.data(),embedding.numDataPoints());
            drawer_fixed.setPointSize(5);
            drawer_fixed.setAlpha(0.2);
            viewer_fix_color.addDrawer(&drawer_fixed);

            viewer_fix_color.updateGL();
            viewer_fix_color.show();
            QApplication::processEvents();
        }


        std::vector<unsigned int> labels_pivot(num_samples);
        std::vector<uint32_t> flags_pivot(num_samples,0);
        hdi::viz::ScatterplotCanvas viewer_pivot;
        hdi::viz::ScatterplotDrawerLabels drawer_labels_pivot;
        {
            for(int i = 0; i < num_samples; ++i){
                labels_pivot[i] = labels[std::atoi(sampleid[i])];
            }
        }
        {
            viewer_pivot.setBackgroundColors(qRgb(255,255,255),qRgb(255,255,255));
            viewer_pivot.resize(600,600);
            viewer_pivot.show();
            std::vector<scalar_type> limits;
            embedding.computeEmbeddingBBox(limits,0.2);
            auto tr = QVector2D(limits[1],limits[3]);
            auto bl = QVector2D(limits[0],limits[2]);
            viewer_pivot.setTopRightCoordinates(tr);
            viewer_pivot.setBottomLeftCoordinates(bl);

            drawer_labels_pivot.initialize(viewer_pivot.context());
            drawer_labels_pivot.setData(embedding_samples.getContainer().data(),flags_pivot.data(),labels_pivot.data(),palette,embedding_samples.numDataPoints());
            drawer_labels_pivot.setPointSize(8);
            viewer_pivot.addDrawer(&drawer_labels_pivot);

            viewer_pivot.updateGL();
            viewer_pivot.show();
            QApplication::processEvents();
        }

        std::vector<scalar_type> precision;
        std::vector<scalar_type> recall;
        std::vector<unsigned int> pnts_to_evaluate(panel_data.numDataPoints());
        std::iota(pnts_to_evaluate.begin(),pnts_to_evaluate.end(),0);

        hdi::dr::computePrecisionRecall(panel_data,embedding,pnts_to_evaluate,precision,recall,30);

        std::cout << std::endl << std::endl;
        std::ofstream latex_chart_stream("cifar_pr.tex");
        latex_chart_stream << "\\addplot[orange]coordinates{" << std::endl;
        for(int i = 0; i < precision.size(); ++i){
            std::cout << precision[i] << "\t" << recall[i] << std::endl;
            latex_chart_stream << "(" << precision[i] << "," << recall[i] << ")" << std::endl;
        }
        latex_chart_stream << "};\\label{plots:LAMP:CIFAR}" << std::endl;

        int it = 0;
        while(++it<10){
            viewer_fix_color.saveToFile("cifar_fixed_color.png");
            viewer_full.saveToFile("cifar_full.png");
            viewer_pivot.saveToFile("cifar_landmarks.png");
            QApplication::processEvents();
        }

		return app.exec();
	}
	catch(std::logic_error& ex){ std::cout << "Logic error: " << ex.what();}
	catch(std::runtime_error& ex){ std::cout << "Runtime error: " << ex.what();}
	catch(...){ std::cout << "An unknown error occurred";}
}
