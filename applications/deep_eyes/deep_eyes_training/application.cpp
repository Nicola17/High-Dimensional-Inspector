#include "application.h"
#include "ui_application.h"
#include "QPushButton"
#include <QSettings>
#include <QInputDialog>
#include "hdi/utils/log_helper_functions.h"
#include "hdi/deep_learning/view_layer_overview_qobj.h"



DeepEyesApp::DeepEyesApp(hdi::utils::PTELog* log, QWidget *parent) :
    QMainWindow(parent),
    _log(log),
    _ui(new Ui::DeepEyesApp)
{
    QSettings settings("NicolaPezzotti", "DeepEyes");
    restoreGeometry(settings.value("geometry").toByteArray());
    restoreState(settings.value("windowState").toByteArray());

    std::string filename("/home/nicola/Development/caffe/examples/mnist/lenet_solver.prototxt");
//    std::string filename("/home/nicola/Development/caffe/examples/mnist/lenet_solver_original.prototxt");
    //std::string filename("/home/nicola/Development/caffe/examples/cifar10/cifar10_quick_solver.prototxt");
    //std::string filename("/home/nicola/Development/caffe/examples/cifar10/cifar10_full_solver.prototxt");
    //std::string filename("/media/nicola/5A0C48CC24750E7F/Development/MitoticFigures/Caffe/mitotic_solver.prototxt");
    //std::string filename("/media/nicola/5A0C48CC24750E7F/Development/MitoticFigures/Caffe/mitotic_solver_original.prototxt");
//    std::string filename("/home/nicola/Development/caffe/models/finetune_flickr_style/solver.prototxt");
    //std::string filename("/media/nicola/5A0C48CC24750E7F/Development/SegmentsEnumeration/segmenti_paper/cnn_prototxt/solver_files.prototxt");

    _ui->setupUi(this);

    _log->setPTE(_ui->_log_pte);
    _log->display("Initializing the application...",true);
    _solver = std::shared_ptr<hdi::dl::ModelCaffeSolver>(new hdi::dl::ModelCaffeSolver());
    _solver->_log = _log;
    _controller._log = _log;
    _controller._settings_wdg = _ui->_settings_wdg;
    _controller._overview_vlayout = _ui->_overview_vlayout;
    _controller._layer_tab_wdg = _ui->_layer_tab_wdg;

    _model_training = std::shared_ptr<hdi::dl::ModelTraining>(new hdi::dl::ModelTraining());
    _view_training = std::shared_ptr<hdi::dl::ViewTraining>(new hdi::dl::ViewTraining(_ui->_overview_grid_layout));

    //solver
    _solver->initialize(filename);

//    _solver->_solver->net()->CopyTrainedLayersFrom("/home/nicola/Development/caffe/models/bvlc_reference_caffenet/bvlc_reference_caffenet.caffemodel");
    //_solver->_solver->net()->CopyTraineildLayersFrom("BAP_counting_a_iter_20000.caffemodel");
    _controller.initialize(_solver,_model_training,_view_training);


    /*
    for(auto& view: _layer_views){
        addDockWidget(Qt::RightDockWidgetArea,view.get());
        //view->show();
    }
    for(int i = 1; i < _layer_views.size(); ++i){
        tabifyDockWidget(_layer_views[0].get(),_layer_views[i].get());
    }
    */

    _log->display("Inizialization complete!",true);

    //Connections
    connect(_ui->_iterate_pbtn,&QPushButton::clicked,this,&DeepEyesApp::onIterate);
}

void DeepEyesApp::closeEvent(QCloseEvent *event)
{
    QSettings settings("NicolaPezzotti", "DeepEyes");
    settings.setValue("geometry", saveGeometry());
    settings.setValue("windowState", saveState());
    QMainWindow::closeEvent(event);
}


DeepEyesApp::~DeepEyesApp(){
    delete _ui;
}


void DeepEyesApp::onIterate(){
    hdi::utils::secureLog(_log,"iteration");
    bool ok;
    int iter = QInputDialog::getInt(this, tr("Train"), tr("#iter:"), 1000, 1, 20000, 100, &ok);
    if (!ok)
        return;
    for(int i = 0; i <= iter; ++i)//TODO
        _controller.iterate();
}

