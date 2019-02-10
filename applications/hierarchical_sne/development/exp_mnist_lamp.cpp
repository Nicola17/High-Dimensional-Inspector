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

extern "C" {
    #include "pan_dconv.h"
    #include "pan_force.h"
    #include "pan_lamp.h"
    #include "pan_estimate.h"
}

namespace hdi{

template <typename scalar_type>
class PanelDataNanoFlannAdaptor{
public:
    typedef data::PanelData<scalar_type> panel_data_type;
    PanelDataNanoFlannAdaptor(const panel_data_type& panel_data):_panel_data(panel_data){}

    // Must return the number of data points
    inline size_t kdtree_get_point_count() const { return _panel_data.numDataPoints(); }

    // Returns the distance between the vector "p1[0:size-1]" and the data point with index "idx_p2" stored in the class:
    inline scalar_type kdtree_distance(const scalar_type *p1, const size_t idx_p2,size_t /*size*/) const
    {
        scalar_type dist(0);
        for(int d = 0; d < _panel_data.numDimensions(); ++d){
            const scalar_type v = p1[d]-_panel_data.dataAt(idx_p2,d);
            dist += v*v;
        }
        return dist;
    }

    // Returns the dim'th component of the idx'th point in the class:
    // Since this is inlined and the "dim" argument is typically an immediate value, the
    //  "if/else's" are actually solved at compile time.
    inline scalar_type kdtree_get_pt(const size_t idx, int dim) const
    {
        return _panel_data.dataAt(idx,dim);
    }

    // Optional bounding-box computation: return false to default to a standard bbox computation loop.
    //   Return true if the BBOX was already computed by the class and returned in "bb" so it can be avoided to redo it again.
    //   Look at bb.size() to find out the expected dimensionality (e.g. 2 or 3 for point clouds)
    template <class BBOX>
    bool kdtree_get_bbox(BBOX& /* bb */) const { return false; }

private:
    const panel_data_type& _panel_data;
};

template <typename scalar_type>
class EmbeddingNanoFlannAdaptor{
public:
    typedef data::Embedding<scalar_type> embedding_type;
    EmbeddingNanoFlannAdaptor(const embedding_type& embedding):_embedding(embedding){}

    // Must return the number of data points
    inline size_t kdtree_get_point_count() const { return _embedding.numDataPoints(); }

    // Returns the distance between the vector "p1[0:size-1]" and the data point with index "idx_p2" stored in the class:
    inline scalar_type kdtree_distance(const scalar_type *p1, const size_t idx_p2,size_t /*size*/) const
    {
        scalar_type dist(0);
        for(int d = 0; d < _embedding.numDimensions(); ++d){
            const scalar_type v = p1[d]-_embedding.dataAt(idx_p2,d);
            dist += v*v;
        }
        return dist;
    }

    // Returns the dim'th component of the idx'th point in the class:
    // Since this is inlined and the "dim" argument is typically an immediate value, the
    //  "if/else's" are actually solved at compile time.
    inline scalar_type kdtree_get_pt(const size_t idx, int dim) const
    {
        return _embedding.dataAt(idx,dim);
    }

    // Optional bounding-box computation: return false to default to a standard bbox computation loop.
    //   Return true if the BBOX was already computed by the class and returned in "bb" so it can be avoided to redo it again.
    //   Look at bb.size() to find out the expected dimensionality (e.g. 2 or 3 for point clouds)
    template <class BBOX>
    bool kdtree_get_bbox(BBOX& /* bb */) const { return false; }

private:
    const embedding_type& _embedding;
};

    template <typename scalar_type>
    void computePrecisionRecall(data::PanelData<scalar_type>& high_dim_data, data::Embedding<scalar_type> embedding, unsigned int max_k, std::vector<scalar_type>& precision, std::vector<scalar_type>& recall){
        //nanoflann::KDTreeSingleIndexAdaptor<>
        PanelDataNanoFlannAdaptor<scalar_type> p_d_adaptor(high_dim_data);
        EmbeddingNanoFlannAdaptor<scalar_type> emb_adaptor(embedding);


        const unsigned int num_data_points = high_dim_data.numDataPoints();
        max_k = std::min(num_data_points,max_k);

        typedef nanoflann::KDTreeSingleIndexAdaptor<
                nanoflann::L2_Simple_Adaptor<scalar_type, PanelDataNanoFlannAdaptor<scalar_type> > ,
                PanelDataNanoFlannAdaptor<scalar_type>
                > panel_data_kd_tree_t;
        typedef nanoflann::KDTreeSingleIndexAdaptor<
                nanoflann::L2_Simple_Adaptor<scalar_type, EmbeddingNanoFlannAdaptor<scalar_type> > ,
                EmbeddingNanoFlannAdaptor<scalar_type>
                > embedding_kd_tree_t;

        panel_data_kd_tree_t p_d_index(high_dim_data.numDimensions(), p_d_adaptor);
        embedding_kd_tree_t  emb_index(embedding.numDimensions(), emb_adaptor);

        p_d_index.buildIndex();
        emb_index.buildIndex();
        std::vector<nanoflann::KNNResultSet<scalar_type> > p_d_result_set(num_data_points,nanoflann::KNNResultSet<scalar_type>(max_k+1));
        std::vector<std::vector<size_t>> p_d_ret_idx(num_data_points,std::vector<size_t>(max_k+1,0));
        std::vector<std::vector<scalar_type>>  p_d_out_dist_sqr(num_data_points,std::vector<scalar_type>(max_k+1,0));
        std::vector<nanoflann::KNNResultSet<scalar_type> > emb_result_set(num_data_points,nanoflann::KNNResultSet<scalar_type>(max_k+1));
        std::vector<std::vector<size_t>> emb_ret_idx(num_data_points,std::vector<size_t>(max_k+1,0));
        std::vector<std::vector<scalar_type>>  emb_out_dist_sqr(num_data_points,std::vector<scalar_type>(max_k+1,0));
        int i = 0;
#pragma omp parallel for
        for(i = 0; i < num_data_points; ++i){
            p_d_result_set[i].init(p_d_ret_idx[i].data(),p_d_out_dist_sqr[i].data());
            p_d_index.findNeighbors(p_d_result_set[i],&(high_dim_data.getData()[i*high_dim_data.numDimensions()]),nanoflann::SearchParams(10));
            emb_result_set[i].init(emb_ret_idx[i].data(),emb_out_dist_sqr[i].data());
            emb_index.findNeighbors(emb_result_set[i],&(embedding.getContainer()[i*embedding.numDimensions()]),nanoflann::SearchParams(10));
        }

        //check
        int wrong = 0;
        for(i = 0; i < num_data_points; ++i){
            std::unordered_set<size_t> p_d_set;
            std::unordered_set<size_t> emb_set;
            for(int j = 0; j < p_d_ret_idx[i].size(); ++j){
                p_d_set.insert(p_d_ret_idx[i][j]);
                emb_set.insert(emb_ret_idx[i][j]);
            }
            {
                double distance = p_d_out_dist_sqr[i][p_d_out_dist_sqr[i].size()-1];
                for(int j = 0; j < num_data_points; ++j){
                    double d = p_d_adaptor.kdtree_distance(&(high_dim_data.getData()[i*high_dim_data.numDimensions()]),j,0);
                    if(d < distance && p_d_set.find(j) == p_d_set.end()){
                        ++wrong;
                    }
                }
            }
            {
                double distance = emb_out_dist_sqr[i][emb_out_dist_sqr[i].size()-1];
                for(int j = 0; j < num_data_points; ++j){
                    double d = emb_adaptor.kdtree_distance(&(embedding.getContainer()[i*embedding.numDimensions()]),j,0);
                    if(d < distance && emb_set.find(j) == emb_set.end()){
                        ++wrong;
                    }
                }
            }
        }
        std::cout << "WRONG "<<wrong << std::endl;

        precision.clear();
        recall.clear();
        precision.resize(max_k,0);
        recall.resize(max_k,0);
        for(int i = 0; i < num_data_points; ++i){
            for(int k = 0; k < max_k; ++k){
                std::unordered_set<size_t> p_d_set;
                std::unordered_set<size_t> emb_set;
                for(int j = 0; j <= k; ++j){
                    p_d_set.insert(p_d_ret_idx[i][j]);
                    emb_set.insert(emb_ret_idx[i][j]);
                }
                int true_positive = 0;
                for(auto e: emb_set){
                    if(p_d_set.find(e) != p_d_set.end()){
                        ++true_positive;
                    }
                }
                scalar_type prec = scalar_type(true_positive)/(k+1);
                scalar_type rec = scalar_type(true_positive)/max_k;

                precision[k] += prec;
                recall[k] += rec;
            }
        }
        for(int k = 0; k < max_k; ++k){
            precision[k] /= num_data_points;
            recall[k] /= num_data_points;
        }

    }
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

		const int num_pics(std::atoi(argv[3]));
		const int num_dimensions(784);

		std::ifstream file_data(argv[1], std::ios::in|std::ios::binary);
		std::ifstream file_labels(argv[2], std::ios::in|std::ios::binary);
		if (!file_labels.is_open()){
			throw std::runtime_error("label file cannot be found");
		}
		if (!file_data.is_open()){
			throw std::runtime_error("data file cannot be found");
		}
		{//removing headers
			int32_t appo;
			file_labels.read((char*)&appo,4);
			file_labels.read((char*)&appo,4);
			file_data.read((char*)&appo,4);
			file_data.read((char*)&appo,4);
			file_data.read((char*)&appo,4);
			file_data.read((char*)&appo,4);
		}
		
		hdi::data::PanelData<scalar_type> panel_data;
		{//initializing panel data
			for(int j = 0; j < 28; ++j){
				for(int i = 0; i < 28; ++i){
					panel_data.addDimension(std::make_shared<hdi::data::PixelData>(hdi::data::PixelData(j,i,28,28)));
				}
			}
			panel_data.initialize();
		}
			

		std::vector<QImage> images;
		std::vector<std::vector<scalar_type> > input_data;
        std::vector<unsigned int> labels;

		{//reading data
			images.reserve(num_pics);
			input_data.reserve(num_pics);
			labels.reserve(num_pics);
			
			for(int i = 0; i < num_pics; ++i){
				unsigned char label;
				file_labels.read((char*)&label,1);
				labels.push_back(label);

				//still some pics to read for this digit
				input_data.push_back(std::vector<scalar_type>(num_dimensions));
				images.push_back(QImage(28,28,QImage::Format::Format_ARGB32));
				const int idx = int(input_data.size()-1);
				for(int i = 0; i < num_dimensions; ++i){
					unsigned char pixel;
					file_data.read((char*)&pixel,1);
					const scalar_type intensity(255.f - pixel);
					input_data[idx][i] = intensity;
					images[idx].setPixel(i%28,i/28,qRgb(intensity,intensity,intensity));
				}
			}

			{
				//moving a digit at the beginning digits of the vectors
				const int digit_to_be_moved = 1;
				int idx_to_be_swapped = 0;
				for(int i = 0; i < images.size(); ++i){
					if(labels[i] == digit_to_be_moved){
						std::swap(images[i],		images[idx_to_be_swapped]);
						std::swap(input_data[i],	input_data[idx_to_be_swapped]);
						std::swap(labels[i],		labels[idx_to_be_swapped]);
						++idx_to_be_swapped;
					}
				}
			}
			const int digit_to_be_selected = 4;
			for(int i = 0; i < images.size(); ++i){
				panel_data.addDataPoint(std::make_shared<hdi::data::ImageData>(hdi::data::ImageData(images[i])), input_data[i]);
				if(labels[i] == digit_to_be_selected){
					panel_data.getFlagsDataPoints()[i] = hdi::data::PanelData<scalar_type>::Selected;
				}
			}
		}



///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////

        int num_samples = sqrt(num_pics) * 3;
        //int num_samples = 411;
        hdi::utils::secureLogValue(&log,"#samples", num_samples);
        int projdim = 2;
        hdi::data::Embedding<scalar_type> embedding;
        embedding.resize(projdim,num_pics);

        hdi::data::Embedding<scalar_type> embedding_samples;
        embedding_samples.resize(projdim,num_samples);


        struct sampling_struct sampleinfo = {DEFAULT_VALUE};
          sampleinfo.numpoints = num_pics;
          sampleinfo.highdim = num_dimensions;
          sampleinfo.numsamples = num_samples;

        unsigned char hasid=0;
        unsigned char hascl=0;
        struct id_struct ids = {DEFAULT_VALUE};
        ids.retrieve = hasid ? GET_ID : CREATE_ID;

        std::vector<std::string> omg;
        for(int i = 0; i < num_pics; ++i){
            std::stringstream ss;
            ss << i;
            omg.push_back(ss.str());
        }

        char pippo []= "pippo\n";
        std::vector<char*> ids_values(num_pics);
        for(int i = 0; i < num_pics; ++i){
            ids_values[i] = (char*)(omg[i].c_str());
        }

        /*for(auto& s: ids_values){
            s = pippo;
        }
        */

        struct class_struct classes = {DEFAULT_VALUE};
            if (hascl) {
              classes.retrieve = GET_CLASS;
              classes.values = (char**) malloc (num_pics * sizeof(char*));
              classes.enumeration = (char**) calloc (maxclass_get(), sizeof(char*));
            }
    double time(0);
    char **sampleid = (char**) malloc (num_samples*sizeof(char*));

    {
        hdi::utils::ScopedTimer<double>timer(time);
        hdi::utils::secureLog(&log,"Sampling...");
        std::vector<scalar_type> sampled_data(num_samples*num_dimensions,0);
        sampling_execute(panel_data.getData().data(), ids_values.data(), &sampleinfo, sampled_data.data(), sampleid);

        hdi::utils::secureLog(&log,"IDMap...");
        struct idmap_struct idmapinfo = {DEFAULT_VALUE};
          idmapinfo.numpoints = num_samples;
          idmapinfo.highdim = num_dimensions;
          idmapinfo.projdim = projdim;
          idmapinfo.numiterations = 100;
        idmap_execute(sampled_data.data(), sampleid, &idmapinfo, embedding_samples.getContainer().data());


        hdi::utils::secureLog(&log,"LAMP...");
        struct lamp_struct lampinfo = {DEFAULT_VALUE};
          lampinfo.numpoints = num_pics;
          lampinfo.numsamples = num_samples;
          lampinfo.highdim = num_dimensions;
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
        viewer_full.resize(900,900);
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
        viewer_fix_color.resize(900,900);
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
            viewer_pivot.resize(900,900);
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
        for(int i = 0; i < precision.size(); ++i){
            std::cout << precision[i] << "\t" << recall[i] << std::endl;
        }

		return app.exec();
	}
	catch(std::logic_error& ex){ std::cout << "Logic error: " << ex.what();}
	catch(std::runtime_error& ex){ std::cout << "Runtime error: " << ex.what();}
	catch(...){ std::cout << "An unknown error occurred";}
}
