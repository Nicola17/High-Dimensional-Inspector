#ifndef HYPERSPECTRAL_IMAGES_APP_H
#define HYPERSPECTRAL_IMAGES_APP_H

#include <QMainWindow>
#include "hdi/utils/cout_log.h"
#include "hdi/analytics/multiscale_embedder_system_qobj.h"
#include "QTreeWidgetItem"
#include "hdi/visualization/anatomical_planes_view_qobj.h"

namespace Ui {
class hyperspectral_images_app;
}

class hyperspectral_images_app : public QMainWindow
{
  Q_OBJECT
  typedef std::tuple<unsigned int, unsigned int> id_type;
public:
  explicit hyperspectral_images_app(QWidget *parent = 0);
  ~hyperspectral_images_app();

public slots:
  void onNewEmbedding();
  void onPropagateSelection();
  void onComputeStats();
  void initTree();

  void onExportImages();
  void onExportInfluence();
  void onExportHierarchy();
  void onExportAnalysis();

  void onViewStd(){_multiscale_embedder->onActivateUserDefinedMode(hdi::analytics::MultiscaleEmbedderSystem::embedder_id_type(0,0));}
  void onViewAoi(){_multiscale_embedder->onActivateInfluenceMode(hdi::analytics::MultiscaleEmbedderSystem::embedder_id_type(0,0));}
  void onViewLandmarks(){_multiscale_embedder->onActivateSelectionMode(hdi::analytics::MultiscaleEmbedderSystem::embedder_id_type(0,0));}

  void onClusterSelection();

public:
  Ui::hyperspectral_images_app *ui;

  hdi::utils::CoutLog* _log;
  hdi::analytics::MultiscaleEmbedderSystem* _multiscale_embedder;
  hdi::analytics::MultiscaleEmbedderSystem::panel_data_type* _panel_data;

  std::vector<QTreeWidgetItem*> _tree_items;
  std::vector<id_type> _idxes;

  hdi::viz::AnatomicalPlanesView* _view;
};

#endif // HYPERSPECTRAL_IMAGES_APP_H
