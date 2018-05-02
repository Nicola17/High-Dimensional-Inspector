#include "hdi/visualization/dl_filter_viz_qobj.h"

#include <QFileDialog>
#include <QLayout>
#include <QtWebKitWidgets/QWebView>
#include <QtWebKitWidgets/QWebFrame>
#include <QtWebKit/QWebElement>
#include <QFile>
#include <QDir>
#include <sstream>
#include <cassert>
#include <iostream>
#include "hdi/data/text_data.h"
#include "hdi/utils/log_helper_functions.h"


//call to Q_INIT_RESOURCE should be outside of any namespace
void loadDLFilterVizResources(){
  Q_INIT_RESOURCE(dl_filter_viz_qobj);
}

namespace hdi {
  namespace viz {

    DLFilterViz::DLFilterViz(QWidget *parent):
      _webView(nullptr),
      _mainFrame(nullptr),
      _connection_ready(false),
      _logger(nullptr)
    {
      loadDLFilterVizResources();
      initUI();
    }

    QString DLFilterViz::readTextFromFile(QString filename){
      QFile file(filename);
      if (!file.open(QIODevice::ReadOnly | QIODevice::Text)){
        std::stringstream ss;
        ss << "Unable to open the file: " << filename.toStdString();
        throw std::logic_error(ss.str());
      }
      QTextStream in(&file);
      return in.readAll();
    }

    void DLFilterViz::initUI(){
      _webView = new QWebView(this);

      connect(_webView, &QWebView::loadFinished, this, &DLFilterViz::onWebViewFinishedLoading);

      _mainFrame = _webView->page()->mainFrame();
      _mainFrame->setScrollBarPolicy(Qt::Vertical, Qt::ScrollBarAlwaysOff);
      _mainFrame->setScrollBarPolicy(Qt::Horizontal, Qt::ScrollBarAlwaysOff);

      QObject::connect(_mainFrame,&QWebFrame::javaScriptWindowObjectCleared,this,&DLFilterViz::onConnectJs);
      if(!_webView->settings()->testAttribute(QWebSettings::JavascriptEnabled)){
        throw std::runtime_error("Javascript is not enabled!");
      }

      QString html = readTextFromFile(":/myWidget/dl_filter_viz.html");
      _webView->setHtml(html, QUrl("qrc:/myWidget/"));
    }

    void DLFilterViz::onJsLog(QString text){
      hdi::utils::secureLogValue(_logger,"DLFilterViz",text.toStdString());
    }
    void DLFilterViz::onJsError(QString text){
      throw std::runtime_error(text.toStdString().c_str());
    }

    void DLFilterViz::onConnectJs(){
      _mainFrame->addToJavaScriptWindowObject("Qt", this);
    }

    void DLFilterViz::onWebViewFinishedLoading(bool ok){
      _connection_ready = true;
      _webView->page()->setViewportSize(size());
    }

    DLFilterViz::~DLFilterViz(){
    }

    void DLFilterViz::onSetData(){
      while(!_connection_ready){//it is better to use condition_variable
         QApplication::processEvents();
      }
      typedef float scalar_type;
      std::stringstream ss;
      ss << "name,order,value,confidence,flags,frequency" << std::endl;

      assert(_values.size() == _names.size());
      assert(_values.size() == _order.size());
      for(int i = 0; i < _values.size(); ++i){
        double confidence = 1-(_current_iteration - _values[i].first)/500.;
        if(confidence > 1) confidence = 1;
        if(confidence < 0) confidence = 0;
        ss << _names[i] << "," << _order[i] << "," << _values[i].second << "," <<  confidence << "," << _flags[i] << "," << _frequencies[i] << std::endl;
      }

      //std::cout << ss.str();
      emit sgnSetData(QString::fromStdString(ss.str()));
    }

    void DLFilterViz::onDrawVisualization(){
      while(!_connection_ready){//it is better to use condition_variable
         QApplication::processEvents();
      }
      emit sgnDrawVisualization();
    }

    void DLFilterViz::setMaxValue(scalar_type v){
      while(!_connection_ready){//it is better to use condition_variable
         QApplication::processEvents();
      }
      emit sgnSetMaxValue(QString("%1").arg(v));
    }

    void DLFilterViz::resizeEvent(QResizeEvent * e){
      _webView->resize(size());
      _webView->page()->setViewportSize(size());
      onDrawVisualization();
    }
  }
}

