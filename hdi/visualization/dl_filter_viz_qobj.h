#ifndef DL_NEURON_VIZ_H
#define DL_NEURON_VIZ_H

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

    class DLFilterViz: public QWidget
    {
      Q_OBJECT
    public:
      typedef float scalar_type;
      typedef uint32_t unsigned_int_type;
      typedef std::pair<unsigned_int_type,scalar_type> timed_scalar_type;


    public:
      explicit DLFilterViz(QWidget *parent = 0);
      ~DLFilterViz();

      //! Return the current log
      utils::AbstractLog* logger()const{return _logger;}
      //! Set a pointer to an existing log
      void setLogger(utils::AbstractLog* logger){_logger = logger;}
      void updateView(){onSetData();onDrawVisualization();}

      void setData(std::vector<timed_scalar_type>& values){_values = values;}
      void setFrequencies(std::vector<scalar_type>& frequencies){_frequencies = frequencies;}
      void setFilterNames(std::vector<std::string>& names){_names = names;}
      void setFilterOrder(std::vector<unsigned_int_type>& order){_order = order;}
      void setFilterFlags(std::vector<unsigned_int_type>& flags){_flags = flags;}
      void setMaxValue(scalar_type v);
      void setMaxViz(){emit sgnSetMaxViz();}
      void setFreqViz(){emit sgnSetFreqViz();}
      void setCurrentIteration(unsigned_int_type current_iteration){_current_iteration = current_iteration;}

    //Slots used by C++
    private slots:
      void onSetData();
      void onDrawVisualization();

    //Slots used by JS
    public slots:
      void onJsLog(QString text);
      void onJsError(QString text);
      void onClickOnFilter(QString id){emit sgnClickOnFilter(std::atoi(id.toStdString().c_str()));}

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
      void sgnDrawVisualization();
      void sgnSetMaxValue(QString);
      void sgnClickOnFilter(int);
      void sgnSetMaxViz();
      void sgnSetFreqViz();

    protected:
      virtual void resizeEvent(QResizeEvent * e);

    private:
      QWebView* _webView;
      QWebFrame* _mainFrame;
      bool _connection_ready;
      std::mutex _mutex;
      std::condition_variable _cv;
      std::thread _connection_thread;

      utils::AbstractLog* _logger;

      std::vector<timed_scalar_type> _values;
      std::vector<scalar_type> _frequencies;
      std::vector<std::string> _names;
      std::vector<unsigned_int_type> _order;
      std::vector<unsigned_int_type> _flags;
      unsigned_int_type _current_iteration;
    };

  }
}


#endif // JS_WIDGET_H
