#ifndef UI_DOCKABLE_WIDGET
#define UI_DOCKABLE_WIDGET

#include <QDockWidget>

class QWidget;
class QGridLayout;

class DockableWidget : public QDockWidget
{
  Q_OBJECT
public:
  explicit DockableWidget(QWidget *parent = 0);
  ~DockableWidget();

  QGridLayout* mainLayout();

public slots :
  void setVisibility(bool visible);
  void toggleVisibility();

protected:

private:

public:

protected:
  bool _isVisible;

  QGridLayout* _mainLayout;

private:
  QWidget* _mainWidget;
};

#endif // UI_DOCKABLE_WIDGET
