#ifndef HISTOGRAM_ZCENT_VIEW_H
#define HISTOGRAM_ZCENT_VIEW_H

#include <QWidget>
#include <QApplication>
#include <utility>
#include <mutex>
#include <condition_variable>
#include <thread>
#include "hdi/visualization/abstract_view.h"
#include "hdi/utils/abstract_log.h"
#include "hdi/data/histogram.h"

class QLayout;
class QWebView;
class QWebFrame;
class QResizeEvent;

namespace hdi {
  namespace viz {

    class HistogramZCentView: public QWidget
    {
      Q_OBJECT
    public:
      typedef float scalar_type;

    public:
      explicit HistogramZCentView(QWidget *parent = 0);
      ~HistogramZCentView();

      virtual QWidget* widgetPtr(){ return this; }
      virtual const QWidget* widgetPtr()const{ return this; }

      //! Return the current log
      utils::AbstractLog* logger()const{return _logger;}
      //! Set a pointer to an existing log
      void setLogger(utils::AbstractLog* logger){_logger = logger;}
      void setData(const data::Histogram<scalar_type>& data);
      void setMax(scalar_type max){emit sgnSetMax(QString("%1").arg(max));}

    //Slots used by C++
    public slots:
      void onUpdate();

    //Slots used by JS
    public slots:
      void onJsLog(QString text);
      void onJsError(QString text);

    private:
      void initUI();
      //! Load the html page
      QString readTextFromFile(QString filename);
      void asyncSetData(std::vector<std::pair<scalar_type,scalar_type>> points);

    //JS connection handling
    private slots:
      void onWebViewFinishedLoading(bool ok);
      void onConnectJs();

    signals:
      void sgnSetData(QString);
      void sgnSetMax(QString);
      void sgnDrawVisualization();

    protected:
      virtual void resizeEvent(QResizeEvent * e);

    private:
      utils::AbstractLog* _logger;
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
