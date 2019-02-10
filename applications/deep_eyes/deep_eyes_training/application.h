#ifndef DEEP_EYES_APP_H
#define DEEP_EYES_APP_H

#include <QMainWindow>
#include "hdi/utils/pte_log.h"
#include "hdi/deep_learning/controller_training.h"
#include "hdi/deep_learning/model_layer.h"
#include "hdi/deep_learning/view_layer_overview_qobj.h"
#include "hdi/deep_learning/model_caffe_solver.h"
#include "hdi/deep_learning/view_training.h"
#include "hdi/deep_learning/model_training.h"

namespace Ui {
class DeepEyesApp;
}

class DeepEyesApp : public QMainWindow
{
    Q_OBJECT
    typedef std::tuple<unsigned int, unsigned int> id_type;
public:
    explicit DeepEyesApp(hdi::utils::PTELog* log, QWidget *parent = 0);
    ~DeepEyesApp();

protected:
    void closeEvent(QCloseEvent *event);

public slots:
    void onIterate();

public:
    Ui::DeepEyesApp *_ui;
    hdi::utils::PTELog* _log;

    hdi::dl::ControllerTraining _controller;

    std::shared_ptr<hdi::dl::ModelCaffeSolver> _solver;
    std::shared_ptr<hdi::dl::ModelTraining> _model_training;
    std::shared_ptr<hdi::dl::ViewTraining> _view_training;


};

#endif // HYPERSPECTRAL_IMAGES_APP_H
