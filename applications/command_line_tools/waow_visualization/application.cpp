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
#include "hdi/analytics/waow_vis_qobj.h"
#include "hdi/visualization/scatterplot_drawer_scalar_attribute.h"
#include "hdi/visualization/scatterplot_drawer_two_scalar_attributes.h"
#include "hdi/visualization/embedding_lines_drawer.h"
#include "hdi/visualization/embedding_bundled_lines_drawer.h"
#include "hdi/visualization/word_cloud_qobj.h"
#include "hdi/data/text_data.h"
#include <queue>


WAOWApplication::WAOWApplication(QWidget *parent) :
  QMainWindow(parent),
  _ui(new Ui::WAOWApplication)
{
  _ui->setupUi(this);
  _ui->_histograms_setA_grpbx->setVisible(false);
  _ui->_histograms_setB_grpbx->setVisible(false);
  connect(_ui->_new_analysis_btn,&QPushButton::clicked,this,&WAOWApplication::onNewEmbedding);
}


WAOWApplication::~WAOWApplication()
{
  delete _ui;
}

void WAOWApplication::onNewEmbedding(){
  uint32_t id = _ui->_embeddings_widget->currentIndex();
  _waow_vis->onCreateNewView(id);
}

void WAOWApplication::onUpdateWordClouds(id_type id){
  assert(id < _ui->_embeddings_widget->count());
  const embedder_type& view = *(_views[id]);

  std::vector<uint32_t> selectionA;
  view.getSelectionA(selectionA);
  std::vector<uint32_t> selectionB;
  view.getSelectionB(selectionB);

  std::unordered_map<std::string,uint32_t> word_cloud;
  if(selectionA.size()){
    std::priority_queue<std::pair<double,int>> heap;
    double max_v = 0;
    for(int i = 0; i < view.getPanelDataB().getDataPoints().size(); ++i){
      max_v = std::max<double>(view._selection_B[i],max_v);
      heap.push(std::pair<double,int>(double(view._selection_B[i]),i));
    }

    if(max_v > 0){
      const int max_elem = _ui->_max_elem_word_cloud_spbx->value();
      for(int j = 0; j < max_elem && !heap.empty(); ++j){
        auto pair = heap.top();
        if(pair.first > max_v*0.3){
          auto data = reinterpret_cast<hdi::data::TextData*>(view.getPanelDataB().getDataPoints()[pair.second].get());
          word_cloud[data->text()] = pair.first*10;
        }
        heap.pop();
      }
    }
  }else if(selectionB.size()){
    double max_v = 0;
    for(int i = 0; i < selectionB.size(); ++i){
      uint32_t l = selectionB[i];
      max_v = std::max(max_v,double(std::sqrt(view._landmark_weights_B[l])));
    }
    std::priority_queue<std::pair<double,int>> heap;
    for(int i = 0; i < selectionB.size(); ++i){
      uint32_t l = selectionB[i];
      heap.push(std::pair<double,int>(double(std::sqrt(view._landmark_weights_B[l]))/max_v*10+1,l));
    }
    const int max_elem = _ui->_max_elem_word_cloud_spbx->value();
    for(int j = 0; j < max_elem && !heap.empty(); ++j){
      auto pair = heap.top();
      auto data = reinterpret_cast<hdi::data::TextData*>(view.getPanelDataB().getDataPoints()[pair.second].get());
      word_cloud[data->text()] = pair.first;
      heap.pop();
    }
  }
  _word_clouds_B[id]->setWords(word_cloud);
  _word_clouds_B[id]->updateView();
}

void WAOWApplication::iterate(){
  const int id = _ui->_embeddings_widget->currentIndex();
  embedder_type& view = *(_views[id]);

  //not expensive
  reinterpret_cast<hdi::viz::ScatterplotDrawerTwoScalarAttributes*>(view._drawers["Weights"]["A_2D"].get())->setAlpha(_ui->_alpha_A_spbx->value());
  reinterpret_cast<hdi::viz::ScatterplotDrawerTwoScalarAttributes*>(view._drawers["Weights"]["A_1D"].get())->setAlpha(_ui->_alpha_A_spbx->value()/3.);
  reinterpret_cast<hdi::viz::ScatterplotDrawerTwoScalarAttributes*>(view._drawers["Weights"]["B_2D"].get())->setAlpha(_ui->_alpha_B_spbx->value());
  reinterpret_cast<hdi::viz::ScatterplotDrawerTwoScalarAttributes*>(view._drawers["Weights"]["B_1D"].get())->setAlpha(_ui->_alpha_B_spbx->value()/1.5);

  reinterpret_cast<hdi::viz::EmbeddingBundledLinesDrawer*>     (view._drawers["Weights"]["Lines_A_B"].get())->enabled()     =_ui->_bundling_chbx->isChecked();
  reinterpret_cast<hdi::viz::EmbeddingBundledLinesDrawer*>     (view._drawers["Weights"]["Lines_A_B"].get())->sgd_samples()   =_ui->_samples_bundling_spbx->value();
  reinterpret_cast<hdi::viz::EmbeddingBundledLinesDrawer*>     (view._drawers["Weights"]["Lines_A_B"].get())->sgd_strength()  =_ui->_strength_bundling_spbx->value();

  if(std::abs(view._lines_alpha-_ui->_alpha_lines_spbx->value()) > 0.0001){
    view._lines_alpha = _ui->_alpha_lines_spbx->value();
    view.updateConnections(true);
  }

  if(view._always_show_lines != _ui->_lines_alwasy_chbx->isChecked()){
     view._always_show_lines = _ui->_lines_alwasy_chbx->isChecked();
    view.updateConnections(true);
  }

  view._iterate    = _ui->_iterate_chbx->isChecked();
  view._intra_set_eq   = _ui->_intra_set_align_chbx->isChecked();
  view._inter_set_eq   = _ui->_inter_set_align_chbx->isChecked();

  view._emph_selection = _ui->_emph_selection_spbx->value();

  view.gettSNEA1D().exaggeration_baseline() = _ui->_exg_spbx->value();
  view.gettSNEA2D().exaggeration_baseline() = _ui->_exg_spbx->value();
  view.gettSNEB1D().exaggeration_baseline() = 1 + (_ui->_exg_spbx->value()-1)*0.5;
  view.gettSNEB2D().exaggeration_baseline() = 1 + (_ui->_exg_spbx->value()-1)*0.5;


  view.doAnIteration();
  QApplication::processEvents();
}

void WAOWApplication::initialize(std::shared_ptr<embedder_type> embedder)
{
  connect(embedder.get(),&embedder_type::sgnSelection,this,&WAOWApplication::onUpdateWordClouds);
  initializeWeights(embedder);

}

void WAOWApplication::initializeWeights(std::shared_ptr<embedder_type> embedder)
{

  const std::vector<scalar_type>&   weights_A   = embedder->_landmark_weights_A;
  const std::vector<scalar_type>&   weights_B   = embedder->_landmark_weights_B;
  const std::vector<scalar_type>&   selection_A = embedder->_selection_A;
  const std::vector<scalar_type>&   selection_B = embedder->_selection_B;

  QColor color_A      (qRgb(34,94,168));
  QColor color_B      (qRgb(35,132,67));
  QColor selection_color  (qRgb(200,120,0));

  std::array<QColor,4> colors_A{qRgb(237,248,177),qRgb(199,233,180),qRgb(173,221,142),qRgb(34,94,168)};
  std::array<QColor,4> colors_B{qRgb(35,132,67),qRgb(217,240,163),qRgb(65,182,196),selection_color};

  std::shared_ptr<hdi::viz::ScatterplotDrawerTwoScalarAttributes> drawer_A_2D = std::make_shared<hdi::viz::ScatterplotDrawerTwoScalarAttributes>();
  drawer_A_2D->setData(embedder->getEmbeddingA2DView().getContainer().data(), weights_A.data(), selection_A.data(), embedder->getPanelDataA().getFlagsDataPoints().data(), embedder->getPanelDataA().numDataPoints());
  drawer_A_2D->setAlpha(0.15);
  drawer_A_2D->setPointSize(50);
  drawer_A_2D->setMinPointSize(5);
  drawer_A_2D->updateLimitsFromData();
  drawer_A_2D->setColors(colors_A);
  drawer_A_2D->setSelectionColor(selection_color);
  embedder->addDrawer("Weights","A_2D",drawer_A_2D);

  std::shared_ptr<hdi::viz::ScatterplotDrawerTwoScalarAttributes> drawer_A_1D = std::make_shared<hdi::viz::ScatterplotDrawerTwoScalarAttributes>();
  drawer_A_1D->setData(embedder->getEmbeddingA1DView().getContainer().data(), weights_A.data(), selection_A.data(), embedder->getPanelDataA().getFlagsDataPoints().data(), embedder->getPanelDataA().numDataPoints());
  drawer_A_1D->setAlpha(0.05);
  drawer_A_1D->setPointSize(50);
  drawer_A_1D->setMinPointSize(5);
  drawer_A_1D->updateLimitsFromData();
  drawer_A_1D->setColors(colors_A);
  drawer_A_1D->setSelectionColor(selection_color);
  embedder->addDrawer("Weights","A_1D",drawer_A_1D);


  std::shared_ptr<hdi::viz::ScatterplotDrawerTwoScalarAttributes> drawer_B_2D = std::make_shared<hdi::viz::ScatterplotDrawerTwoScalarAttributes>();
  drawer_B_2D->setData(embedder->getEmbeddingB2DView().getContainer().data(), weights_B.data(), selection_B.data(), embedder->getPanelDataB().getFlagsDataPoints().data(), embedder->getPanelDataB().numDataPoints());
  drawer_B_2D->setAlpha(0.4);
  drawer_B_2D->setPointSize(35);
  drawer_B_2D->setMinPointSize(7);
  drawer_B_2D->updateLimitsFromData();
  drawer_B_2D->setColors(colors_B);
  drawer_B_2D->setSelectionColor(selection_color);
  embedder->addDrawer("Weights","B_2D",drawer_B_2D);

  std::shared_ptr<hdi::viz::ScatterplotDrawerTwoScalarAttributes> drawer_B_1D = std::make_shared<hdi::viz::ScatterplotDrawerTwoScalarAttributes>();
  drawer_B_1D->setData(embedder->getEmbeddingB1DView().getContainer().data(), weights_B.data(), selection_B.data(), embedder->getPanelDataB().getFlagsDataPoints().data(), embedder->getPanelDataB().numDataPoints());
  drawer_B_1D->setAlpha(0.3);
  drawer_B_1D->setPointSize(35);
  drawer_B_1D->setMinPointSize(7);
  drawer_B_1D->updateLimitsFromData();
  drawer_B_1D->setColors(colors_B);
  drawer_B_1D->setSelectionColor(selection_color);
  embedder->addDrawer("Weights","B_1D",drawer_B_1D);

  //std::shared_ptr<hdi::viz::EmbeddingLinesDrawer> lines_A_B = std::make_shared<hdi::viz::EmbeddingLinesDrawer>();
  //lines_A_B->setData(embedder->getEmbeddingA1DView().getContainer().data(), embedder->getEmbeddingB1DView().getContainer().data());
  std::shared_ptr<hdi::viz::EmbeddingBundledLinesDrawer> lines_A_B = std::make_shared<hdi::viz::EmbeddingBundledLinesDrawer>();
  lines_A_B->setData(embedder->getEmbeddingA1DView().getContainer().data(), embedder->getEmbeddingB1DView().getContainer().data());

  lines_A_B->setAlpha(0.2);
  lines_A_B->setColor(selection_color);

  embedder->addDrawer("Weights","Lines_A_B",lines_A_B);

  QWidget* wdg = new QWidget();
  QGridLayout* grid = new QGridLayout();
  QGridLayout* viz_layout = new QGridLayout();
  wdg->setLayout(grid);

  hdi::viz::WordCloud* wc_tweets  = new hdi::viz::WordCloud();

  grid->addLayout(viz_layout,       0,1);
  grid->addWidget(wc_tweets->widgetPtr(), 0,2);
  //wc_tweets->show();

  viz_layout->addWidget(embedder->getCanvas());

  embedder->getCanvas()->setFixedSize(1200,400);
  embedder->onUpdateCanvas();

  _ui->_embeddings_widget->addTab(wdg,QIcon(),"New View");

  QString emb_name = QInputDialog::getText(0,"New embedding","Embedding name");
  _ui->_embeddings_widget->setTabText(_ui->_embeddings_widget->count()-1,emb_name);
  _ui->_embeddings_widget->setCurrentIndex(_ui->_embeddings_widget->count()-1);

  _views.push_back(embedder);
  _word_clouds_B.push_back(wc_tweets);

}
