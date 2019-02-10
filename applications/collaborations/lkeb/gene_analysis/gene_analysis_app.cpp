#include "gene_analysis_app.h"
#include "ui_gene_analysis_app.h"
#include "QPushButton"
#include "hdi/utils/log_helper_functions.h"
#include "hdi/data/pixel_data.h"
#include "QImage"
#include "hdi/dimensionality_reduction/sparse_tsne_user_def_probabilities.h"

gene_analysis_app::gene_analysis_app(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::gene_analysis_app)
{
    ui->setupUi(this);
    connect(ui->_new_analysis_btn,&QPushButton::clicked,this,&gene_analysis_app::onNewEmbedding);
    connect(ui->_propagate_btn,&QPushButton::clicked,this,&gene_analysis_app::onPropagateSelection);
}

gene_analysis_app::~gene_analysis_app()
{
    delete ui;
}

void gene_analysis_app::onNewEmbedding(){
    auto old_idx = ui->_embeddings_widget->currentIndex();
    auto id = _idxes[old_idx];
    hdi::utils::secureLogValue(_log,"id scale", std::get<0>(id));
    hdi::utils::secureLogValue(_log,"id analysis", std::get<1>(id));
    _multiscale_embedder->onNewAnalysisTriggered(id);

    _tree_items.push_back(new QTreeWidgetItem(QStringList(ui->_embeddings_widget->tabText(ui->_embeddings_widget->count()-1))));
    _tree_items[old_idx]->addChild(_tree_items[_tree_items.size()-1]);
    ui->_embeddings_widget->setCurrentIndex(ui->_embeddings_widget->count()-1);
}
void gene_analysis_app::onPropagateSelection(){
    auto id = _idxes[ui->_embeddings_widget->currentIndex()];
    hdi::utils::secureLogValue(_log,"id scale", std::get<0>(id));
    hdi::utils::secureLogValue(_log,"id analysis", std::get<1>(id));
    _multiscale_embedder->onLinkSelectionToDataPoints(id);
}

void gene_analysis_app::initTree(){
    _tree_items.push_back(new QTreeWidgetItem(QStringList(ui->_embeddings_widget->tabText(0))));
    ui->_tree->addTopLevelItem(_tree_items[_tree_items.size()-1]);
    ui->_tree->setHeaderLabel("Name");
}


void gene_analysis_app::onCompute3DColorImage(){
}

