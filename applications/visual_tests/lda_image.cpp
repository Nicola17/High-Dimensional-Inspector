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

  template <typename scalar_type>
  void lda(const Eigen::MatrixXd& cov_mat_a, const Eigen::MatrixXd& cov_mat_b, const std::vector<scalar_type>& mean_a, const std::vector<scalar_type>& mean_b, std::vector<scalar_type>& res){

    Eigen::MatrixXd temp(cov_mat_a+cov_mat_b);
    Eigen::VectorXd vector(Eigen::VectorXd::Constant(mean_a.size(),0));
    for(int i = 0; i < mean_a.size(); ++i){
      vector(i) = mean_a[i]-mean_b[i];
    }

    vector = temp.inverse()*vector;
    res.resize(mean_a.size());

    for(int i = 0; i < mean_a.size(); ++i){
      res[i] = vector(i);
    }
  }



//////////////////////////////////////

  template <typename scalar_type>
  void panelDataFromImageRed(const QImage& image, data::PanelData<scalar_type>& panel_data){

    typedef data::PanelData<scalar_type> panel_data_type;
    panel_data.addDimension(std::make_shared<data::EmptyData>());
    panel_data.addDimension(std::make_shared<data::EmptyData>());
    panel_data.initialize();

    for(int j = 0; j < image.height(); ++j){
      for(int i = 0; i < image.width(); ++i){
        if(QColor(image.pixel(i,j)).redF() > 0.5 && QColor(image.pixel(i,j)).blueF() < 0.5){
          std::vector<scalar_type> hd(2,0);
          hd[0] = i;
          hd[1] = j;
          panel_data.addDataPoint(std::make_shared<data::PixelData>(i,j),hd);
        }
      }
    }

  }

  template <typename scalar_type>
  void panelDataFromImageGreen(const QImage& image, data::PanelData<scalar_type>& panel_data){

    typedef data::PanelData<scalar_type> panel_data_type;
    panel_data.addDimension(std::make_shared<data::EmptyData>());
    panel_data.addDimension(std::make_shared<data::EmptyData>());
    panel_data.initialize();

    for(int j = 0; j < image.height(); ++j){
      for(int i = 0; i < image.width(); ++i){
        if(QColor(image.pixel(i,j)).greenF() > 0.5 && QColor(image.pixel(i,j)).blueF() < 0.5){
          std::vector<scalar_type> hd(2,0);
          hd[0] = i;
          hd[1] = j;
          panel_data.addDataPoint(std::make_shared<data::PixelData>(i,j),hd);
        }
      }
    }

  }

  template <typename scalar_type>
  void panelDataFromImageBlue(const QImage& image, data::PanelData<scalar_type>& panel_data){

    typedef data::PanelData<scalar_type> panel_data_type;
    panel_data.addDimension(std::make_shared<data::EmptyData>());
    panel_data.addDimension(std::make_shared<data::EmptyData>());
    panel_data.initialize();

    for(int j = 0; j < image.height(); ++j){
      for(int i = 0; i < image.width(); ++i){
        if(QColor(image.pixel(i,j)).blueF() > 0.5 && QColor(image.pixel(i,j)).redF() < 0.5){
          std::vector<scalar_type> hd(2,0);
          hd[0] = i;
          hd[1] = j;
          panel_data.addDataPoint(std::make_shared<data::PixelData>(i,j),hd);
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
    hdi::data::PanelData<scalar_type> panel_data_red;
    hdi::panelDataFromImageRed(image,panel_data_red);
    hdi::data::PanelData<scalar_type> panel_data_green;
    hdi::panelDataFromImageGreen(image,panel_data_green);
    hdi::data::PanelData<scalar_type> panel_data_blue;
    hdi::panelDataFromImageBlue(image,panel_data_blue);

    std::vector<scalar_type> mean_red;
    hdi::data::computeMean(panel_data_red,mean_red);
    std::vector<scalar_type> mean_green;
    hdi::data::computeMean(panel_data_green,mean_green);
    std::vector<scalar_type> mean_blue;
    hdi::data::computeMean(panel_data_blue,mean_blue);

    Eigen::MatrixXd cov_matrix_red;
    hdi::computeCovarianceMatrix(panel_data_red,cov_matrix_red);
    std::cout << std::endl << std::endl << cov_matrix_red << std::endl << std::endl ;
    Eigen::MatrixXd cov_matrix_green;
    hdi::computeCovarianceMatrix(panel_data_green,cov_matrix_green);
    std::cout << std::endl << std::endl << cov_matrix_green << std::endl << std::endl ;
    Eigen::MatrixXd cov_matrix_blue;
    hdi::computeCovarianceMatrix(panel_data_blue,cov_matrix_blue);
    std::cout << std::endl << std::endl << cov_matrix_blue << std::endl << std::endl ;

    QPixmap pixmap(QPixmap::fromImage(image));
    QPainter painter;
    QPen pen;
    pen.setWidth(20);
    pen.setColor(qRgb(30,30,30));
    painter.begin(&pixmap);
    painter.setPen(pen);

    {
      Eigen::EigenSolver<Eigen::MatrixXd> eigensolver(cov_matrix_red);

      pen.setWidth(2);
      painter.setPen(pen);

      painter.drawLine(
            QPointF(mean_red[0]+eigensolver.eigenvectors()(0,0).real()*std::sqrt(eigensolver.eigenvalues()(0,0).real()),
                mean_red[1]+eigensolver.eigenvectors()(1,0).real()*std::sqrt(eigensolver.eigenvalues()(0,0).real())),
            QPointF(mean_red[0]-eigensolver.eigenvectors()(0,0).real()*std::sqrt(eigensolver.eigenvalues()(0,0).real()),
                mean_red[1]-eigensolver.eigenvectors()(1,0).real()*std::sqrt(eigensolver.eigenvalues()(0,0).real()))
          );
      painter.drawLine(
            QPointF(mean_red[0]+eigensolver.eigenvectors()(0,1).real()*std::sqrt(eigensolver.eigenvalues()(1,0).real()),
                mean_red[1]+eigensolver.eigenvectors()(1,1).real()*std::sqrt(eigensolver.eigenvalues()(1,0).real())),
            QPointF(mean_red[0]-eigensolver.eigenvectors()(0,1).real()*std::sqrt(eigensolver.eigenvalues()(1,0).real()),
                mean_red[1]-eigensolver.eigenvectors()(1,1).real()*std::sqrt(eigensolver.eigenvalues()(1,0).real()))
          );


      std::cout << "tell me something " << eigensolver.eigenvalues()<< std::endl;
      std::cout << "tell me something more " << eigensolver.eigenvectors() << std::endl;
      std::cout << eigensolver.eigenvalues()(0,0).real();
      double a = eigensolver.eigenvalues()(0,0).real();

    }


    {
      Eigen::EigenSolver<Eigen::MatrixXd> eigensolver(cov_matrix_green);

      pen.setWidth(2);
      painter.setPen(pen);

      painter.drawLine(
            QPointF(mean_green[0]+eigensolver.eigenvectors()(0,0).real()*std::sqrt(eigensolver.eigenvalues()(0,0).real()),
                mean_green[1]+eigensolver.eigenvectors()(1,0).real()*std::sqrt(eigensolver.eigenvalues()(0,0).real())),
            QPointF(mean_green[0]-eigensolver.eigenvectors()(0,0).real()*std::sqrt(eigensolver.eigenvalues()(0,0).real()),
                mean_green[1]-eigensolver.eigenvectors()(1,0).real()*std::sqrt(eigensolver.eigenvalues()(0,0).real()))
          );
      painter.drawLine(
            QPointF(mean_green[0]+eigensolver.eigenvectors()(0,1).real()*std::sqrt(eigensolver.eigenvalues()(1,0).real()),
                mean_green[1]+eigensolver.eigenvectors()(1,1).real()*std::sqrt(eigensolver.eigenvalues()(1,0).real())),
            QPointF(mean_green[0]-eigensolver.eigenvectors()(0,1).real()*std::sqrt(eigensolver.eigenvalues()(1,0).real()),
                mean_green[1]-eigensolver.eigenvectors()(1,1).real()*std::sqrt(eigensolver.eigenvalues()(1,0).real()))
          );


      std::cout << "tell me something " << eigensolver.eigenvalues()<< std::endl;
      std::cout << "tell me something more " << eigensolver.eigenvectors() << std::endl;
      std::cout << eigensolver.eigenvalues()(0,0).real();
      double a = eigensolver.eigenvalues()(0,0).real();

    }


    {
      Eigen::EigenSolver<Eigen::MatrixXd> eigensolver(cov_matrix_blue);

      pen.setWidth(2);
      painter.setPen(pen);

      painter.drawLine(
            QPointF(mean_blue[0]+eigensolver.eigenvectors()(0,0).real()*std::sqrt(eigensolver.eigenvalues()(0,0).real()),
                mean_blue[1]+eigensolver.eigenvectors()(1,0).real()*std::sqrt(eigensolver.eigenvalues()(0,0).real())),
            QPointF(mean_blue[0]-eigensolver.eigenvectors()(0,0).real()*std::sqrt(eigensolver.eigenvalues()(0,0).real()),
                mean_blue[1]-eigensolver.eigenvectors()(1,0).real()*std::sqrt(eigensolver.eigenvalues()(0,0).real()))
          );
      painter.drawLine(
            QPointF(mean_blue[0]+eigensolver.eigenvectors()(0,1).real()*std::sqrt(eigensolver.eigenvalues()(1,0).real()),
                mean_blue[1]+eigensolver.eigenvectors()(1,1).real()*std::sqrt(eigensolver.eigenvalues()(1,0).real())),
            QPointF(mean_blue[0]-eigensolver.eigenvectors()(0,1).real()*std::sqrt(eigensolver.eigenvalues()(1,0).real()),
                mean_blue[1]-eigensolver.eigenvectors()(1,1).real()*std::sqrt(eigensolver.eigenvalues()(1,0).real()))
          );


      std::cout << "tell me something " << eigensolver.eigenvalues()<< std::endl;
      std::cout << "tell me something more " << eigensolver.eigenvectors() << std::endl;
      std::cout << eigensolver.eigenvalues()(0,0).real();
      double a = eigensolver.eigenvalues()(0,0).real();

    }

/////////////////////////////////////////////////////////////////

    {
      std::vector<scalar_type> res;
      hdi::lda(cov_matrix_red,cov_matrix_green,mean_red,mean_green,res);
      painter.drawLine(
            QPointF(0.5*mean_red[0]+0.5*mean_green[0]+res[0]*200,
                0.5*mean_red[1]+0.5*mean_green[1]+res[1]*200),
            QPointF(0.5*mean_red[0]+0.5*mean_green[0]-res[0]*200,
                0.5*mean_red[1]+0.5*mean_green[1]-res[1]*200)
          );

    }

    {
      std::vector<scalar_type> res;
      hdi::lda(cov_matrix_red,cov_matrix_blue,mean_red,mean_blue,res);
      painter.drawLine(
            QPointF(0.5*mean_red[0]+0.5*mean_blue[0]+res[0]*200,
                0.5*mean_red[1]+0.5*mean_blue[1]+res[1]*200),
            QPointF(0.5*mean_red[0]+0.5*mean_blue[0]-res[0]*200,
                0.5*mean_red[1]+0.5*mean_blue[1]-res[1]*200)
          );

    }

    {
      std::vector<scalar_type> res;
      hdi::lda(cov_matrix_green,cov_matrix_blue,mean_green,mean_blue,res);
      painter.drawLine(
            QPointF(0.5*mean_green[0]+0.5*mean_blue[0]+res[0]*200,
                0.5*mean_green[1]+0.5*mean_blue[1]+res[1]*200),
            QPointF(0.5*mean_green[0]+0.5*mean_blue[0]-res[0]*200,
                0.5*mean_green[1]+0.5*mean_blue[1]-res[1]*200)
          );

    }


    image = pixmap.toImage();
    image.save(argv[2]);

    return 0;
  }
  catch(std::logic_error& ex){ std::cout << "Logic error: " << ex.what();}
  catch(std::runtime_error& ex){ std::cout << "Runtime error: " << ex.what();}
  catch(...){ std::cout << "An unknown error occurred";}
}
