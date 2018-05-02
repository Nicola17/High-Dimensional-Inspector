#include "scalable_pcp_view_qobj.h"

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
void loadScalablePCPViewResources(){
  Q_INIT_RESOURCE(scalable_pcp_view_qobj);
}

namespace hdi {
  namespace viz {

    ScalablePCPView::ScalablePCPView(QWidget *parent):
      _webView(nullptr),
      _mainFrame(nullptr),
      _connection_ready(false),
      _logger(nullptr)
    {
      loadScalablePCPViewResources();
      initUI();
    }

    QString ScalablePCPView::readTextFromFile(QString filename){
      QFile file(filename);
      if (!file.open(QIODevice::ReadOnly | QIODevice::Text)){
        std::stringstream ss;
        ss << "Unable to open the file: " << filename.toStdString();
        throw std::logic_error(ss.str());
      }
      QTextStream in(&file);
      return in.readAll();
    }

    void ScalablePCPView::initUI(){
      _webView = new QWebView(this);

      connect(_webView, &QWebView::loadFinished, this, &ScalablePCPView::onWebViewFinishedLoading);

      _mainFrame = _webView->page()->mainFrame();
      _mainFrame->setScrollBarPolicy(Qt::Vertical, Qt::ScrollBarAlwaysOff);
      _mainFrame->setScrollBarPolicy(Qt::Horizontal, Qt::ScrollBarAlwaysOff);

      QObject::connect(_mainFrame,&QWebFrame::javaScriptWindowObjectCleared,this,&ScalablePCPView::onConnectJs);
      if(!_webView->settings()->testAttribute(QWebSettings::JavascriptEnabled)){
        throw std::runtime_error("Javascript is not enabled!");
      }

      QString html = readTextFromFile(":/myWidget/scalable_pcp.html");
      _webView->setHtml(html, QUrl("qrc:/myWidget/"));
    }

    void ScalablePCPView::onJsLog(QString text){
      hdi::utils::secureLogValue(_logger,"ScalablePCP",text.toStdString());
    }
    void ScalablePCPView::onJsError(QString text){
      throw std::runtime_error(text.toStdString().c_str());
    }

    void ScalablePCPView::onConnectJs(){
      _mainFrame->addToJavaScriptWindowObject("Qt", this);
    }

    void ScalablePCPView::onWebViewFinishedLoading(bool ok){
      _connection_ready = true;
      _webView->page()->setViewportSize(size());
    }

    ScalablePCPView::~ScalablePCPView(){
    }

    void ScalablePCPView::setMaxY(scalar_type v){
      emit sgnSetMaxY(QString("%1").arg(v));
    }

    void ScalablePCPView::setMinY(scalar_type v){
      emit sgnSetMinY(QString("%1").arg(v));
    }

    void ScalablePCPView::addDataPointAndDraw(const std::vector<scalar_type>& data, QString color, float opacity) {
      std::stringstream ss;
      ss << "value" << std::endl;
      for(auto& v: data){
        ss << v << std::endl;
      }
      emit sgnAddDataPointAndDraw(QString::fromStdString(ss.str()), color, opacity);
    }

    void ScalablePCPView::onUpdate(){
      while(!_connection_ready){//it is better to use condition_variable
         QApplication::processEvents();
      }
      emit sgnDrawVisualization();
    }

    void ScalablePCPView::resizeEvent(QResizeEvent * e){
      _webView->resize(size());
      _webView->page()->setViewportSize(size());
      onUpdate();
    }
  }
}

