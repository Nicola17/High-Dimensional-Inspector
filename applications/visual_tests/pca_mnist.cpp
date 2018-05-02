#include <exception>
#include <QApplication>
#include "hdi/utils/cout_log.h"
#include "hdi/utils/assert_by_exception.h"
#include "hdi/utils/log_helper_functions.h"
#include <iostream>
#include <hdi/data/panel_data.h>
#include <QImage>
#include "hdi/data/pixel_data.h"
#include "hdi/data/empty_data.h"
#include <QColor>
#include <QPainter>
#include <QPixmap>
#include <QPen>
#include "hdi/utils/Eigen/Eigen"
#include <hdi/utils/Eigen/Dense>
#include <hdi/utils/Eigen/Eigenvalues>
#include "hdi/utils/dataset_utils.h"
#include "hdi/data/embedding.h"
#include "hdi/dimensionality_reduction/sparse_tsne_user_def_probabilities.h"
#include "hdi/dimensionality_reduction/hd_joint_probability_generator.h"
#include "hdi/visualization/scatterplot_canvas_qobj.h"
#include "hdi/visualization/scatterplot_drawer_user_defined_colors.h"
#include "hdi/visualization/multiple_image_view_qobj.h"
#include "hdi/visualization/controller_embedding_selection_qobj.h"
#include "hdi/data/image_data.h"

namespace hdi{

  template <typename scalar_type>
  void computeCovarianceMatrix(const data::PanelData<scalar_type>& panel_data, Eigen::MatrixXd& cov_mat ){
    typedef data::PanelData<scalar_type> panel_data_type;
    const unsigned int num_dimensions = panel_data.numDimensions();
    const unsigned int num_data_points = panel_data.numDataPoints();
    cov_mat = Eigen::MatrixXd::Constant(num_dimensions,num_dimensions,0);

    std::vector<scalar_type> mean;
    hdi::data::computeMean(panel_data,mean);

    auto& data = panel_data.getData();
    for(int p = 0; p < num_data_points; ++p){
      for(int d1 = 0; d1 < num_dimensions; ++d1){
        for(int d2 = 0; d2 < num_dimensions; ++d2){
          cov_mat(d1,d2) += (data[p*num_dimensions+d1]-mean[d1])*(data[p*num_dimensions+d2]-mean[d2]);
        }
      }
    }

    for(int d1 = 0; d1 < num_dimensions; ++d1){
      for(int d2 = 0; d2 < num_dimensions; ++d2){
        cov_mat(d1,d2) /= num_data_points;
      }
    }
  }

  template <typename scalar_type, typename map_type>
  void localizedCovarianceMatrix(const data::PanelData<scalar_type>& panel_data, uint32_t id, const map_type& neighborhood, Eigen::MatrixXd& cov_mat){
    typedef data::PanelData<scalar_type> panel_data_type;
    const unsigned int num_dimensions = panel_data.numDimensions();
    cov_mat = Eigen::MatrixXd::Constant(num_dimensions,num_dimensions,0);

    auto& data = panel_data.getData();
    double total_weight = 0;
    for(auto n: neighborhood){
      for(int d1 = 0; d1 < num_dimensions; ++d1){
        for(int d2 = 0; d2 < num_dimensions; ++d2){
          cov_mat(d1,d2) +=   (data[n.first*num_dimensions+d1]-data[id*num_dimensions+d1])*
                    (data[n.first*num_dimensions+d2]-data[id*num_dimensions+d1]);
          total_weight += n.second;
        }
      }
    }

    for(int d1 = 0; d1 < num_dimensions; ++d1){
      for(int d2 = 0; d2 < num_dimensions; ++d2){
        cov_mat(d1,d2) /= total_weight;
      }
    }
  }





}

int main(int argc, char *argv[]){
  try{
    typedef float scalar_type;
    QApplication app(argc, argv);
    QIcon icon;
    icon.addFile(":/brick32.png");
    icon.addFile(":/brick128.png");
    app.setWindowIcon(icon);

    hdi::utils::CoutLog log;

///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////
    if(argc != 4){
      hdi::utils::secureLog(&log,"Wrong number of parameters...");
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

    std::vector<unsigned int>labels;
    hdi::data::PanelData<scalar_type> panel_data;
    hdi::utils::IO::loadMNIST(panel_data,labels,argv[1],argv[2],std::atoi(argv[3]));

    hdi::utils::secureLog(&log,"Data loaded...");

///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////

    const unsigned int num_dimensions = panel_data.numDimensions();
    const unsigned int num_pics = panel_data.numDataPoints();

    hdi::dr::HDJointProbabilityGenerator<scalar_type>::sparse_scalar_matrix_type probability;
    hdi::dr::HDJointProbabilityGenerator<scalar_type> prob_gen;
    prob_gen.setLogger(&log);
    prob_gen.computeJointProbabilityDistribution(panel_data.getData().data(),num_dimensions,num_pics,probability);
    prob_gen.statistics().log(&log);


    std::vector<scalar_type> intrinsic_dim(num_pics);

    for(int d = 0; d < num_pics; ++d){
      Eigen::MatrixXd cov_mat;
      hdi::localizedCovarianceMatrix(panel_data,d,probability[d],cov_mat);
      hdi::utils::secureLogValue(&log,"A",d);
      Eigen::EigenSolver<Eigen::MatrixXd> eigensolver(cov_mat);
      hdi::utils::secureLogValue(&log,"B",d);
      double max = 0;
      int max_id = 0;
      for(int i = 0; i < eigensolver.eigenvalues().rows(); ++i){
        if(eigensolver.eigenvalues()(i,0).real() > max){
          max = eigensolver.eigenvalues()(i,0).real();
          max_id = i;
        }
      }
      double intr_dim = 0;
      for(int i = 0; i < eigensolver.eigenvalues().rows(); ++i){
        intr_dim += eigensolver.eigenvalues()(i,0).real()/max;
      }
      intrinsic_dim[d] = intr_dim;


      auto data_ptr = dynamic_cast<hdi::data::ImageData*>(panel_data.getDataPoints()[d].get());
      if(data_ptr != nullptr){
        data_ptr->image().save(QString("%1_%2.png").arg(d).arg(intr_dim));

        QImage new_img = data_ptr->image();
        for(int i = 0; i < 28*28; ++i){
          double v  = eigensolver.eigenvectors()(i,max_id).real()*3;
          new_img.setPixel(i%28,i/28,qRgb(255-255*v,255,255));
          /*
          if(eigensolver.eigenvectors()(i,max_id).real()>0){
            new_img.setPixel(i%28,i/28,qRgb(255,255-255*eigensolver.eigenvectors()(i,max_id).real()*3,255-255*eigensolver.eigenvectors()(i,max_id).real()));
          }else{
            new_img.setPixel(i%28,i/28,qRgb(255-255*eigensolver.eigenvectors()(i,max_id).real(),255-255*eigensolver.eigenvectors()(i,max_id).real(),255));
          }*/
        }
        new_img.save(QString("%1_%2_vect.png").arg(d).arg(intr_dim));
      }

      hdi::utils::secureLogValue(&log,"max",max);
      hdi::utils::secureLogValue(&log,"intr_dim",intr_dim);
      hdi::utils::secureLogValue(&log,"C",d);
    }

    hdi::utils::secureLogVectorWithStats(&log,"Intrinsic Dim",intrinsic_dim);



    scalar_type theta = 0.5;

    hdi::data::Embedding<scalar_type> embedding;
    hdi::dr::SparseTSNEUserDefProbabilities<scalar_type> tSNE;
    hdi::dr::SparseTSNEUserDefProbabilities<scalar_type>::Parameters tSNE_params;

    tSNE.setLogger(&log);
    tSNE_params._seed = 10;
    tSNE.setTheta(theta);
    tSNE.initializeWithJointProbabilityDistribution(probability,&embedding,tSNE_params);

    hdi::viz::ScatterplotCanvas viewer;
    viewer.setBackgroundColors(qRgb(240,240,240),qRgb(200,200,200));
    viewer.setSelectionColor(qRgb(50,50,50));
    viewer.resize(500,500);
    viewer.show();

    hdi::viz::MultipleImageView image_view;
    image_view.setPanelData(&panel_data);
    image_view.show();
    image_view.updateView();

    hdi::viz::ControllerSelectionEmbedding selection_controller;
    selection_controller.setActors(&panel_data,&embedding,&viewer);
    selection_controller.setLogger(&log);
    selection_controller.initialize();
    selection_controller.addView(&image_view);

    std::vector<uint32_t> flags(panel_data.numDataPoints(),0);
    std::vector<float> embedding_colors_for_viz(panel_data.numDataPoints()*3,0);

    for(int i = 0; i < panel_data.numDataPoints(); ++i){
      int label = labels[i];
      auto color = color_per_digit[label];
      embedding_colors_for_viz[i*3+0] = color.redF();
      embedding_colors_for_viz[i*3+1] = color.greenF();
      embedding_colors_for_viz[i*3+2] = color.blueF();
    }

    hdi::viz::ScatterplotDrawerUsedDefinedColors drawer;
    drawer.initialize(viewer.context());
    drawer.setData(embedding.getContainer().data(), embedding_colors_for_viz.data(), flags.data(), panel_data.numDataPoints());
    drawer.setAlpha(0.15);
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

      if((iter%10) == 0){
        viewer.updateGL();
        hdi::utils::secureLogValue(&log,"Iter",iter);
      }
      QApplication::processEvents();
      ++iter;
    }

    return app.exec();


    return 0;
  }
  catch(std::logic_error& ex){ std::cout << "Logic error: " << ex.what();}
  catch(std::runtime_error& ex){ std::cout << "Runtime error: " << ex.what();}
  catch(...){ std::cout << "An unknown error occurred";}
}
