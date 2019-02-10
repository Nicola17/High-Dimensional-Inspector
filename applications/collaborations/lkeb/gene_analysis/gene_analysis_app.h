#ifndef gene_analysis_app_H
#define gene_analysis_app_H

#include <QMainWindow>
#include "hdi/utils/cout_log.h"
#include "hdi/analytics/multiscale_embedder_system_qobj.h"
#include "QTreeWidgetItem"

namespace Ui {
class gene_analysis_app;
}

class gene_analysis_app : public QMainWindow
{
    Q_OBJECT
    typedef std::tuple<unsigned int, unsigned int> id_type;
public:
    explicit gene_analysis_app(QWidget *parent = 0);
    ~gene_analysis_app();

public slots:
    void onNewEmbedding();
    void onPropagateSelection();
    void onCompute3DColorImage();
    void initTree();

public:
    Ui::gene_analysis_app *ui;

    hdi::utils::CoutLog* _log;
    hdi::analytics::MultiscaleEmbedderSystem* _multiscale_embedder;
    hdi::analytics::MultiscaleEmbedderSystem::panel_data_type* _panel_data;

    std::vector<QTreeWidgetItem*> _tree_items;
int _width,_height;
    std::vector<id_type> _idxes;
};

#endif // gene_analysis_app_H
