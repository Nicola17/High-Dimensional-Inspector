#ifndef UI_HEAT_MAP_WIDGET
#define UI_HEAT_MAP_WIDGET

#include <QWidget>
#include <QApplication>
#include "hdi/utils/abstract_log.h"
#include "hdi/data/flow_model.h"
#include "hdi/utils/log_helper_functions.h"
#include "hdi/utils/assert_by_exception.h"
#include "hdi/utils/timing_utils.h"

class QLayout;
class QWebView;
class QWebFrame;
class QResizeEvent;

namespace hdi {
  namespace viz {

    class SankeyDiagram: public QWidget
    {
      Q_OBJECT
    public:
      explicit SankeyDiagram(QWidget *parent = 0);
      ~SankeyDiagram();

      //! Return the current log
      utils::AbstractLog* logger()const{return _logger;}
      //! Set a pointer to an existing log
      void setLogger(utils::AbstractLog* logger){_logger = logger;}
      //! Is verbose?
      bool isVerbose()const{return _verbose;}
      //! Set a pointer to an existing log
      void setVerbose(bool verbose){_verbose = verbose;}
      //! Change the visualization based on the model
      template <class Traits>
      void visualizeFlow(data::FlowModel<Traits>& model);

    private:
      void initUI();
      void resizeEvent(QResizeEvent* event);

    signals:
      void sgnSetData(QString link_csv, QString node_csv);
      void sgnClearCanvas();

    public slots:
      void onWebViewFinishedLoading(bool ok);
      void onConnectJs();
      void onJsLog(QString text);
      void onJsError(QString text);


      void selectionUpdated(QList<int> selectedClusters);//TOREMOVE
      void dataRefreshed(void* data);//TOREMOVE
      void selectionRefreshed(void* data);//TOREMOVE

    public:

    private:
      QWebView* _webView;
      QWebFrame* _mainFrame;

      QList<int> _selection;//TOREMOVE

      utils::AbstractLog* _logger;
      bool _verbose;
      bool _connection_ready;
    };

    template <class Traits>
    void SankeyDiagram::visualizeFlow(data::FlowModel<Traits>& model){
      utils::secureLog(_logger,"Visualizing flow...",_verbose);
      while(!_connection_ready){//OMG are you really doing this!? :P
        utils::sleepFor<utils::Milliseconds>(10);
        QApplication::processEvents();
      }

      std::stringstream ss_links;
      ss_links << Traits::flow_type::getCSVHeader().toStdString() << "\n";
      for(auto link: model.flows()){
        ss_links << link.getCSVValues().toStdString() << "\n";
      }

      std::stringstream ss_nodes;
      ss_nodes << Traits::node_type::getCSVHeader().toStdString() << "\n";
      for(auto node: model.nodes()){
        ss_nodes<< node.getCSVValues().toStdString() << "\n";
      }

      sgnClearCanvas();
      sgnSetData(ss_links.str().c_str(),ss_nodes.str().c_str());
    }

  }
}


#endif // UI_HEAT_MAP_WIDGET
