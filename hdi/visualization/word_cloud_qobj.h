#ifndef WORD_CLOUD_H
#define WORD_CLOUD_H

#include <QWidget>
#include <QApplication>
#include <utility>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <unordered_map>
#include "hdi/visualization/abstract_view.h"

class QLayout;
class QWebView;
class QWebFrame;
class QResizeEvent;

namespace hdi {
  namespace viz {

    class WordCloud: public QWidget
    {
      Q_OBJECT
    public:
      typedef std::unordered_map<std::string,uint32_t> words_count_type;
    public:
      explicit WordCloud(QWidget *parent = 0);
      ~WordCloud();

      virtual QWidget* widgetPtr(){ return this; }
      virtual const QWidget* widgetPtr()const{ return this; }
      virtual void updateView(){onSendData();}

      void setWords(const words_count_type& words){_words = words;}

    //Slots used by C++
    private slots:
      void onSendData();

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
      void sgnSendData(QString);

    protected:
      virtual void resizeEvent(QResizeEvent * e);

    private:
      QWebView* _webView;
      QWebFrame* _mainFrame;
      bool _connection_ready;
      std::mutex _mutex;
      std::condition_variable _cv;
      std::thread _connection_thread;
      words_count_type _words;
    };

  }
}


#endif // JS_WIDGET_H
