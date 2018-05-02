#include "sankey_diagram_qobj.h"

#include <QFileDialog>
#include <QLayout>
#include <QtWebKitWidgets/QWebView>
#include <QtWebKitWidgets/QWebFrame>
#include <QtWebKit/QWebElement>
#include <QFile>
#include <QDir>

#include <assert.h>
#include "hdi/utils/log_helper_functions.h"
#include "hdi/utils/file_utils.h"
#include "hdi/utils/assert_by_exception.h"

//call to Q_INIT_RESOURCE should be outside of any namespace
void loadSankeyDiagramResources(){
  Q_INIT_RESOURCE(sankey_diagram_qobj);
}

namespace hdi {
  namespace viz {

    SankeyDiagram::SankeyDiagram(QWidget *parent) :
      _webView(nullptr),
      _mainFrame(nullptr),
      _logger(nullptr),
      _verbose(false),
      _connection_ready(false)
    {
      loadSankeyDiagramResources();
      initUI();
    }

    void SankeyDiagram::initUI(){
      _webView = new QWebView(this);

      connect(_webView, &QWebView::loadFinished, this, &SankeyDiagram::onWebViewFinishedLoading);

      _mainFrame = _webView->page()->mainFrame();
      _mainFrame->setScrollBarPolicy(Qt::Vertical, Qt::ScrollBarAlwaysOff);
      _mainFrame->setScrollBarPolicy(Qt::Horizontal, Qt::ScrollBarAlwaysOff);

      QObject::connect(_mainFrame,&QWebFrame::javaScriptWindowObjectCleared,this,&SankeyDiagram::onConnectJs);
      checkAndThrowRuntime(_webView->settings()->testAttribute(QWebSettings::JavascriptEnabled),"Javascript is not enabled!");

      QString html = utils::readTextFromFile(":/sankey/sankey.html");
      _webView->setHtml(html, QUrl("qrc:/sankey/"));

    }

    void SankeyDiagram::onJsLog(QString text){
      utils::secureLogValue(_logger,"Javascript",text.toStdString());
    }
    void SankeyDiagram::onJsError(QString text){
      throw std::runtime_error(text.toStdString().c_str());
      //utils::secureLogValue(_logger,"Javascript",text.toStdString());
    }

    void SankeyDiagram::selectionUpdated(QList<int> selectedClusters){
    //  _selectionIdxs.clear();
    //  long size = 0;
    //
    //  if (selectedClusters.size() > 0 || _prevSelectionSize > 0)
    //  {
    //    //MCV_CytometryData::Instance()->derivedDataClusters("Spade Tree")->
    //    //std::vector< std::vector< std::pair <int, int> > >* clusters = _analysis->clusters();
    //
    //    for (int i = 0; i < selectedClusters.size(); i++)
    //    {
    //      //size += (*clusters)[selectedClusters[i]].size();
    //    }
    //
    //    _selectionIdxs.reserve(size);
    //
    ////    for (int i = 0; i < selectedClusters.size(); i++)
    ////    {
    ////      int clusterId = selectedClusters[i];
    ////      //_selectionIdxs.insert(_selectionIdxs.end(), (*clusters)[clusterId].begin(), (*clusters)[clusterId].end());
    ////
    ////    }
    //
    //    assert(_selectionIdxs.size() == size);
    //
    //    for (int i = 0; i < _linkedViews.size(); i++)
    //    {
    ////      _linkedViews[i]->select(&_selectionIdxs);
    //    }
    //
    //    _prevSelectionSize = size;
    //  }
    }


    void SankeyDiagram::resizeEvent(QResizeEvent* event){
    }

    void SankeyDiagram::dataRefreshed(void* data){
      //  std::cout << "Refreshed item pointer = " << data << "\n";
      //  std::cout << "Local item pointer = " << (void*)_data << "\n";

      //if ((void*)_data != data) return;

      //qDebug() << _data->jsonObject()->c_str();
      //qDebug() << "setting data in heatmap view";
      //qt_setData(QString(_data->jsonObject()->c_str()));
    }

    void SankeyDiagram::selectionRefreshed(void* data){
      //if ((void*)_data != data) return;

      std::vector<bool> sel; //= *(_data->selection());

      _selection.clear();
      _selection.reserve(sel.size());

      for (int i = 0; i < sel.size(); i++)
      {
        _selection.append(sel[i] ? 1 : 0);
      }

      //qt_setSelection(_selection);
    }

    void SankeyDiagram::onConnectJs(){
      _mainFrame->addToJavaScriptWindowObject("Qt", this);
      utils::secureLog(_logger,"Connecting...",_verbose);
    }

    void SankeyDiagram::onWebViewFinishedLoading(bool ok){
      utils::secureLog(_logger,"Sankey diagram: web view finished loading...",_verbose);
      _connection_ready = true;
    }

    SankeyDiagram::~SankeyDiagram(){
    }

  }
}
