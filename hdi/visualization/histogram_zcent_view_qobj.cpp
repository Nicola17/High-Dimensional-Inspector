#include "histogram_zcent_view_qobj.h"

#include <QFileDialog>
#include <QLayout>
#include <QtWebKitWidgets/QWebView>
#include <QtWebKitWidgets/QWebFrame>
#include <QtWebKit/QWebElement>
#include <QFile>
#include <QDir>
#include <sstream>
#include <iostream>
#include "hdi/utils/log_helper_functions.h"


//call to Q_INIT_RESOURCE should be outside of any namespace
void loadHistogramZCentViewResources(){
  Q_INIT_RESOURCE(histogram_zcent_view_qobj);
}

namespace hdi {
  namespace viz {

    HistogramZCentView::HistogramZCentView(QWidget *parent):
      _webView(nullptr),
      _mainFrame(nullptr),
      _connection_ready(false),
      _logger(nullptr)
    {
      loadHistogramZCentViewResources();
      initUI();
    }

    QString HistogramZCentView::readTextFromFile(QString filename){
      QFile file(filename);
      if (!file.open(QIODevice::ReadOnly | QIODevice::Text)){
        std::stringstream ss;
        ss << "Unable to open the file: " << filename.toStdString();
        throw std::logic_error(ss.str());
      }
      QTextStream in(&file);
      return in.readAll();
    }

    void HistogramZCentView::initUI(){
      _webView = new QWebView(this);

      connect(_webView, &QWebView::loadFinished, this, &HistogramZCentView::onWebViewFinishedLoading);

      _mainFrame = _webView->page()->mainFrame();
      _mainFrame->setScrollBarPolicy(Qt::Vertical, Qt::ScrollBarAlwaysOff);
      _mainFrame->setScrollBarPolicy(Qt::Horizontal, Qt::ScrollBarAlwaysOff);

      QObject::connect(_mainFrame,&QWebFrame::javaScriptWindowObjectCleared,this,&HistogramZCentView::onConnectJs);
      if(!_webView->settings()->testAttribute(QWebSettings::JavascriptEnabled)){
        throw std::runtime_error("Javascript is not enabled!");
      }

      QString html = readTextFromFile(":/myWidget/histogram_zcent.html");
      _webView->setHtml(html, QUrl("qrc:/myWidget/"));
    }

    void HistogramZCentView::onJsLog(QString text){
      hdi::utils::secureLogValue(_logger,"LineChart",text.toStdString());
    }
    void HistogramZCentView::onJsError(QString text){
      throw std::runtime_error(text.toStdString().c_str());
    }

    void HistogramZCentView::onConnectJs(){
      _mainFrame->addToJavaScriptWindowObject("Qt", this);
    }

    void HistogramZCentView::onWebViewFinishedLoading(bool ok){
      _connection_ready = true;
      _webView->page()->setViewportSize(size());
    }

    HistogramZCentView::~HistogramZCentView(){
    }

    void HistogramZCentView::setData(const data::Histogram<scalar_type>& hist){
      std::stringstream ss;
      ss << "x0,x1,v" << std::endl;
      double step = (hist.max()-hist.min())/hist.num_buckets();
      for(int i = 0; i < hist.num_buckets(); ++i){
        ss << i*step << "," << (i+1)*step << "," << hist.data()[i] << std::endl;
      }
      emit sgnSetData(QString::fromStdString(ss.str()));
    }

    void HistogramZCentView::onUpdate(){
      while(!_connection_ready){//it is better to use condition_variable
         QApplication::processEvents();
      }
      emit sgnDrawVisualization();
    }

    void HistogramZCentView::resizeEvent(QResizeEvent * e){
      _webView->resize(size());
      _webView->page()->setViewportSize(size());
      onUpdate();
    }
  }
}

