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
#include "hdi/utils/visual_utils.h"
#include "hdi/utils/graph_algorithms.h"
#include "hdi/data/embedding.h"
#include "hdi/visualization/controller_embedding_selection_qobj.h"
#include <QDir>
#include "hdi/utils/math_utils.h"


bool checkNumInString(int num, const std::string& string){
    for(int i = 0; i < string.size(); ++i){
        if(string[i]-48 == num){
            return true;
        }
    }
    return false;
}

int main(int argc, char *argv[]){
	try{
		typedef float scalar_type;
		QApplication app(argc, argv);

		hdi::utils::CoutLog log;

///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////
///
/// dataset_utils.h in develop
///
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
		std::vector<unsigned char> labels;

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

        hdi::dr::HierarchicalSNE<scalar_type> hsne;
		{//initializing
            hsne.setLogger(&log);
            hsne.setDimensionality(num_dimensions);
			auto& data = panel_data.getData();
		}
		//Initialization
        hdi::dr::HierarchicalSNE<scalar_type>::Parameters params;
        params._seed = -2;
        params._monte_carlo_sampling = true;

        hsne.initialize(panel_data.getData().data(),panel_data.numDataPoints(),params);
        hsne.statistics().log(&log);

        const int desired_scale = 3;
		for(int s = 0; s < desired_scale; ++s){
            hsne.addScale();
            hdi::utils::secureLog(&log,"--------------\n");
		}

///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////
        typedef hdi::dr::HierarchicalSNE<scalar_type>::ClusterTree cluster_tree_type;
        typedef cluster_tree_type::cluster_type cluster_type;

        cluster_tree_type cluster_tree(hsne);
        cluster_tree.setLogger(&log);

        //In the top_level I add a Cluster for 1-7-9-4, 2-3-5-8 and 0... 6 are unclustered
        {
            cluster_type cluster;
            cluster.parent_id() = cluster_type::NULL_LINK;
            cluster.id() = 0;
            cluster.notes() = "1-4-7-9";

            int scale = hsne.hierarchy().size()-1;
            for(int i = 0; i < hsne.scale(scale)._landmark_to_original_data_idx.size(); ++i){
                auto label = labels[hsne.scale(scale)._landmark_to_original_data_idx[i]];
                if(label == 1 || label == 4 || label == 7 || label == 9){
                    cluster.landmarks().insert(i);
                }
            }
            cluster_tree.addCluster(scale,cluster);
        }
        {
            cluster_type cluster;
            cluster.parent_id() = cluster_type::NULL_LINK;
            cluster.id() = 5;
            cluster.notes() = "2-3-5-8";

            int scale = hsne.hierarchy().size()-1;
            for(int i = 0; i < hsne.scale(scale)._landmark_to_original_data_idx.size(); ++i){
                auto label = labels[hsne.scale(scale)._landmark_to_original_data_idx[i]];
                if(label == 2 || label == 3 || label == 5 || label == 8){
                    cluster.landmarks().insert(i);
                }
            }
            cluster_tree.addCluster(scale,cluster);
        }

        {
            cluster_type cluster;
            cluster.parent_id() = cluster_type::NULL_LINK;
            cluster.id() = 10;
            cluster.notes() = "0";

            int scale = hsne.hierarchy().size()-1;
            for(int i = 0; i < hsne.scale(scale)._landmark_to_original_data_idx.size(); ++i){
                auto label = labels[hsne.scale(scale)._landmark_to_original_data_idx[i]];
                if(label == 0){
                    cluster.landmarks().insert(i);
                }
            }
            cluster_tree.addCluster(scale,cluster);
        }

        ///////////////////////////////////////////
        // 1-7-9-4 becomes 7-9 , 4 and unclusterd
        // 2-3-5-8 becomes 3-5-8 and unclusterd

        {
            cluster_type cluster;
            cluster.parent_id() = 0;
            cluster.id() = 0;
            cluster.notes() = "7-9";

            int scale = hsne.hierarchy().size()-2;
            for(int i = 0; i < hsne.scale(scale)._landmark_to_original_data_idx.size(); ++i){
                auto label = labels[hsne.scale(scale)._landmark_to_original_data_idx[i]];
                if(label == 7 || label == 9){
                    cluster.landmarks().insert(i);
                }
            }
            cluster_tree.addCluster(scale,cluster);
        }
        {
            cluster_type cluster;
            cluster.parent_id() = 0;
            cluster.id() = 1;
            cluster.notes() = "4";

            int scale = hsne.hierarchy().size()-2;
            for(int i = 0; i < hsne.scale(scale)._landmark_to_original_data_idx.size(); ++i){
                auto label = labels[hsne.scale(scale)._landmark_to_original_data_idx[i]];
                if(label == 4){
                    cluster.landmarks().insert(i);
                }
            }
            cluster_tree.addCluster(scale,cluster);
        }
        {
            cluster_type cluster;
            cluster.parent_id() = 5;
            cluster.id() = 2;
            cluster.notes() = "3-5-8";

            int scale = hsne.hierarchy().size()-2;
            for(int i = 0; i < hsne.scale(scale)._landmark_to_original_data_idx.size(); ++i){
                auto label = labels[hsne.scale(scale)._landmark_to_original_data_idx[i]];
                if(label == 3 || label == 5 || label == 8){
                    cluster.landmarks().insert(i);
                }
            }
            cluster_tree.addCluster(scale,cluster);
        }


        ///////////////////////////////////////////
        // 7-9 becomes 7 and 9
        // 3-5-8 becomes 3,5 and 8 unclusterd

        {
            cluster_type cluster;
            cluster.parent_id() = 0;
            cluster.id() = 0;
            cluster.notes() = "7";

            int scale = hsne.hierarchy().size()-3;
            for(int i = 0; i < hsne.scale(scale)._landmark_to_original_data_idx.size(); ++i){
                auto label = labels[hsne.scale(scale)._landmark_to_original_data_idx[i]];
                if(label == 7){
                    cluster.landmarks().insert(i);
                }
            }
            cluster_tree.addCluster(scale,cluster);
        }

        {
            cluster_type cluster;
            cluster.parent_id() = 0;
            cluster.id() = 1;
            cluster.notes() = "9";

            int scale = hsne.hierarchy().size()-3;
            for(int i = 0; i < hsne.scale(scale)._landmark_to_original_data_idx.size(); ++i){
                auto label = labels[hsne.scale(scale)._landmark_to_original_data_idx[i]];
                if(label == 9){
                    cluster.landmarks().insert(i);
                }
            }
            cluster_tree.addCluster(scale,cluster);
        }

        {
            cluster_type cluster;
            cluster.parent_id() = 2;
            cluster.id() = 2;
            cluster.notes() = "3";

            int scale = hsne.hierarchy().size()-3;
            for(int i = 0; i < hsne.scale(scale)._landmark_to_original_data_idx.size(); ++i){
                auto label = labels[hsne.scale(scale)._landmark_to_original_data_idx[i]];
                if(label == 3){
                    cluster.landmarks().insert(i);
                }
            }
            cluster_tree.addCluster(scale,cluster);
        }
        {
            cluster_type cluster;
            cluster.parent_id() = 2;
            cluster.id() = 3;
            cluster.notes() = "5";

            int scale = hsne.hierarchy().size()-3;
            for(int i = 0; i < hsne.scale(scale)._landmark_to_original_data_idx.size(); ++i){
                auto label = labels[hsne.scale(scale)._landmark_to_original_data_idx[i]];
                if(label == 5){
                    cluster.landmarks().insert(i);
                }
            }
            cluster_tree.addCluster(scale,cluster);
        }

        //////////////////////////////////////////////////

        {
            //cluster_tree.checkCluterConsistency(hsne,hsne.hierarchy().size()-1,0);
            //cluster_tree.checkCluterConsistency(hsne,hsne.hierarchy().size()-2,0);
            hdi::utils::secureLog(&log,"-------------------------------------------------------------");
            bool res = cluster_tree.checkTreeConsistency(hsne);
            if(res){
                hdi::utils::secureLog(&log,"VALID TREE!");
            }else{
                hdi::utils::secureLog(&log,"INVALID TREE!");
            }
            hdi::utils::secureLog(&log,"-------------------------------------------------------------");
       }

        //////////////////////////////////////////////////
        // Testing for few points

        {
            const int num_pnts_to_test = 30;

            std::default_random_engine generator;
            std::uniform_int_distribution<int> distribution(0,num_pics);

            for(int i = 0; i < num_pnts_to_test; ++i){
                int dp = distribution(generator);
                std::tuple<uint32_t,int32_t,float> res;
                cluster_tree.computePointToClusterAssociation(hsne,dp,res);

                std::stringstream ss;
                if(std::get<1>(res) != -1){
                    ss << int(labels[dp]) << " : " << cluster_tree.cluster(std::get<0>(res),std::get<1>(res)).notes() << " : " << std::get<2>(res);
                }else{
                    ss << int(labels[dp]) << " : unclustered : " << std::get<2>(res);
                }
                hdi::utils::secureLog(&log,ss.str());
            }
        }

        //////////////////////////////////////////////////
        // Testing for the whole dataset

        {
            std::vector<std::tuple<uint32_t,int32_t,scalar_type>> res;
            cluster_tree.computePointsToClusterAssociation(hsne,res);

            double valid = 0;
            double total = 0;
            for(int i = 0; i < res.size(); ++i){
                if(std::get<1>(res[i]) != -1){
                    if(checkNumInString(labels[i],cluster_tree.cluster(std::get<0>(res[i]),std::get<1>(res[i])).notes())){
                        ++valid;
                    }
                    ++total;
                }
            }
            hdi::utils::secureLogValue(&log,"Accuracy",valid/total);

        }


        hdi::utils::secureLog(&log,"Done!");
        //return app.exec();
	}
	catch(std::logic_error& ex){ std::cout << "Logic error: " << ex.what();}
	catch(std::runtime_error& ex){ std::cout << "Runtime error: " << ex.what();}
	catch(...){ std::cout << "An unknown error occurred";}
}
