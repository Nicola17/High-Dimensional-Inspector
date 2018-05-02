#include "word_cloud_qobj.h"

#include <QFileDialog>
#include <QLayout>
#include <QtWebKitWidgets/QWebView>
#include <QtWebKitWidgets/QWebFrame>
#include <QtWebKit/QWebElement>
#include <QFile>
#include <QDir>
#include <sstream>
#include <iostream>
#include "hdi/data/text_data.h"


//call to Q_INIT_RESOURCE should be outside of any namespace
void loadWordCloudResources(){
  Q_INIT_RESOURCE(word_cloud_qobj);
}

namespace hdi {
  namespace viz {

    WordCloud::WordCloud(QWidget *parent):
      _webView(nullptr),
      _mainFrame(nullptr),
      _connection_ready(false)
    {
      loadWordCloudResources();
      initUI();
    }

    QString WordCloud::readTextFromFile(QString filename){
      QFile file(filename);
      if (!file.open(QIODevice::ReadOnly | QIODevice::Text)){
        std::stringstream ss;
        ss << "Unable to open the file: " << filename.toStdString();
        throw std::logic_error(ss.str());
      }
      QTextStream in(&file);
      return in.readAll();
    }

    void WordCloud::initUI(){
      _webView = new QWebView(this);

      connect(_webView, &QWebView::loadFinished, this, &WordCloud::onWebViewFinishedLoading);

      _mainFrame = _webView->page()->mainFrame();
      _mainFrame->setScrollBarPolicy(Qt::Vertical, Qt::ScrollBarAlwaysOff);
      _mainFrame->setScrollBarPolicy(Qt::Horizontal, Qt::ScrollBarAlwaysOff);

      QObject::connect(_mainFrame,&QWebFrame::javaScriptWindowObjectCleared,this,&WordCloud::onConnectJs);
      if(!_webView->settings()->testAttribute(QWebSettings::JavascriptEnabled)){
        throw std::runtime_error("Javascript is not enabled!");
      }

      QString html = readTextFromFile(":/word_cloud/word_cloud.html");
      _webView->setHtml(html, QUrl("qrc:/word_cloud/"));
    }

    void WordCloud::onJsLog(QString text){
      //QTextStream(stdout)  << "JS Log:\t" << text << endl;
    }
    void WordCloud::onJsError(QString text){
      throw std::runtime_error(text.toStdString().c_str());
    }

    void WordCloud::onConnectJs(){
      _mainFrame->addToJavaScriptWindowObject("Qt", this);
    }

    void WordCloud::onWebViewFinishedLoading(bool ok){
      _connection_ready = true;
      _webView->page()->setViewportSize(size());
    }

    WordCloud::~WordCloud(){
    }

    void WordCloud::onSendData(){
      std::stringstream ss;
      ss << "[";
      auto it = _words.begin();
      for(int i = 0; i < _words.size(); ++i, ++it){
        ss << "{\"text\": \"" << it->first << "\",\"size\":" << it->second << "}";
        if(i != _words.size()-1){
          ss << ",";
        }

      }
      ss << "]";
      while(!_connection_ready){//it is better to use condition_variable
         QApplication::processEvents();
      }
      std::string str = ss.str();
      emit sgnSendData(QString::fromStdString(str));
      QTextStream(stdout)  << str.c_str() << endl;
    }

    void WordCloud::resizeEvent(QResizeEvent * e){
      _webView->resize(size());
      _webView->page()->setViewportSize(size());
      //updateView();
    }
  }
}

