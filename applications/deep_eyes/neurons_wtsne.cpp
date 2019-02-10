
#include "hdi/utils/cout_log.h"
#include "hdi/utils/log_helper_functions.h"
#include "hdi/dimensionality_reduction/tsne.h"
#include "hdi/dimensionality_reduction/sparse_tsne_user_def_probabilities.h"
#include "hdi/dimensionality_reduction/wtsne.h"
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
#include "hdi/dimensionality_reduction/hd_joint_probability_generator.h"
#include "hdi/utils/visual_utils.h"


class MyInterfaceInitializer: public hdi::analytics::MultiscaleEmbedderSystem::AbstractInterfaceInitializer{
public:
    virtual ~MyInterfaceInitializer(){}
    virtual void initializeStandardVisualization(embedder_type* embedder, const std::vector<unsigned int>& idxes_to_orig_data){
        std::shared_ptr<hdi::viz::ScatterplotDrawerFixedColor> drawer(new hdi::viz::ScatterplotDrawerFixedColor());
        embedder->addUserDefinedDrawer(drawer);
        drawer->setData(embedder->getEmbedding().getContainer().data(), embedder->getPanelData().getFlagsDataPoints().data(), embedder->getPanelData().numDataPoints());
        drawer->setAlpha(0.5);
        drawer->setPointSize(10);
    }
    virtual void initializeInfluenceVisualization(embedder_type* embedder, const std::vector<unsigned int>& idxes_to_orig_data, scalar_type* influence){
        std::shared_ptr<hdi::viz::ScatterplotDrawerScalarAttribute> drawer(new hdi::viz::ScatterplotDrawerScalarAttribute());
        embedder->addAreaOfInfluenceDrawer(drawer);
        drawer->setData(embedder->getEmbedding().getContainer().data(), influence, embedder->getPanelData().getFlagsDataPoints().data(), embedder->getPanelData().numDataPoints());
        drawer->updateLimitsFromData();
        drawer->setPointSize(15);
        drawer->setAlpha(0.8);
    }
    virtual void initializeSelectionVisualization(embedder_type* embedder, const std::vector<unsigned int>& idxes_to_orig_data, scalar_type* selection){
        std::shared_ptr<hdi::viz::ScatterplotDrawerFixedColor> drawer(new hdi::viz::ScatterplotDrawerFixedColor());
        embedder->addSelectionDrawer(drawer);
        drawer->setData(embedder->getEmbedding().getContainer().data(), embedder->getPanelData().getFlagsDataPoints().data(), embedder->getPanelData().numDataPoints());
        drawer->setPointSize(7.5);
        drawer->setColor(qRgb(255,0,150));
    }
    virtual void updateSelection(embedder_type* embedder, scalar_type* selection){

    }

    virtual void dataPointSelectionChanged(const std::vector<scalar_type>& selection){}
};

int main(int argc, char *argv[]){
    try{
        typedef float scalar_type;
        QApplication app(argc, argv);

        hdi::utils::CoutLog log;

///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////
        if(argc != 3){
            hdi::utils::secureLog(&log,"Not enough input parameters...");
            return 1;
        }

        std::ifstream file_pictures(argv[1], std::ios::in);
        std::ifstream file_hd_space(argv[2], std::ios::in|std::ios::binary|std::ios::ate);
        if (!file_pictures.is_open()){
            throw std::runtime_error("label file cannot be found");
        }
        if (!file_hd_space.is_open()){
            throw std::runtime_error("data file cannot be found");
        }

        //hdi::analytics::MultiscaleEmbedderSystem multiscale_embedder;
        //multiscale_embedder.setLogger(&log);
        hdi::data::PanelData<scalar_type> panel_data;

        std::string line;
        std::getline(file_pictures, line);

        const int num_pics(std::atoi(line.c_str()));
        const int num_dimensions(file_hd_space.tellg()/sizeof(float)/num_pics);

        hdi::utils::secureLogValue(&log,"#pics",num_pics);
        hdi::utils::secureLogValue(&log,"#dimensions",num_dimensions);

        file_hd_space.seekg (0, std::ios::beg);
        std::vector<float> raw_data(num_pics*num_dimensions);
        file_hd_space.read(reinterpret_cast<char*>(raw_data.data()), sizeof(float) * num_dimensions * num_pics);

        hdi::utils::secureLog(&log,"Reading data...");
        {//initializing panel data
            for(int i = 0; i < num_pics; ++i){
                panel_data.addDimension(std::make_shared<hdi::data::EmptyData>(hdi::data::EmptyData()));
            }
            panel_data.initialize();
        }

        std::vector<scalar_type> degree_centrality(num_dimensions,0);
        double sparsity = 0;
        {//reading data
            for(int i = 0; i < num_dimensions; ++i){
                std::vector<scalar_type> data(num_pics);
                for(int j = 0; j < num_pics; ++j){
                    if(raw_data[j*num_dimensions + i] == 0){
                        ++sparsity;
                    }
                    if(raw_data[j*num_dimensions + i] == 0){
                        ++degree_centrality[i];
                    }
                    data[j] = raw_data[j*num_dimensions + i];
                }
                panel_data.addDataPoint(std::make_shared<hdi::data::EmptyData>(hdi::data::EmptyData()), data);
            }
        }
        sparsity /= (num_dimensions*num_pics);

        hdi::utils::secureLog(&log,"done!");
        hdi::utils::secureLogValue(&log,"Sparsity", sparsity);
        hdi::utils::secureLogVectorStats(&log,"Degree centrality",degree_centrality);

///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////

        hdi::dr::HDJointProbabilityGenerator<scalar_type>::sparse_scalar_matrix_type probability;
        hdi::dr::HDJointProbabilityGenerator<scalar_type> prob_gen;
        prob_gen.setLogger(&log);
        prob_gen.computeJointProbabilityDistribution(panel_data.getData().data(),num_pics,num_dimensions,probability);

        prob_gen.statistics().log(&log);

        hdi::utils::imageFromSparseMatrix(probability).save("neurons_P.png");

        hdi::data::Embedding<scalar_type> embedding;
        hdi::dr::WeightedTSNE<scalar_type> tSNE;
        hdi::dr::WeightedTSNE<scalar_type>::Parameters tSNE_params;
        tSNE.setLogger(&log);
        tSNE_params._seed = 1;
        tSNE.setTheta(0.3);
        tSNE.initializeWithJointProbabilityDistribution(probability,&embedding,tSNE_params);

        hdi::viz::ScatterplotCanvas viewer;
        viewer.setBackgroundColors(qRgb(240,240,240),qRgb(200,200,200));
        viewer.setSelectionColor(qRgb(50,50,50));
        viewer.resize(500,500);
        viewer.show();

        std::vector<uint32_t> flags(panel_data.numDataPoints(),0);

        hdi::viz::ScatterplotDrawerFixedColor drawer;
        drawer.initialize(viewer.context());
        drawer.setData(embedding.getContainer().data(), flags.data(), panel_data.numDataPoints());
        drawer.setAlpha(0.7);
        drawer.setPointSize(5);
        viewer.addDrawer(&drawer);

        int iter = 0;
        while(true){
            tSNE.doAnIteration();

            {//limits
                std::vector<scalar_type> limits;
                embedding.computeEmbeddingBBox(limits,0.25);
                auto tr = QVector2D(limits[1],limits[3]);
                auto bl = QVector2D(limits[0],limits[2]);
                viewer.setTopRightCoordinates(tr);
                viewer.setBottomLeftCoordinates(bl);
            }

            if((iter%1) == 0){
                viewer.updateGL();
                hdi::utils::secureLogValue(&log,"Iter",iter);
            }
            QApplication::processEvents();
            ++iter;
        }

        return app.exec();
    }
    catch(std::logic_error& ex){ std::cout << "Logic error: " << ex.what();}
    catch(std::runtime_error& ex){ std::cout << "Runtime error: " << ex.what();}
    catch(...){ std::cout << "An unknown error occurred";}
}
