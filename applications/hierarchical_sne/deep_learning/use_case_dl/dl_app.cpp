#include "dl_app.h"
#include "ui_dl_app.h"
#include "QPushButton"
#include "hdi/utils/log_helper_functions.h"
#include "hdi/data/pixel_data.h"
#include "QImage"


dl_app::dl_app(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::dl_app)
{
    ui->setupUi(this);
    connect(ui->_new_analysis_btn,&QPushButton::clicked,this,&dl_app::onNewEmbedding);
    connect(ui->_change_viz,&QPushButton::toggled,this,&dl_app::toggleViz);
    //connect(ui->_out_btn,&QPushButton::clicked,this,&dl_app::onShowOutliers);
}

dl_app::~dl_app()
{
    delete ui;
}

void dl_app::toggleViz(bool v){
    if(!v)
        _multiscale_embedder->onActivateInfluenceMode(std::tuple<unsigned int, unsigned int>(0,0));
    else
        _multiscale_embedder->onActivateSelectionMode(std::tuple<unsigned int, unsigned int>(0,0));
}

void dl_app::onNewEmbedding(){
    auto old_idx = ui->_embeddings_widget->currentIndex();
    auto id = _idxes[old_idx];
    hdi::utils::secureLogValue(_log,"id scale", std::get<0>(id));
    hdi::utils::secureLogValue(_log,"id analysis", std::get<1>(id));
    _multiscale_embedder->onNewAnalysisTriggered(id);

    _tree_items.push_back(new QTreeWidgetItem(QStringList(ui->_embeddings_widget->tabText(ui->_embeddings_widget->count()-1))));
    _tree_items[old_idx]->addChild(_tree_items[_tree_items.size()-1]);
    ui->_embeddings_widget->setCurrentIndex(ui->_embeddings_widget->count()-1);
}
void dl_app::onPropagateSelection(){
  /*
    auto id = _idxes[ui->_embeddings_widget->currentIndex()];
    hdi::utils::secureLogValue(_log,"id scale", std::get<0>(id));
    hdi::utils::secureLogValue(_log,"id analysis", std::get<1>(id));
    _multiscale_embedder->onLinkSelectionToDataPoints(id);
  */
}
void dl_app::onShowOutliers(){
    /*
    typedef hdi::analytics::MultiscaleEmbedderSystem::panel_data_type panel_data_type;
    const auto& data = _panel_data->getDataPoints();

    QImage selection_image = QImage(_width,_height,QImage::Format_ARGB32);
    for(int i = 0; i < _multiscale_embedder->_random_walker._outliers.size(); ++i){
        auto data_ptr = dynamic_cast<hdi::data::PixelData*>(data[_multiscale_embedder->_random_walker._outliers[i]].get());
        if(data_ptr == nullptr){
            continue;
        }
        if(data_ptr->_height != selection_image.height() || data_ptr->_width != selection_image.width()){
            continue;
        }

        selection_image.setPixel(data_ptr->_u,data_ptr->_v,qRgb(255,0,0));
    }
    ui->_aoi_label->setPixmap(QPixmap::fromImage(selection_image.scaledToHeight(512,Qt::SmoothTransformation)));
*/
}


void dl_app::initTree(){
    _tree_items.push_back(new QTreeWidgetItem(QStringList(ui->_embeddings_widget->tabText(0))));
    ui->_tree->addTopLevelItem(_tree_items[_tree_items.size()-1]);
    ui->_tree->setHeaderLabel("Name");
}
