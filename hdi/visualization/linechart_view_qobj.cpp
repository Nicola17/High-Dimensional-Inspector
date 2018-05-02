#include "linechart_view_qobj.h"

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
void loadLineChartViewResources(){
  Q_INIT_RESOURCE(linechart_view_qobj);
}

namespace hdi {
  namespace viz {

    LineChartView::LineChartView(QWidget *parent):
      _webView(nullptr),
      _mainFrame(nullptr),
      _connection_ready(false),
      _logger(nullptr)
    {
      loadLineChartViewResources();
      initUI();
    }

    QString LineChartView::readTextFromFile(QString filename){
      QFile file(filename);
      if (!file.open(QIODevice::ReadOnly | QIODevice::Text)){
        std::stringstream ss;
        ss << "Unable to open the file: " << filename.toStdString();
        throw std::logic_error(ss.str());
      }
      QTextStream in(&file);
      return in.readAll();
    }

    void LineChartView::initUI(){
      _webView = new QWebView(this);

      connect(_webView, &QWebView::loadFinished, this, &LineChartView::onWebViewFinishedLoading);

      _mainFrame = _webView->page()->mainFrame();
      _mainFrame->setScrollBarPolicy(Qt::Vertical, Qt::ScrollBarAlwaysOff);
      _mainFrame->setScrollBarPolicy(Qt::Horizontal, Qt::ScrollBarAlwaysOff);

      QObject::connect(_mainFrame,&QWebFrame::javaScriptWindowObjectCleared,this,&LineChartView::onConnectJs);
      if(!_webView->settings()->testAttribute(QWebSettings::JavascriptEnabled)){
        throw std::runtime_error("Javascript is not enabled!");
      }

      QString html = readTextFromFile(":/myWidget/linechart.html");
      _webView->setHtml(html, QUrl("qrc:/myWidget/"));
    }

    void LineChartView::onJsLog(QString text){
      hdi::utils::secureLogValue(_logger,"LineChart",text.toStdString());
    }
    void LineChartView::onJsError(QString text){
      throw std::runtime_error(text.toStdString().c_str());
    }

    void LineChartView::onConnectJs(){
      _mainFrame->addToJavaScriptWindowObject("Qt", this);
    }

    void LineChartView::onWebViewFinishedLoading(bool ok){
      _connection_ready = true;
      _webView->page()->setViewportSize(size());
    }

    LineChartView::~LineChartView(){
    }

    void LineChartView::setMaxX(scalar_type v){
      emit sgnSetMaxX(QString("%1").arg(v));
    }

    void LineChartView::setMinX(scalar_type v){
      emit sgnSetMinX(QString("%1").arg(v));
    }

    void LineChartView::setMaxY(scalar_type v){
      emit sgnSetMaxY(QString("%1").arg(v));
    }

    void LineChartView::setMinY(scalar_type v){
      emit sgnSetMinY(QString("%1").arg(v));
    }

    void LineChartView::setData(const std::vector<std::pair<scalar_type,scalar_type>>& data){
      std::stringstream ss;
      ss << "x,y" << std::endl;
      for(auto& v: data){
        ss << v.first << "," << v.second << std::endl;
      }
      emit sgnSetData(QString::fromStdString(ss.str()));
    }

    void LineChartView::onUpdate(){
      while(!_connection_ready){//it is better to use condition_variable
         QApplication::processEvents();
      }
      emit sgnDrawVisualization();
    }

    void LineChartView::resizeEvent(QResizeEvent * e){
      _webView->resize(size());
      _webView->page()->setViewportSize(size());
      onUpdate();
    }
  }
}
