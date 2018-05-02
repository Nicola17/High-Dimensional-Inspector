#ifndef HEATMAP_VIEW_H
#define HEATMAP_VIEW_H

#include <QWidget>
#include <QApplication>
#include <utility>
#include <mutex>
#include <condition_variable>
#include <thread>
#include "hdi/visualization/abstract_view.h"

class QLayout;
class QWebView;
class QWebFrame;
class QResizeEvent;

namespace hdi {
  namespace viz {

    class HeatMapView: public QWidget, public AbstractView
    {
      Q_OBJECT
    public:
      explicit HeatMapView(QWidget *parent = 0);
      ~HeatMapView();

      virtual QWidget* widgetPtr(){ return this; }
      virtual const QWidget* widgetPtr()const{ return this; }
      virtual void updateView(){onSetData();}

    //Slots used by C++
    private slots:
      void onSetData();

    //Slots used by JS
    public slots:
      void onJsLog(QString text);
      void onJsError(QString text);

    private:
      void initUI();
      QString readTextFromFile(QString filename);
      void asyncSetData(std::vector<std::pair<double,double>> points);

    //JS connection handling
    private slots:
      void onWebViewFinishedLoading(bool ok);
      void onConnectJs();

    signals:
      void sgnSetData(QString);
      void sgnSetMaxXElements(QString);
      void sgnSetMaxYElements(QString);
      void sgnSetXLabels(QString);
      void sgnSetYLabels(QString);

    protected:
      virtual void resizeEvent(QResizeEvent * e);

    private:
      QWebView* _webView;
      QWebFrame* _mainFrame;
      bool _connection_ready;
      std::mutex _mutex;
      std::condition_variable _cv;
      std::thread _connection_thread;
    };

  }
}


#endif // JS_WIDGET_H
