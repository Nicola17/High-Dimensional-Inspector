#include "dockable_widget_qobj.h"

#include <QLayout>
#include <QFile>
#include <QTextStream>
#include <QObject>
#include <QAction>

DockableWidget::DockableWidget(QWidget *parent) :
  QDockWidget(parent),
  _isVisible(false),
  _mainLayout(nullptr),
  _mainWidget(nullptr)
{
  _mainWidget = new QWidget(this);
  _mainLayout = new QGridLayout(_mainWidget);
  _mainLayout->setMargin(0);
  setWidget(_mainWidget);

  QObject::connect(toggleViewAction(), SIGNAL(toggled(bool)), this, SLOT(setVisibility(bool)));
}

QGridLayout* DockableWidget::mainLayout()
{
  return _mainLayout;
}

void DockableWidget::setVisibility(bool visible)
{
  _isVisible = visible;
  setVisible(_isVisible);
}

void DockableWidget::toggleVisibility()
{
  setVisibility(!_isVisible);
}

DockableWidget::~DockableWidget()
{
  if (_mainLayout) delete _mainLayout;
  if (_mainWidget) delete _mainWidget;
}
