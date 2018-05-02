#ifndef WAOW_APPLICATION_H
#define WAOW_APPLICATION_H

#include <QMainWindow>
#include "hdi/utils/cout_log.h"
#include "hdi/analytics/multiscale_embedder_system_qobj.h"
#include "QTreeWidgetItem"
#include "hdi/visualization/anatomical_planes_view_qobj.h"
#include "hdi/analytics/waow_vis_qobj.h"
#include "hdi/visualization/word_cloud_qobj.h"
#include "hdi/visualization/embedding_lines_drawer.h"
#include "hdi/visualization/embedding_similarity_bundled_lines_drawer.h"

#include "ui_application.h"

class WAOWVisInterface;

class WAOWApplication : public QMainWindow, public hdi::analytics::WAOWVis::AbstractInterfaceInitializer
{
  Q_OBJECT
  typedef uint32_t id_type;
  typedef hdi::analytics::WAOWVis::embedder_type embedder_type;
  typedef hdi::analytics::WAOWVis::scalar_type scalar_type;

public:
  explicit WAOWApplication(QWidget *parent = 0);
  ~WAOWApplication();

public slots:
  void onNewEmbedding();
  void onUpdateWordClouds(id_type);
  void iterate();

private:
  virtual void initialize(std::shared_ptr<embedder_type> embedder);
  void initializeWeights(std::shared_ptr<embedder_type> embedder);


public:
  Ui::WAOWApplication*   _ui;
  hdi::analytics::WAOWVis* _waow_vis;
  hdi::utils::CoutLog*   _log;

  std::vector<std::shared_ptr<embedder_type>> _views;

  std::vector<hdi::viz::WordCloud*> _word_clouds_B;

};

#endif // WAOW_APPLICATION_H
