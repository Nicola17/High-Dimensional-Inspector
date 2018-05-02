#include "application.h"
#include "ui_application.h"
#include "QPushButton"
#include "hdi/utils/log_helper_functions.h"
#include "hdi/data/pixel_data.h"
#include "QImage"
#include "hdi/dimensionality_reduction/sparse_tsne_user_def_probabilities.h"
#include <iostream>
#include "hdi/data/voxel_data.h"
#include <iostream>
#include <fstream>
#include <QFileDialog>
#include <QMessageBox>
#include <future>
#include <QInputDialog>


hyperspectral_images_app::hyperspectral_images_app(QWidget *parent) :
  QMainWindow(parent),
  ui(new Ui::hyperspectral_images_app)
{
  ui->setupUi(this);
  connect(ui->_new_analysis_btn,&QPushButton::clicked,this,&hyperspectral_images_app::onNewEmbedding);
  connect(ui->_propagate_btn,&QPushButton::clicked,this,&hyperspectral_images_app::onPropagateSelection);
  connect(ui->action_export_images,&QAction::triggered,this,&hyperspectral_images_app::onExportImages);
  connect(ui->action_export_influence,&QAction::triggered,this,&hyperspectral_images_app::onExportInfluence);
  connect(ui->action_export_hierarchy,&QAction::triggered,this,&hyperspectral_images_app::onExportHierarchy);
  connect(ui->action_export_analysis,&QAction::triggered,this,&hyperspectral_images_app::onExportAnalysis);
  connect(ui->action_cluster_selection,&QAction::triggered,this,&hyperspectral_images_app::onClusterSelection);

  connect(ui->_std_view_push_btn,&QPushButton::clicked,this,&hyperspectral_images_app::onViewStd);
  connect(ui->_aoi_view_push_btn,&QPushButton::clicked,this,&hyperspectral_images_app::onViewAoi);
  connect(ui->_landmarks_view_push_btn,&QPushButton::clicked,this,&hyperspectral_images_app::onViewLandmarks);

  ui->_view_grp_bx->setEnabled(false);
}


hyperspectral_images_app::~hyperspectral_images_app()
{
  delete ui;
}

void hyperspectral_images_app::onNewEmbedding(){
  auto old_idx = ui->_embeddings_widget->currentIndex();
  auto id = _idxes[old_idx];
  hdi::utils::secureLogValue(_log,"id scale", std::get<0>(id));
  hdi::utils::secureLogValue(_log,"id analysis", std::get<1>(id));
  _multiscale_embedder->onNewAnalysisTriggered(id);

  _tree_items.push_back(new QTreeWidgetItem(QStringList(ui->_embeddings_widget->tabText(ui->_embeddings_widget->count()-1))));
  _tree_items[old_idx]->addChild(_tree_items[_tree_items.size()-1]);
  ui->_embeddings_widget->setCurrentIndex(ui->_embeddings_widget->count()-1);
}
void hyperspectral_images_app::onPropagateSelection(){
  QMessageBox m_box(QMessageBox::Information,"","");
  m_box.setText("Computing the Area of Influence on the volume...");
  m_box.setModal(true);
  m_box.setStandardButtons(0);
  m_box.open();
  QApplication::processEvents();

  auto id = _idxes[ui->_embeddings_widget->currentIndex()];
  hdi::utils::secureLogValue(_log,"id scale", std::get<0>(id));
  hdi::utils::secureLogValue(_log,"id analysis", std::get<1>(id));

  //fancy!
  auto future = std::async(std::launch::async,[](hdi::analytics::MultiscaleEmbedderSystem* embedder, id_type id){embedder->onLinkSelectionToDataPoints(id);},_multiscale_embedder,id);
  auto status = future.wait_for(std::chrono::milliseconds(0));
  while(status != std::future_status::ready){
    status = future.wait_for(std::chrono::milliseconds(10));
    QApplication::processEvents();
  }

  ui->_view_grp_bx->setEnabled(true);
}

class WeightedAvgFunctor{
public:
  void operator()(int id,float weight){
    for(int d = 0; d < _data->numDimensions(); ++d){
      (*_mean)[d] += _data->dataAt(id,d)*weight;
    }
    _total_weight += weight;
  }

  hdi::data::PanelData<float>* _data;
  std::vector<float>* _mean;
  float _total_weight;
};

void hyperspectral_images_app::onComputeStats(){
  auto id = _idxes[ui->_embeddings_widget->currentIndex()];
  hdi::utils::secureLogValue(_log,"id scale", std::get<0>(id));
  hdi::utils::secureLogValue(_log,"id analysis", std::get<1>(id));

  std::vector<float> mean(_multiscale_embedder->getPanelData().numDimensions(),0);

  WeightedAvgFunctor functor;
  functor._mean = &mean;
  functor._data = &_multiscale_embedder->getPanelData();

  std::vector<uint32_t> selection_in_scale;
  _multiscale_embedder->getSelectedLandmarksInScale(id,selection_in_scale);
  hdi::analytics::MultiscaleEmbedderSystem::hsne_type::computeAoI(_multiscale_embedder->hSNE(),std::get<0>(id),selection_in_scale,functor);

  //TODO NORMALIZE MEAN
}

void hyperspectral_images_app::initTree(){
  _tree_items.push_back(new QTreeWidgetItem(QStringList(ui->_embeddings_widget->tabText(0))));
  ui->_tree->addTopLevelItem(_tree_items[_tree_items.size()-1]);
  ui->_tree->setHeaderLabel("Name");
}

void hyperspectral_images_app::onExportImages(){
  try{
    hdi::utils::secureLog(_log,"Export images...");
    _view->xyImage().save("xy.png");
    _view->xzImage().save("xz.png");
    _view->yzImage().save("yz.png");
    hdi::utils::secureLog(_log,"done!");
  }
  catch(std::logic_error& ex){ std::cout << "Logic error: " << ex.what();}
  catch(std::runtime_error& ex){ std::cout << "Runtime error: " << ex.what();}
  catch(...){ std::cout << "An unknown error occurred";}
}
void hyperspectral_images_app::onExportInfluence(){
  try{
    std::ofstream file("aoi.csv");
    hdi::utils::secureLog(_log,"Export area of influence...");
    for(int i = 0; i < _panel_data->numDataPoints(); ++i){
      auto data_ptr = dynamic_cast<hdi::data::VoxelData*>(_panel_data->getDataPoints()[i].get());
      if(data_ptr == nullptr){
        continue;
      }
      file << data_ptr->_x << "," << data_ptr->_y << "," << data_ptr->_z << "," << _view->selection()[i] << std::endl;
    }
    hdi::utils::secureLog(_log,"done!");
  }
  catch(std::logic_error& ex){ std::cout << "Logic error: " << ex.what();}
  catch(std::runtime_error& ex){ std::cout << "Runtime error: " << ex.what();}
  catch(...){ std::cout << "An unknown error occurred";}
}
void hyperspectral_images_app::onExportHierarchy(){
  try{
    hdi::utils::secureLog(_log,"Export hierarchy...");
    auto fileName = QFileDialog::getSaveFileName(this, tr("Save to HSNE file"), ".", tr("HSNE File (*.hsne)"));
    std::ofstream file(fileName.toStdString().c_str(),std::ios::binary);
    hdi::dr::IO::saveHSNE(_multiscale_embedder->hSNE(),file,_log);
    hdi::utils::secureLog(_log,"done!");
  }
  catch(std::logic_error& ex){ std::cout << "Logic error: " << ex.what();}
  catch(std::runtime_error& ex){ std::cout << "Runtime error: " << ex.what();}
  catch(...){ std::cout << "An unknown error occurred";}
}

void hyperspectral_images_app::onExportAnalysis(){
  try{
    hdi::utils::secureLog(_log,"Export analysis...");
    auto name = QInputDialog::getText(this, tr("Save hierarchical analysis"), tr("Folder name:"), QLineEdit::Normal, QDir::home().dirName()).toStdString();
    hdi::analytics::IO::exportToCSV(_multiscale_embedder->hSNE(), _multiscale_embedder->analysis(), name);
    hdi::utils::secureLog(_log,"done!");
  }
  catch(std::logic_error& ex){ std::cout << "Logic error: " << ex.what();}
  catch(std::runtime_error& ex){ std::cout << "Runtime error: " << ex.what();}
  catch(...){ std::cout << "An unknown error occurred";}
}

void hyperspectral_images_app::onClusterSelection(){
  try{
    hdi::utils::secureLog(_log,"Cluster selection...");
    auto id = _idxes[ui->_embeddings_widget->currentIndex()];
    _multiscale_embedder->onClusterizeSelection(id);
    hdi::utils::secureLog(_log,"done!");
  }
  catch(std::logic_error& ex){ std::cout << "Logic error: " << ex.what();}
  catch(std::runtime_error& ex){ std::cout << "Runtime error: " << ex.what();}
  catch(...){ std::cout << "An unknown error occurred";}
}
