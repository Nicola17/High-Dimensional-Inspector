#include "linechart_dual_view_qobj.h"

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
void loadLineChartDualViewResources(){
  Q_INIT_RESOURCE(linechart_dual_view_qobj);
}

namespace hdi {
  namespace viz {

    LineChartDualView::LineChartDualView(QWidget *parent):
      _webView(nullptr),
      _mainFrame(nullptr),
      _connection_ready(false),
      _logger(nullptr)
    {
      loadLineChartDualViewResources();
      initUI();
    }

    QString LineChartDualView::readTextFromFile(QString filename){
      QFile file(filename);
      if (!file.open(QIODevice::ReadOnly | QIODevice::Text)){
        std::stringstream ss;
        ss << "Unable to open the file: " << filename.toStdString();
        throw std::logic_error(ss.str());
      }
      QTextStream in(&file);
      return in.readAll();
    }

    void LineChartDualView::initUI(){
      _webView = new QWebView(this);

      connect(_webView, &QWebView::loadFinished, this, &LineChartDualView::onWebViewFinishedLoading);

      _mainFrame = _webView->page()->mainFrame();
      _mainFrame->setScrollBarPolicy(Qt::Vertical, Qt::ScrollBarAlwaysOff);
      _mainFrame->setScrollBarPolicy(Qt::Horizontal, Qt::ScrollBarAlwaysOff);

      QObject::connect(_mainFrame,&QWebFrame::javaScriptWindowObjectCleared,this,&LineChartDualView::onConnectJs);
      if(!_webView->settings()->testAttribute(QWebSettings::JavascriptEnabled)){
        throw std::runtime_error("Javascript is not enabled!");
      }

      QString html = readTextFromFile(":/myWidget/linechart_dual.html");
      _webView->setHtml(html, QUrl("qrc:/myWidget/"));
    }

    void LineChartDualView::onJsLog(QString text){
      hdi::utils::secureLogValue(_logger,"LineChart",text.toStdString());
    }
    void LineChartDualView::onJsError(QString text){
      throw std::runtime_error(text.toStdString().c_str());
    }

    void LineChartDualView::onConnectJs(){
      _mainFrame->addToJavaScriptWindowObject("Qt", this);
    }

    void LineChartDualView::onWebViewFinishedLoading(bool ok){
      _connection_ready = true;
      _webView->page()->setViewportSize(size());
    }

    LineChartDualView::~LineChartDualView(){
    }

    void LineChartDualView::setMaxX(scalar_type v){
      emit sgnSetMaxX(QString("%1").arg(v));
    }

    void LineChartDualView::setMinX(scalar_type v){
      emit sgnSetMinX(QString("%1").arg(v));
    }

    void LineChartDualView::setMaxY(scalar_type v){
      emit sgnSetMaxY(QString("%1").arg(v));
    }

    void LineChartDualView::setMinY(scalar_type v){
      emit sgnSetMinY(QString("%1").arg(v));
    }

    void LineChartDualView::setDualMaxY(scalar_type v){
      emit sgnSetDualMaxY(QString("%1").arg(v));
    }

    void LineChartDualView::setDualMinY(scalar_type v){
      emit sgnSetDualMinY(QString("%1").arg(v));
    }

    void LineChartDualView::setData(const std::vector<std::pair<scalar_type,scalar_type>>& data){
      std::stringstream ss;
      ss << "x,y" << std::endl;
      for(auto& v: data){
        ss << v.first << "," << v.second << std::endl;
      }
      emit sgnSetData(QString::fromStdString(ss.str()));
    }
    void LineChartDualView::setDualData(const std::vector<std::pair<scalar_type,scalar_type>>& data){
      std::stringstream ss;
      ss << "x,y" << std::endl;
      for(auto& v: data){
        ss << v.first << "," << v.second << std::endl;
      }
      emit sgnSetDualData(QString::fromStdString(ss.str()));
    }

    void LineChartDualView::onUpdate(){
      while(!_connection_ready){//it is better to use condition_variable
         QApplication::processEvents();
      }
      emit sgnDrawVisualization();
    }

    void LineChartDualView::resizeEvent(QResizeEvent * e){
      _webView->resize(size());
      _webView->page()->setViewportSize(size());
      onUpdate();
    }
  }
}

