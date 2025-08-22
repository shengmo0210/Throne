#pragma once

#include <QMainWindow>
#include <include/global/HTTPRequestHelper.hpp>
#ifndef Q_MOC_RUN
#include <libcore.pb.h>
#endif

#include "include/global/Configs.hpp"
#include "include/stats/connections/connectionLister.hpp"
#include "3rdparty/qv2ray/v2/ui/widgets/speedchart/SpeedWidget.hpp"
#ifdef Q_OS_LINUX
#include <QtDBus>
#endif

#ifndef MW_INTERFACE

#include <QTime>
#include <QTableWidgetItem>
#include <QKeyEvent>
#include <QSystemTrayIcon>
#include <QProcess>
#include <QTextDocument>
#include <QShortcut>
#include <QSemaphore>
#include <QMutex>
#include <QThreadPool>

#include "group/GroupSort.hpp"

#include "include/dataStore/ProxyEntity.hpp"
#include "include/global/GuiUtils.hpp"
#include "ui_mainwindow.h"

#endif

namespace Configs_sys {
    class CoreProcess;
}

QT_BEGIN_NAMESPACE
namespace Ui {
    class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

    ~MainWindow() override;

    void prepare_exit();

    void refresh_proxy_list(const int &id = -1);

    void show_group(int gid);

    void refresh_groups();

    void refresh_status(const QString &traffic_update = "");

    void update_traffic_graph(int proxyDl, int proxyUp, int directDl, int directUp);

    void profile_start(int _id = -1);

    void profile_stop(bool crash = false, bool block = false, bool manual = false);

    void set_spmode_system_proxy(bool enable, bool save = true);

    void toggle_system_proxy();

    void set_spmode_vpn(bool enable, bool save = true);

    bool get_elevated_permissions(int reason = 3);

    void show_log_impl(const QString &log);

    void start_select_mode(QObject *context, const std::function<void(int)> &callback);

    void RegisterHotkey(bool unregister);

    bool StopVPNProcess();

    void UpdateConnectionList(const QMap<QString, Stats::ConnectionMetadata>& toUpdate, const QMap<QString, Stats::ConnectionMetadata>& toAdd);

    void UpdateConnectionListWithRecreate(const QList<Stats::ConnectionMetadata>& connections);

    void UpdateDataView(bool force = false);

    void setDownloadReport(const DownloadProgressReport& report, bool show);

signals:

    void profile_selected(int id);

public slots:

    void on_commitDataRequest();

    void on_menu_exit_triggered();

#ifndef MW_INTERFACE

private slots:

    void on_masterLogBrowser_customContextMenuRequested(const QPoint &pos);

    void on_menu_basic_settings_triggered();

    void on_menu_routing_settings_triggered();

    void on_menu_vpn_settings_triggered();

    void on_menu_hotkey_settings_triggered();

    void on_menu_add_from_input_triggered();

    static void on_menu_add_from_clipboard_triggered();

    void on_menu_clone_triggered();

    void on_menu_delete_repeat_triggered();

    void on_menu_delete_triggered();

    void on_menu_reset_traffic_triggered();

    void on_menu_copy_links_triggered();

    void on_menu_copy_links_nkr_triggered();

    void on_menu_export_config_triggered();

    void display_qr_link(bool nkrFormat = false);

    void on_menu_scan_qr_triggered();

    void on_menu_clear_test_result_triggered();

    void on_menu_manage_groups_triggered();

    void on_menu_select_all_triggered();

    void on_menu_remove_unavailable_triggered();

    void on_menu_remove_invalid_triggered();

    void on_menu_resolve_selected_triggered();

    void on_menu_resolve_domain_triggered();

    void on_menu_update_subscription_triggered();

    void on_proxyListTable_itemDoubleClicked(QTableWidgetItem *item);

    void on_proxyListTable_customContextMenuRequested(const QPoint &pos);

    void on_tabWidget_currentChanged(int index);

    void on_tabWidget_customContextMenuRequested(const QPoint& p);

private:
    Ui::MainWindow *ui;
    QSystemTrayIcon *tray;
    QShortcut *shortcut_ctrl_f = new QShortcut(QKeySequence("Ctrl+F"), this);
    QShortcut *shortcut_esc = new QShortcut(QKeySequence("Esc"), this);
    //
    QThreadPool *parallelCoreCallPool = new QThreadPool(this);
    std::atomic<bool> stopSpeedtest = false;
    QMutex speedtestRunning;
    //
    Configs_sys::CoreProcess *core_process;
    qint64 vpn_pid = 0;
    //
    bool qvLogAutoScoll = true;
    QTextDocument *qvLogDocument = new QTextDocument(this);
    //
    QString title_error;
    int icon_status = -1;
    std::shared_ptr<Configs::ProxyEntity> running;
    QString traffic_update_cache;
    QTime last_test_time;
    //
    int proxy_last_order = -1;
    bool select_mode = false;
    QMutex mu_starting;
    QMutex mu_stopping;
    QMutex mu_exit;
    int exit_reason = 0;
    //
    QMutex mu_download_update;
    //
    int toolTipID;
    //
    SpeedWidget *speedChartWidget;
    //
    // for data view
    QDateTime lastUpdated = QDateTime::currentDateTime();
    QString currentSptProfileName;
    bool showSpeedtestData = false;
    bool showDownloadData = false;
    libcore::SpeedTestResult currentTestResult;
    DownloadProgressReport currentDownloadReport; // could use a list, but don't think can show more than one anyways

    std::map<std::string, std::string> ruleSetMap;

    QStringList remoteRouteProfiles;
    QMutex mu_remoteRouteProfiles;

    QList<std::shared_ptr<Configs::ProxyEntity>> get_now_selected_list();

    QList<std::shared_ptr<Configs::ProxyEntity>> get_selected_or_group();

    void dialog_message_impl(const QString &sender, const QString &info);

    void refresh_proxy_list_impl(const int &id = -1, GroupSortAction groupSortAction = {});

    void refresh_proxy_list_impl_refresh_data(const int &id = -1, bool stopping = false);

    void refresh_table_item(int row, const std::shared_ptr<Configs::ProxyEntity>& profile, bool stopping);

    void keyPressEvent(QKeyEvent *event) override;

    void closeEvent(QCloseEvent *event) override;

    //

    void HotkeyEvent(const QString &key);

    void RegisterShortcuts();

    // grpc

    static void setup_grpc();

    void urltest_current_group(const QList<std::shared_ptr<Configs::ProxyEntity>>& profiles);

    void stopTests();

    void runURLTest(const QString& config, bool useDefault, const QStringList& outboundTags, const QMap<QString, int>& tag2entID, int entID = -1);

    void url_test_current();

    void speedtest_current_group(const QList<std::shared_ptr<Configs::ProxyEntity>>& profiles, bool testCurrent = false);

    void runSpeedTest(const QString& config, bool useDefault, bool testCurrent, const QStringList& outboundTags, const QMap<QString, int>& tag2entID, int entID = -1);

    bool set_system_dns(bool set, bool save_set = true);

    void CheckUpdate();

    void setupConnectionList();

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

#endif // MW_INTERFACE
};

inline MainWindow *GetMainWindow() {
    return (MainWindow *) mainwindow;
}

void UI_InitMainWindow();

#ifdef Q_OS_LINUX
/*
 * Proxy class for interface org.freedesktop.portal.Request
 */
class OrgFreedesktopPortalRequestInterface : public QDBusAbstractInterface
{
    Q_OBJECT
public:
    OrgFreedesktopPortalRequestInterface(const QString& service,
                                         const QString& path,
                                         const QDBusConnection& connection,
                                         QObject* parent = nullptr);

    ~OrgFreedesktopPortalRequestInterface();

public Q_SLOTS:
    inline QDBusPendingReply<> Close()
    {
        QList<QVariant> argumentList;
        return asyncCallWithArgumentList(QStringLiteral("Close"), argumentList);
    }

Q_SIGNALS: // SIGNALS
    void Response(uint response, QVariantMap results);
};

namespace org {
namespace freedesktop {
namespace portal {
typedef ::OrgFreedesktopPortalRequestInterface Request;
}
}
}
#endif
