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
#include <QIcon>

namespace hdi{

  template <typename scalar_type>
  void computeCovarianceMatrix(const data::PanelData<scalar_type>& panel_data, Eigen::MatrixXd& cov_mat ){
    typedef data::PanelData<scalar_type> panel_data_type;
    const unsigned int num_dimensions = panel_data.numDimensions();
    const unsigned int num_data_points = panel_data.numDataPoints();
    cov_mat = Eigen::MatrixXd::Constant(num_dimensions,num_dimensions,0);

    std::vector<scalar_type> mean;
    hdi::data::computeMean(panel_data,mean);

    auto root_weights = panel_data.getProperty("weights");
    scalar_type total_weight = 0;
    for(auto& w: root_weights){
       //w = std::sqrt(w);
       total_weight += w;
    }

    auto& data = panel_data.getData();
    for(int p = 0; p < num_data_points; ++p){
      for(int d1 = 0; d1 < num_dimensions; ++d1){
        for(int d2 = 0; d2 < num_dimensions; ++d2){
          cov_mat(d1,d2) += (data[p*num_dimensions+d1]-mean[d1])*(data[p*num_dimensions+d2]-mean[d2])*root_weights[p];
        }
      }
    }

    for(int d1 = 0; d1 < num_dimensions; ++d1){
      for(int d2 = 0; d2 < num_dimensions; ++d2){
        //cov_mat(d1,d2) /= num_data_points;
        cov_mat(d1,d2) /= total_weight;
      }
    }


  }

//////////////////////////////////////

  template <typename scalar_type>
  void panelDataFromImage(const QImage& image, data::PanelData<scalar_type>& panel_data){

    typedef data::PanelData<scalar_type> panel_data_type;
    panel_data.addDimension(std::make_shared<data::EmptyData>());
    panel_data.addDimension(std::make_shared<data::EmptyData>());
    panel_data.initialize();
    panel_data.requestProperty("weights");
    auto& weights = panel_data.getProperty("weights");

    for(int j = 0; j < image.height(); ++j){
      for(int i = 0; i < image.width(); ++i){
        auto l = QColor(image.pixel(i,j)).redF();

        if(l < 1){
          std::vector<scalar_type> hd(2,0);
          hd[0] = i;
          hd[1] = j;
          panel_data.addDataPoint(std::make_shared<data::PixelData>(i,j),hd);
          weights[panel_data.numDataPoints()-1] = std::pow(1-l,2);
        }
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
    if(argc != 3){
      hdi::utils::secureLog(&log,"Wrong number of parameters...");
      return 1;
    }

    QImage image(argv[1]);
    hdi::data::PanelData<scalar_type> panel_data;
    hdi::panelDataFromImage(image,panel_data);

    std::vector<scalar_type> mean;
    hdi::data::computeMean(panel_data,mean);

    Eigen::MatrixXd cov_matrix;
    hdi::computeCovarianceMatrix(panel_data,cov_matrix);
    std::cout << std::endl << std::endl << cov_matrix << std::endl << std::endl ;


    Eigen::EigenSolver<Eigen::MatrixXd> eigensolver(cov_matrix);
    std::cout << "\nEigenValues\n" << eigensolver.eigenvalues()<< std::endl;
    std::cout << "\nEigenVectors\n" << eigensolver.eigenvectors() << std::endl;
    std::cout << eigensolver.eigenvalues()(0,0).real();
    double a = eigensolver.eigenvalues()(0,0).real();

    QPixmap pixmap(QPixmap::fromImage(image));
    QPainter painter;
    QPen pen;
    pen.setWidth(10);
    pen.setColor(qRgb(200,50,50));
    painter.begin(&pixmap);
    painter.setPen(pen);
    painter.drawPoint(mean[0],mean[1]);

    pen.setWidth(2);
    painter.setPen(pen);

    painter.drawLine(
          QPointF(mean[0]+eigensolver.eigenvectors()(0,0).real()*std::sqrt(eigensolver.eigenvalues()(0,0).real()),
              mean[1]+eigensolver.eigenvectors()(1,0).real()*std::sqrt(eigensolver.eigenvalues()(0,0).real())),
          QPointF(mean[0]-eigensolver.eigenvectors()(0,0).real()*std::sqrt(eigensolver.eigenvalues()(0,0).real()),
              mean[1]-eigensolver.eigenvectors()(1,0).real()*std::sqrt(eigensolver.eigenvalues()(0,0).real()))
        );

    painter.drawLine(
          QPointF(mean[0]+eigensolver.eigenvectors()(0,1).real()*std::sqrt(eigensolver.eigenvalues()(1,0).real()),
              mean[1]+eigensolver.eigenvectors()(1,1).real()*std::sqrt(eigensolver.eigenvalues()(1,0).real())),
          QPointF(mean[0]-eigensolver.eigenvectors()(0,1).real()*std::sqrt(eigensolver.eigenvalues()(1,0).real()),
              mean[1]-eigensolver.eigenvectors()(1,1).real()*std::sqrt(eigensolver.eigenvalues()(1,0).real()))
        );

    image = pixmap.toImage();



    image.save(argv[2]);

    return 0;
  }
  catch(std::logic_error& ex){ std::cout << "Logic error: " << ex.what();}
  catch(std::runtime_error& ex){ std::cout << "Runtime error: " << ex.what();}
  catch(...){ std::cout << "An unknown error occurred";}
}
