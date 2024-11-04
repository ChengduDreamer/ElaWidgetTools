#include "ElaSimpleWindow.h"

#include <QApplication>
#include <QDockWidget>
#include <QHBoxLayout>
#include <QPropertyAnimation>
#include <QResizeEvent>
#include <QScreen>
#include <QStackedWidget>
#include <QStyleOption>
#include <QTimer>
#include <QToolBar>
#include <QVBoxLayout>

#include "ElaApplication.h"
#include "ElaCentralStackedWidget.h"
#include "ElaEventBus.h"
#include "ElaMenu.h"
#include "ElaNavigationBar.h"
#include "ElaNavigationRouter.h"
#include "ElaTheme.h"
#include "ElaWindowStyle.h"
#include "private/ElaAppBarPrivate.h"
#include "private/ElaNavigationBarPrivate.h"
#include "private/ElaSimpleWindowPrivate.h"
Q_PROPERTY_CREATE_Q_CPP(ElaSimpleWindow, int, ThemeChangeTime)
Q_PROPERTY_CREATE_Q_CPP(ElaSimpleWindow, ElaNavigationType::NavigationDisplayMode, NavigationBarDisplayMode)
Q_TAKEOVER_NATIVEEVENT_CPP(ElaSimpleWindow, d_func()->_appBar);
ElaSimpleWindow::ElaSimpleWindow(QWidget* parent)
    : QMainWindow{parent}, d_ptr(new ElaSimpleWindowPrivate())
{
    Q_D(ElaSimpleWindow);
    d->q_ptr = this;

    setProperty("ElaBaseClassName", "ElaSimpleWindow");
    resize(1020, 680); // 默认宽高

    d->_pThemeChangeTime = 700;
    d->_pNavigationBarDisplayMode = ElaNavigationType::NavigationDisplayMode::Auto;
    connect(this, &ElaSimpleWindow::pNavigationBarDisplayModeChanged, d, &ElaSimpleWindowPrivate::onDisplayModeChanged);

    // 自定义AppBar
    d->_appBar = new ElaAppBar(this);
    connect(d->_appBar, &ElaAppBar::routeBackButtonClicked, this, []() {
        ElaNavigationRouter::getInstance()->navigationRouteBack();
    });
    connect(d->_appBar, &ElaAppBar::closeButtonClicked, this, &ElaSimpleWindow::closeButtonClicked);
    // 导航栏
    d->_navigationBar = new ElaNavigationBar(this);
    // 返回按钮状态变更
    connect(ElaNavigationRouter::getInstance(), &ElaNavigationRouter::navigationRouterStateChanged, this, [d](bool isEnable) {
        d->_appBar->setRouteBackButtonEnable(isEnable);
    });

    // 转发用户卡片点击信号
    connect(d->_navigationBar, &ElaNavigationBar::userInfoCardClicked, this, &ElaSimpleWindow::userInfoCardClicked);
    // 转发点击信号
    connect(d->_navigationBar, &ElaNavigationBar::navigationNodeClicked, this, &ElaSimpleWindow::navigationNodeClicked);
    //跳转处理
    connect(d->_navigationBar, &ElaNavigationBar::navigationNodeClicked, d, &ElaSimpleWindowPrivate::onNavigationNodeClicked);
    //新增窗口
    connect(d->_navigationBar, &ElaNavigationBar::navigationNodeAdded, d, &ElaSimpleWindowPrivate::onNavigationNodeAdded);

    // 中心堆栈窗口
    d->_centerStackedWidget = new ElaCentralStackedWidget(this);
    d->_centerStackedWidget->setContentsMargins(0, 0, 0, 0);
    QWidget* centralWidget = new QWidget(this);
    d->_centerLayout = new QHBoxLayout(centralWidget);
    d->_centerLayout->setSpacing(0);
    d->_centerLayout->addWidget(d->_navigationBar);
    d->_centerLayout->addWidget(d->_centerStackedWidget);
    d->_centerLayout->setContentsMargins(d->_contentsMargins, 0, 0, 0);

    // 事件总线
    d->_focusEvent = new ElaEvent("WMWindowClicked", "onWMWindowClickedEvent", d);
    d->_focusEvent->registerAndInit();

    // 展开导航栏
    connect(d->_appBar, &ElaAppBar::navigationButtonClicked, d, &ElaSimpleWindowPrivate::onNavigationButtonClicked);

    // 主题变更动画
    d->_themeMode = eTheme->getThemeMode();
    connect(eTheme, &ElaTheme::themeModeChanged, d, &ElaSimpleWindowPrivate::onThemeModeChanged);
    connect(d->_appBar, &ElaAppBar::themeChangeButtonClicked, d, &ElaSimpleWindowPrivate::onThemeReadyChange);
    d->_isInitFinished = true;
    setCentralWidget(centralWidget);
    centralWidget->installEventFilter(this);

    setObjectName("ElaSimpleWindow");
    setStyleSheet("#ElaSimpleWindow{background-color:transparent;}");
    setStyle(new ElaWindowStyle(style()));

    //延时渲染
    QTimer::singleShot(1, this, [=] {
        QPalette palette = this->palette();
        palette.setBrush(QPalette::Window, ElaThemeColor(d->_themeMode, WindowBase));
        this->setPalette(palette);
    });
    eApp->syncMica(this);
    connect(eApp, &ElaApplication::pIsEnableMicaChanged, this, [=]() {
        d->onThemeModeChanged(d->_themeMode);
    });
}

ElaSimpleWindow::~ElaSimpleWindow()
{
}

void ElaSimpleWindow::setIsStayTop(bool isStayTop)
{
    Q_D(ElaSimpleWindow);
    d->_appBar->setIsStayTop(isStayTop);
    Q_EMIT pIsStayTopChanged();
}

bool ElaSimpleWindow::getIsStayTop() const
{
    return d_ptr->_appBar->getIsStayTop();
}

void ElaSimpleWindow::setIsFixedSize(bool isFixedSize)
{
    Q_D(ElaSimpleWindow);
    d->_appBar->setIsFixedSize(isFixedSize);
    Q_EMIT pIsFixedSizeChanged();
}

bool ElaSimpleWindow::getIsFixedSize() const
{
    return d_ptr->_appBar->getIsFixedSize();
}

void ElaSimpleWindow::setIsDefaultClosed(bool isDefaultClosed)
{
    Q_D(ElaSimpleWindow);
    d->_appBar->setIsDefaultClosed(isDefaultClosed);
    Q_EMIT pIsDefaultClosedChanged();
}

bool ElaSimpleWindow::getIsDefaultClosed() const
{
    return d_ptr->_appBar->getIsDefaultClosed();
}

void ElaSimpleWindow::setAppBarHeight(int appBarHeight)
{
    Q_D(ElaSimpleWindow);
    d->_appBar->setAppBarHeight(appBarHeight);
    Q_EMIT pAppBarHeightChanged();
}

QWidget* ElaSimpleWindow::getCustomWidget() const
{
    Q_D(const ElaSimpleWindow);
    return d->_appBar->getCustomWidget();
}

void ElaSimpleWindow::setCustomWidgetMaximumWidth(int width)
{
    Q_D(ElaSimpleWindow);
    d->_appBar->setCustomWidgetMaximumWidth(width);
    Q_EMIT pCustomWidgetMaximumWidthChanged();
}

int ElaSimpleWindow::getCustomWidgetMaximumWidth() const
{
    Q_D(const ElaSimpleWindow);
    return d->_appBar->getCustomWidgetMaximumWidth();
}

void ElaSimpleWindow::setIsCentralStackedWidgetTransparent(bool isTransparent)
{
    Q_D(ElaSimpleWindow);
    d->_centerStackedWidget->setIsTransparent(isTransparent);
}

bool ElaSimpleWindow::getIsCentralStackedWidgetTransparent() const
{
    Q_D(const ElaSimpleWindow);
    return d->_centerStackedWidget->getIsTransparent();
}

void ElaSimpleWindow::moveToCenter()
{
    if (isMaximized() || isFullScreen())
    {
        return;
    }
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    auto geometry = screen()->availableGeometry();
#else
    auto geometry = qApp->screenAt(this->geometry().center())->geometry();
#endif
    setGeometry((geometry.left() + geometry.right() - width()) / 2, (geometry.top() + geometry.bottom() - height()) / 2, width(), height());
}

void ElaSimpleWindow::setCustomWidget(ElaAppBarType::CustomArea customArea, QWidget* widget)
{
    Q_D(ElaSimpleWindow);
    d->_appBar->setCustomWidget(customArea, widget);
    Q_EMIT customWidgetChanged();
}

int ElaSimpleWindow::getAppBarHeight() const
{
    Q_D(const ElaSimpleWindow);
    return d->_appBar->getAppBarHeight();
}

void ElaSimpleWindow::setIsNavigationBarEnable(bool isVisible)
{
    Q_D(ElaSimpleWindow);
    d->_isNavigationEnable = isVisible;
    d->_navigationBar->setVisible(isVisible);
    d->_centerLayout->setContentsMargins(isVisible ? d->_contentsMargins : 0, 0, 0, 0);
    d->_centerStackedWidget->setIsHasRadius(isVisible);
}

bool ElaSimpleWindow::getIsNavigationBarEnable() const
{
    return d_ptr->_isNavigationEnable;
}

void ElaSimpleWindow::setUserInfoCardVisible(bool isVisible)
{
    Q_D(ElaSimpleWindow);
    d->_navigationBar->setUserInfoCardVisible(isVisible);
}

void ElaSimpleWindow::setUserInfoCardPixmap(QPixmap pix)
{
    Q_D(ElaSimpleWindow);
    d->_navigationBar->setUserInfoCardPixmap(pix);
}

void ElaSimpleWindow::setUserInfoCardTitle(QString title)
{
    Q_D(ElaSimpleWindow);
    d->_navigationBar->setUserInfoCardTitle(title);
}

void ElaSimpleWindow::setUserInfoCardSubTitle(QString subTitle)
{
    Q_D(ElaSimpleWindow);
    d->_navigationBar->setUserInfoCardSubTitle(subTitle);
}

ElaNavigationType::NodeOperateReturnType ElaSimpleWindow::addExpanderNode(QString expanderTitle, QString& expanderKey, ElaIconType::IconName awesome) const
{
    Q_D(const ElaSimpleWindow);
    return d->_navigationBar->addExpanderNode(expanderTitle, expanderKey, awesome);
}

ElaNavigationType::NodeOperateReturnType ElaSimpleWindow::addExpanderNode(QString expanderTitle, QString& expanderKey, QString targetExpanderKey, ElaIconType::IconName awesome) const
{
    Q_D(const ElaSimpleWindow);
    return d->_navigationBar->addExpanderNode(expanderTitle, expanderKey, targetExpanderKey, awesome);
}

ElaNavigationType::NodeOperateReturnType ElaSimpleWindow::addPageNode(QString pageTitle, QWidget* page, ElaIconType::IconName awesome) const
{
    Q_D(const ElaSimpleWindow);
    return d->_navigationBar->addPageNode(pageTitle, page, awesome);
}

ElaNavigationType::NodeOperateReturnType ElaSimpleWindow::addPageNode(QString pageTitle, QWidget* page, QString targetExpanderKey, ElaIconType::IconName awesome) const
{
    Q_D(const ElaSimpleWindow);
    return d->_navigationBar->addPageNode(pageTitle, page, targetExpanderKey, awesome);
}

ElaNavigationType::NodeOperateReturnType ElaSimpleWindow::addPageNode(QString pageTitle, QWidget* page, int keyPoints, ElaIconType::IconName awesome) const
{
    Q_D(const ElaSimpleWindow);
    return d->_navigationBar->addPageNode(pageTitle, page, keyPoints, awesome);
}

ElaNavigationType::NodeOperateReturnType ElaSimpleWindow::addPageNode(QString pageTitle, QWidget* page, QString targetExpanderKey, int keyPoints, ElaIconType::IconName awesome) const
{
    Q_D(const ElaSimpleWindow);
    return d->_navigationBar->addPageNode(pageTitle, page, targetExpanderKey, keyPoints, awesome);
}

ElaNavigationType::NodeOperateReturnType ElaSimpleWindow::addFooterNode(QString footerTitle, QString& footerKey, int keyPoints, ElaIconType::IconName awesome) const
{
    Q_D(const ElaSimpleWindow);
    return d->_navigationBar->addFooterNode(footerTitle, nullptr, footerKey, keyPoints, awesome);
}

ElaNavigationType::NodeOperateReturnType ElaSimpleWindow::addFooterNode(QString footerTitle, QWidget* page, QString& footerKey, int keyPoints, ElaIconType::IconName awesome) const
{
    Q_D(const ElaSimpleWindow);
    return d->_navigationBar->addFooterNode(footerTitle, page, footerKey, keyPoints, awesome);
}

void ElaSimpleWindow::setNodeKeyPoints(QString nodeKey, int keyPoints)
{
    Q_D(ElaSimpleWindow);
    d->_navigationBar->setNodeKeyPoints(nodeKey, keyPoints);
}

int ElaSimpleWindow::getNodeKeyPoints(QString nodeKey) const
{
    Q_D(const ElaSimpleWindow);
    return d->_navigationBar->getNodeKeyPoints(nodeKey);
}

void ElaSimpleWindow::navigation(QString pageKey)
{
    Q_D(ElaSimpleWindow);
    d->_navigationBar->navigation(pageKey);
}

void ElaSimpleWindow::setWindowButtonFlag(ElaAppBarType::ButtonType buttonFlag, bool isEnable)
{
    Q_D(ElaSimpleWindow);
    d->_appBar->setWindowButtonFlag(buttonFlag, isEnable);
}

void ElaSimpleWindow::setWindowButtonFlags(ElaAppBarType::ButtonFlags buttonFlags)
{
    Q_D(ElaSimpleWindow);
    d->_appBar->setWindowButtonFlags(buttonFlags);
}

ElaAppBarType::ButtonFlags ElaSimpleWindow::getWindowButtonFlags() const
{
    return d_ptr->_appBar->getWindowButtonFlags();
}

void ElaSimpleWindow::closeWindow()
{
    Q_D(ElaSimpleWindow);
    d->_isWindowClosing = true;
    d->_appBar->closeWindow();
}

bool ElaSimpleWindow::eventFilter(QObject* watched, QEvent* event)
{
    Q_D(ElaSimpleWindow);
    switch (event->type())
    {
    case QEvent::Resize:
    {
        d->_doNavigationDisplayModeChange();
        break;
    }
    default:
    {
        break;
    }
    }
    return QMainWindow::eventFilter(watched, event);
}

QMenu* ElaSimpleWindow::createPopupMenu()
{
    ElaMenu* menu = nullptr;
    QList<QDockWidget*> dockwidgets = findChildren<QDockWidget*>();
    if (dockwidgets.size())
    {
        menu = new ElaMenu(this);
        for (int i = 0; i < dockwidgets.size(); ++i)
        {
            QDockWidget* dockWidget = dockwidgets.at(i);
            if (dockWidget->parentWidget() == this)
            {
                menu->addAction(dockwidgets.at(i)->toggleViewAction());
            }
        }
        menu->addSeparator();
    }

    QList<QToolBar*> toolbars = findChildren<QToolBar*>();
    if (toolbars.size())
    {
        if (!menu)
        {
            menu = new ElaMenu(this);
        }
        for (int i = 0; i < toolbars.size(); ++i)
        {
            QToolBar* toolBar = toolbars.at(i);
            if (toolBar->parentWidget() == this)
            {
                menu->addAction(toolbars.at(i)->toggleViewAction());
            }
        }
    }
    if (menu)
    {
        menu->setMenuItemHeight(28);
    }
    return menu;
}
