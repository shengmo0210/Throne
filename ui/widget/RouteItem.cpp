#include "RouteItem.h"
#include "ui_RouteItem.h"
#include "db/RouteEntity.h"
#include "db/Database.hpp"
#include "rpc/gRPC.h"

int RouteItem::getIndexOf(const QString& name) const {
    for (int i=0;i<chain->Rules.size();i++) {
        if (chain->Rules[i]->name == name) return i;
    }

    return -1;
}

QString get_outbound_name(int id) {
    // -1 is proxy -2 is direct -3 is block -4 is dns_out
    if (id == -1) return "proxy";
    if (id == -2) return "direct";
    if (id == -3) return "block";
    if (id == -4) return "dns_out";
    auto profiles = NekoGui::profileManager->profiles;
    if (profiles.count(id)) return profiles[id]->bean->name;
    return "INVALID OUTBOUND";
}

int get_outbound_id(const QString& name) {
    if (name == "proxy") return -1;
    if (name == "direct") return -2;
    if (name == "block") return -3;
    if (name == "dns_out") return -4;
    auto profiles = NekoGui::profileManager->profiles;
    for (const auto& item: profiles) {
        if (item.second->bean->name == name) return item.first;
    }

    return NekoGui::INVALID_ID;
}

QStringList get_all_outbounds() {
    QStringList res;
    auto profiles = NekoGui::profileManager->profiles;
    for (const auto &item: profiles) {
        res.append(item.second->bean->name);
    }

    return res;
}

RouteItem::RouteItem(QWidget *parent, const std::shared_ptr<NekoGui::RoutingChain>& routeChain)
    : QDialog(parent), ui(new Ui::RouteItem) {
    ui->setupUi(this);

    // make a copy
    chain = std::make_shared<NekoGui::RoutingChain>(*routeChain);

    // add the default rule if empty
    if (chain->Rules.empty()) {
        auto routeItem = std::make_shared<NekoGui::RouteRule>();
        routeItem->name = "dns-hijack";
        routeItem->protocol = "dns";
        routeItem->outboundID = -4;
        chain->Rules << routeItem;
    }

    // setup rule set helper
    bool ok; // for now we discard this
    auto geoIpList = NekoGui_rpc::defaultClient->GetGeoList(&ok, NekoGui_rpc::GeoRuleSetType::ip);
    auto geoSiteList = NekoGui_rpc::defaultClient->GetGeoList(&ok, NekoGui_rpc::GeoRuleSetType::site);
    geo_items << geoIpList << geoSiteList;
    helperModel = new QStringListModel(geo_items, this);
    ui->rule_set_helper->hide();
    ui->rule_set_helper->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->rule_set_helper->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->rule_set_helper->setSelectionRectVisible(false);
    ui->rule_set_helper->setModel(helperModel);
    connect(ui->rule_set_helper, &QListView::clicked, this, [=](const QModelIndex& index){
        applyRuleHelperSelect(index);
    });

    std::map<QString, int> valueMap;
    for (auto &item: chain->Rules) {
        auto baseName = item->name;
        int randPart;
        if (baseName == "") {
            randPart = int(GetRandomUint64()%1000);
            baseName = "rule_" + Int2String(randPart);
            lastNum = std::max(lastNum, randPart);
        }
        while (true) {
            valueMap[baseName]++;
            if (valueMap[baseName] > 1) {
                valueMap[baseName]--;
                randPart = int(GetRandomUint64()%1000);
                baseName = "rule_" + Int2String(randPart);
                lastNum = std::max(lastNum, randPart);
                continue;
            }
            item->name = baseName;
            break;
        }
        ui->route_items->addItem(item->name);
    }

    QStringList outboundOptions = {"proxy", "direct", "block", "dns_out"};
    outboundOptions << get_all_outbounds();
    // init outbound map
    outboundMap[0] = -1;
    outboundMap[1] = -2;
    outboundMap[2] = -3;
    outboundMap[3] = -4;
    for (const auto& item: NekoGui::profileManager->profiles) {
        outboundMap[outboundMap.size()] = item.second->id;
    }

    ui->route_name->setText(chain->name);
    ui->rule_attr->addItems(NekoGui::RouteRule::get_attributes());
    ui->rule_out->addItems(outboundOptions);
    ui->rule_attr_text->hide();
    ui->rule_attr_data->setTitle("");
    ui->rule_attr_box->setEnabled(false);
    ui->rule_preview->setEnabled(false);
    updateRuleSection();

    connect(ui->route_import_json, &QPushButton::clicked, this, [=] {
        auto w = new QDialog(this);
        w->setWindowTitle(tr("Import JSON Array"));
        w->setWindowModality(Qt::ApplicationModal);

        auto line = 0;
        auto layout = new QGridLayout;
        w->setLayout(layout);

        auto *tEdit = new QTextEdit;
        tEdit->setPlaceholderText("[\n"
            "  {\n"
            "    \"outbound\": \"dns-out\",\n"
            "    \"protocol\": \"dns\"\n"
            "  },\n"
            "  {\n"
            "    \"outbound\": \"dns-out\",\n"
            "    \"protocol\": \"udp\"\n"
            "  }\n"
            "]");
        layout->addWidget(tEdit, line++, 0);

        auto *buttons = new QDialogButtonBox;
        buttons->setOrientation(Qt::Horizontal);
        buttons->setStandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
        layout->addWidget(buttons, line, 0);

        connect(buttons, &QDialogButtonBox::accepted, w, [=]{
           auto err = new QString;
           auto parsed = NekoGui::RoutingChain::parseJsonArray(QString2QJsonArray(tEdit->toPlainText()), err);
           if (!err->isEmpty()) {
               MessageBoxInfo("Invalid JSON Array", "The provided input cannot be parsed to a valid route rule array:\n" + *err);
               return;
           }
           chain->Rules.clear();
           chain->Rules << parsed;
           updateRouteItemsView();
           updateRuleSection();

           w->accept();
        });
        connect(buttons, &QDialogButtonBox::rejected, w, &QDialog::reject);

        w->exec();
        w->deleteLater();
    });

    connect(ui->rule_name, &QLineEdit::textChanged, this, [=](const QString& text) {
        if (currentIndex == -1) return;
        chain->Rules[currentIndex]->name = QString(text);
        updateRouteItemsView();
    });

    connect(ui->rule_attr_selector, &QComboBox::currentTextChanged, this, [=](const QString& text){
       if (currentIndex == -1) return;
       chain->Rules[currentIndex]->set_field_value(ui->rule_attr->currentText(), {QString(text)});
       updateRulePreview();
    });

    connect(ui->rule_attr_text, &QTextEdit::textChanged, this, [=] {
        if (currentIndex == -1) return;
        auto currentVal = ui->rule_attr_text->toPlainText().split('\n');
        chain->Rules[currentIndex]->set_field_value(ui->rule_attr->currentText(), currentVal);
        if (ui->rule_attr->currentText() == "rule_set") updateHelperItems(currentVal.last());
        updateRulePreview();
    });

    connect(ui->rule_out, &QComboBox::currentTextChanged, this, [=](const QString& text) {
        if (currentIndex == -1) return;
        auto id = outboundMap[ui->rule_out->currentIndex()]; // we need to do this to avoid defining a seprate function for SLOT, as Qt 5 does not support lambda for  currentIndexChanged...
        if (id == NekoGui::INVALID_ID) {
            MessageBoxWarning("Invalid state", "selected outbound does not exists in the database, try restarting the app.");
            return;
        }
        chain->Rules[currentIndex]->outboundID = id;
        updateRulePreview();
    });

    connect(ui->route_items, &QListWidget::currentRowChanged, this, [=](const int idx) {
        if (idx == -1) return;
        currentIndex = idx;
        updateRuleSection();
    });

    connect(ui->rule_attr, &QComboBox::currentTextChanged, this, [=](const QString& text){
        updateRuleSection();
    });

    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, [=]{
        accept();
    });
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, [=]{
       QDialog::reject();
    });

    deleteShortcut = new QShortcut(QKeySequence(Qt::Key_Delete), this);

    connect(deleteShortcut, &QShortcut::activated, this, [=]{
        on_delete_route_item_clicked();
    });

    if (chain->isViewOnly()) {
        ui->route_name->setText(chain->name + " (View only)");
        ui->route_name->setEnabled(false);
        ui->rule_attr_box->setEnabled(false);
        ui->new_route_item->setEnabled(false);
        ui->moveup_route_item->setEnabled(false);
        ui->movedown_route_item->setEnabled(false);
        ui->delete_route_item->setEnabled(false);
        ui->route_import_json->setEnabled(false);
    }
}

RouteItem::~RouteItem() {
    delete ui;
}

void RouteItem::accept() {
    chain->name = ui->route_name->text();

    if (chain->name == "") {
        MessageBoxWarning("Invalid operation", "Cannot create Route Profile with empty name");
        return;
    }

    QList<std::shared_ptr<NekoGui::RouteRule>> tmpChain;
    for (const auto& item: chain->Rules) {
        if (!item->isEmpty()) {
            tmpChain << item;
        }
    }
    chain->Rules.clear();
    for (const auto& item: tmpChain) {
        chain->Rules << item;
    }

    if (chain->Rules.empty()) {
        MessageBoxInfo("Empty Route Profile", "No valid rules are in the profile");
        return;
    }

    emit settingsChanged(chain);

    QDialog::accept();
}

void RouteItem::updateRouteItemsView() {
    ui->route_items->clear();
    if (chain->Rules.empty()) return;

    for (const auto& item: chain->Rules) {
        ui->route_items->addItem(item->name);
    }
    if (currentIndex != -1) ui->route_items->setCurrentRow(currentIndex);
}

void RouteItem::updateRuleSection() {
    if (currentIndex == -1) return;

    auto ruleItem = chain->Rules[currentIndex];
    auto currentAttr = ui->rule_attr->currentText();
    switch (ruleItem->get_input_type(currentAttr)) {
        case NekoGui::trufalse: {
            QStringList items = {"false", "true"};
            QString currentVal = ruleItem->get_current_value_bool(currentAttr);
            showSelectItem(items, currentVal);
            break;
        }
        case NekoGui::select: {
            auto items = NekoGui::RouteRule::get_values_for_field(currentAttr);
            auto currentVal = ruleItem->get_current_value_string(currentAttr)[0];
            showSelectItem(items, currentVal);
            break;
        }
        case NekoGui::text: {
            auto currentItems = ruleItem->get_current_value_string(currentAttr);
            showTextEnterItem(currentItems);
            break;
        }
    }
    ui->rule_name->setText(ruleItem->name);
    ui->rule_attr_box->setDisabled(chain->isViewOnly());
    ui->rule_out->setCurrentText(get_outbound_name(ruleItem->outboundID));
    if (currentAttr == "rule_set") ui->rule_set_helper->show();
    else ui->rule_set_helper->hide();

    updateRulePreview();
}

void RouteItem::updateRulePreview() {
    if (currentIndex == -1) return;

    ui->rule_preview->setText(QJsonObject2QString(chain->Rules[currentIndex]->get_rule_json(true), false));
}

void RouteItem::setDefaultRuleData(const QString& currentData) {
    ui->rule_attr->setCurrentText("ip_version");
    ui->rule_attr_data->setTitle("ip_version");
    showSelectItem(NekoGui::RouteRule::get_values_for_field("ip_version"), currentData);
}

void RouteItem::showSelectItem(const QStringList& items, const QString& currentItem) {
    ui->rule_attr_text->hide();
    ui->rule_attr_selector->clear();
    ui->rule_attr_selector->show();
    ui->rule_attr_selector->addItems(items);
    ui->rule_attr_selector->setCurrentText(currentItem);
    adjustSize();
}

void RouteItem::showTextEnterItem(const QStringList& items) {
    ui->rule_attr_selector->hide();
    ui->rule_attr_text->clear();
    ui->rule_attr_text->show();
    ui->rule_attr_text->setText(items.join('\n'));
    adjustSize();
}

void RouteItem::updateHelperItems(const QString& base) {
    ui->rule_set_helper->clearSelection();
    current_helper_items.clear();
    for (const auto& item: geo_items) {
        if (item.contains(base)) current_helper_items << item;
    }
    helperModel->setStringList(current_helper_items);
    ui->rule_set_helper->setModel(helperModel);
}

void RouteItem::applyRuleHelperSelect(const QModelIndex& index) {
    auto option = ui->rule_set_helper->model()->data(index, Qt::DisplayRole).toString();
    auto currentText = ui->rule_attr_text->toPlainText();
    auto parts = currentText.split('\n');
    parts[parts.size() - 1] = option;
    ui->rule_attr_text->setText(parts.join('\n'));
}

void RouteItem::on_new_route_item_clicked() {
    if (chain->isViewOnly()) return;
    auto routeItem = std::make_shared<NekoGui::RouteRule>();
    routeItem->name = "rule_" + Int2String(++lastNum);
    chain->Rules << routeItem;
    currentIndex = chain->Rules.size() - 1;
    ui->rule_name->setText(routeItem->name);
    currentIndex = chain->Rules.size()-1;

    updateRouteItemsView();
    updateRuleSection();
}

void RouteItem::on_moveup_route_item_clicked() {
    if (chain->isViewOnly()) return;
    if (currentIndex == -1 || currentIndex == 0) return;
    auto curr = chain->Rules[currentIndex];
    chain->Rules[currentIndex] = chain->Rules[currentIndex-1];
    chain->Rules[currentIndex-1] = curr;
    currentIndex--;
    updateRouteItemsView();
}

void RouteItem::on_movedown_route_item_clicked() {
    if (chain->isViewOnly()) return;
    if (currentIndex == -1 || currentIndex == chain->Rules.size() - 1) return;
    auto curr = chain->Rules[currentIndex];
    chain->Rules[currentIndex] = chain->Rules[currentIndex+1];
    chain->Rules[currentIndex+1] = curr;
    currentIndex++;
    updateRouteItemsView();
}

void RouteItem::on_delete_route_item_clicked() {
    if (chain->isViewOnly()) return;
    if (currentIndex == -1) return;
    chain->Rules.removeAt(currentIndex);
    if (chain->Rules.empty()) currentIndex = -1;
    else {
        currentIndex--;
        if (currentIndex == -1) currentIndex = 0;
    }
    updateRouteItemsView();
    updateRuleSection();
}
