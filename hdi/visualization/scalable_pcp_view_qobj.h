#ifndef SCALABLE_PCP_VIEW_H
#define SCALABLE_PCP_VIEW_H

#include <QWidget>
#include <QApplication>
#include <utility>
#include <mutex>
#include <condition_variable>
#include <thread>
#include "hdi/visualization/abstract_view.h"
#include "hdi/utils/abstract_log.h"

class QLayout;
class QWebView;
class QWebFrame;
class QResizeEvent;

namespace hdi {
  namespace viz {

    class ScalablePCPView: public QWidget
    {
      Q_OBJECT

    public:
      typedef float scalar_type;
      enum class sorting_type {NO_SORTING, AVG_SORTING, STD_DEV_SORTING};

    public:
      explicit ScalablePCPView(QWidget *parent = 0);
      ~ScalablePCPView();

      virtual QWidget* widgetPtr(){ return this; }
      virtual const QWidget* widgetPtr()const{ return this; }

      //! Return the current log
      utils::AbstractLog* logger()const{return _logger;}
      //! Set a pointer to an existing log
      void setLogger(utils::AbstractLog* logger){_logger = logger;}

      void setMaxY(scalar_type v);
      void setMinY(scalar_type v);
      void setNumDims(int v){emit sgnSetNumDims(QString("%1").arg(v));}
      void setHorTicks(QStringList marks) { emit sgnSetHorTicks(marks); }
      void addDataPointAndDraw(const std::vector<scalar_type>& data, QString color = "#000000", float opacity = 0.15);
      void resetAndDraw(){emit sgnResetDataAndDraw("");}

      void setSorting(sorting_type sorting){_sorting = sorting;}

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

    //JS connection handling
    private slots:
      void onWebViewFinishedLoading(bool ok);
      void onConnectJs();

    signals:
      void sgnAddDataPointAndDraw(QString, QString, float);
      void sgnResetDataAndDraw(QString);
      void sgnSetMaxY(QString);
      void sgnSetMinY(QString);
      void sgnSetNumDims(QString);
      void sgnSetHorTicks(QStringList);
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

      sorting_type _sorting;
    };

  }
}


#endif // JS_WIDGET_H
