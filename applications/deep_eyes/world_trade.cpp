
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
        /*std::shared_ptr<hdi::viz::MultipleImageView> image_view(new hdi::viz::MultipleImageView());
        embedder->addView(image_view);
        image_view->_text_data_as_os_path = true;
        image_view->show();
*/
        std::shared_ptr<hdi::viz::MultipleHeatmapsView> heatmaps_view(new hdi::viz::MultipleHeatmapsView());
        embedder->addView(heatmaps_view);
        heatmaps_view->setAuxData(embedder->getPanelData().numDimensions(),embedder->getPanelData().getData().data());
        heatmaps_view->show();

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
        if(argc != 2){
            hdi::utils::secureLog(&log,"Not enough input parameters...");
            return 1;
        }

        std::ifstream file(argv[1], std::ios::in);
        if (!file.is_open()){
            throw std::runtime_error("file cannot be opened");
        }

        const int num_points(80);
        hdi::utils::secureLogValue(&log,"#points",num_points);
        hdi::dr::HDJointProbabilityGenerator<scalar_type>::sparse_scalar_matrix_type probability;
        probability.resize(num_points);

        for(int i = 0; i < num_points; ++i){
            std::string line;
            std::getline(file,line);
            std::stringstream line_stream(line);
            std::string cell;
            int i_dim = 0;
            while(std::getline(line_stream,cell,',')){
                probability[i][i_dim] = std::atof(cell.c_str());
                ++i_dim;
            }
        }

        scalar_type sum = 0;
        for(int j = 0; j < num_points; ++j){
            for(int i = 0; i < num_points; ++i){
                sum += probability[i][j];
            }
        }

        sum /= num_points;
        for(int j = 0; j < num_points; ++j){
            for(int i = 0; i < num_points; ++i){
                probability[i][j] /= sum;
            }
        }


        hdi::utils::imageFromSparseMatrix(probability).save("neurons_P.png");

        hdi::data::Embedding<scalar_type> embedding;
        hdi::dr::WeightedTSNE<scalar_type> tSNE;
        hdi::dr::WeightedTSNE<scalar_type>::Parameters tSNE_params;
        tSNE.setLogger(&log);
        tSNE_params._seed = -1;
        tSNE_params._minimum_gain = 0;
        tSNE_params._eta = 10;
        tSNE.setTheta(0.1);
        tSNE.initializeWithJointProbabilityDistribution(probability,&embedding,tSNE_params);

        hdi::viz::ScatterplotCanvas viewer;
        viewer.setBackgroundColors(qRgb(240,240,240),qRgb(200,200,200));
        viewer.setSelectionColor(qRgb(50,50,50));
        viewer.resize(500,500);
        viewer.show();

        std::vector<uint32_t> flags(num_points,0);
        std::vector<scalar_type> colors(num_points*3,0);
        for(int i = 0; i < num_points; ++i){
            colors[i*3+0] = scalar_type(i)/num_points;
            colors[i*3+1] = 0;
            colors[i*3+2] = 0;
        }


        hdi::viz::ScatterplotDrawerUsedDefinedColors drawer;
        drawer.initialize(viewer.context());
        drawer.setData(embedding.getContainer().data(), colors.data(), flags.data(), num_points);
        drawer.setAlpha(0.7);
        drawer.setPointSize(10);
        viewer.addDrawer(&drawer);
        viewer.setBackgroundColors(qRgb(255,255,255),qRgb(255,255,255));


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
