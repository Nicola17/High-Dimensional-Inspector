#include "heatmap_view_qobj.h"

#include <QFileDialog>
#include <QLayout>
#include <QtWebKitWidgets/QWebView>
#include <QtWebKitWidgets/QWebFrame>
#include <QtWebKit/QWebElement>
#include <QFile>
#include <QDir>
#include <sstream>
#include <iostream>
#include <cmath>
#include "hdi/data/text_data.h"


//call to Q_INIT_RESOURCE should be outside of any namespace
void loadHeatMapViewResources(){
  Q_INIT_RESOURCE(heatmap_view_qobj);
}

namespace hdi {
  namespace viz {

    HeatMapView::HeatMapView(QWidget *parent):
      _webView(nullptr),
      _mainFrame(nullptr),
      _connection_ready(false)
    {
      loadHeatMapViewResources();
      initUI();
    }

    QString HeatMapView::readTextFromFile(QString filename){
      QFile file(filename);
      if (!file.open(QIODevice::ReadOnly | QIODevice::Text)){
        std::stringstream ss;
        ss << "Unable to open the file: " << filename.toStdString();
        throw std::logic_error(ss.str());
      }
      QTextStream in(&file);
      return in.readAll();
    }

    void HeatMapView::initUI(){
      _webView = new QWebView(this);

      connect(_webView, &QWebView::loadFinished, this, &HeatMapView::onWebViewFinishedLoading);

      _mainFrame = _webView->page()->mainFrame();
      _mainFrame->setScrollBarPolicy(Qt::Vertical, Qt::ScrollBarAlwaysOff);
      _mainFrame->setScrollBarPolicy(Qt::Horizontal, Qt::ScrollBarAlwaysOff);

      QObject::connect(_mainFrame,&QWebFrame::javaScriptWindowObjectCleared,this,&HeatMapView::onConnectJs);
      if(!_webView->settings()->testAttribute(QWebSettings::JavascriptEnabled)){
        throw std::runtime_error("Javascript is not enabled!");
      }

      QString html = readTextFromFile(":/myWidget/heatmap.html");
      _webView->setHtml(html, QUrl("qrc:/myWidget/"));
    }

    void HeatMapView::onJsLog(QString text){
      QTextStream(stdout)  << "JS Log:\t" << text << endl;
    }
    void HeatMapView::onJsError(QString text){
      throw std::runtime_error(text.toStdString().c_str());
    }

    void HeatMapView::onConnectJs(){
      _mainFrame->addToJavaScriptWindowObject("Qt", this);
    }

    void HeatMapView::onWebViewFinishedLoading(bool ok){
      _connection_ready = true;
      _webView->page()->setViewportSize(size());
    }

    HeatMapView::~HeatMapView(){
    }

    void HeatMapView::onSetData(){
      typedef float scalar_type;
      std::stringstream ss;
      ss << "x_label,y_label,value,confidence" << std::endl;

      std::vector<scalar_type> avg(_panel_data->numDimensions(),0);
      std::vector<scalar_type> std_dev(_panel_data->numDimensions(),0);
      std::vector<scalar_type> max(_panel_data->numDimensions(),0);
      int selected_pnts = 0;

      const auto& flags = _panel_data->getFlagsDataPoints();


      for(int i = 0; i < _panel_data->numDataPoints(); ++i){
        if((flags[i] & panel_data_type::Selected) == panel_data_type::Selected){
          for(int d = 0; d < _panel_data->numDimensions(); ++d){
            auto v = _panel_data->dataAt(i,d);
            avg[d] += v;
            std_dev[d] += v*v;
            max[d] = std::max<scalar_type>(max[d],v);
          }
          ++selected_pnts;
        }
      }
      if(selected_pnts){
        for(int d = 0; d < _panel_data->numDimensions(); ++d){
          avg[d] /= selected_pnts;
          std_dev[d] /= selected_pnts;
          std_dev[d] = std::sqrt(std_dev[d]-avg[d]*avg[d]);
        }
      }


      for(int d = 0; d < _panel_data->numDimensions(); ++d){
        ss << "1," << d+1 << "," << avg[d] << ",1" << std::endl;
      }

      while(!_connection_ready){//it is better to use condition_variable
         QApplication::processEvents();
       }

      std::stringstream ss_labels;
      ss_labels << "label" << std::endl;
      for(int d = 0; d < _panel_data->numDimensions(); ++d){
        auto dim_ptr = dynamic_cast<data::TextData*>(_panel_data->getDimensions()[d].get());
        if(dim_ptr == nullptr){
          ss_labels << "Dim" << d << std::endl;
        }else{
          ss_labels << dim_ptr->text() << std::endl;
        }

      }

      emit sgnSetMaxXElements("1");
      emit sgnSetMaxYElements(QString("%1").arg(_panel_data->numDimensions()));
      emit sgnSetXLabels("label\nSel");
      emit sgnSetYLabels(QString::fromStdString(ss_labels.str()));
      emit sgnSetData(QString::fromStdString(ss.str()));
    }

    void HeatMapView::resizeEvent(QResizeEvent * e){
      _webView->resize(size());
      _webView->page()->setViewportSize(size());
      updateView();
    }
  }
}
