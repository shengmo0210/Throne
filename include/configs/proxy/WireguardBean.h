#pragma once

#include "AbstractBean.hpp"

namespace Configs {
    class WireguardBean : public AbstractBean {
    public:
        QString privateKey;
        QString publicKey;
        QString preSharedKey;
        QList<int> reserved;
        int persistentKeepalive = 0;
        QStringList localAddress;
        int MTU = 1420;
        bool useSystemInterface = false;
        int workerCount = 0;

        // Amnezia Options
        bool enable_amnezia = false;
        int junk_packet_count = 0;
        int junk_packet_min_size = 0;
        int junk_packet_max_size = 0;
        int init_packet_junk_size = 0;
        int response_packet_junk_size = 0;
        int init_packet_magic_header = 0;
        int response_packet_magic_header = 0;
        int underload_packet_magic_header = 0;
        int transport_packet_magic_header = 0;

        WireguardBean() : AbstractBean(0) {
            _add(new configItem("private_key", &privateKey, itemType::string));
            _add(new configItem("public_key", &publicKey, itemType::string));
            _add(new configItem("pre_shared_key", &preSharedKey, itemType::string));
            _add(new configItem("reserved", &reserved, itemType::integerList));
            _add(new configItem("persistent_keepalive", &persistentKeepalive, itemType::integer));
            _add(new configItem("local_address", &localAddress, itemType::stringList));
            _add(new configItem("mtu", &MTU, itemType::integer));
            _add(new configItem("use_system_proxy", &useSystemInterface, itemType::boolean));
            _add(new configItem("worker_count", &workerCount, itemType::integer));

            _add(new configItem("enable_amnezia", &enable_amnezia, itemType::boolean));
            _add(new configItem("junk_packet_count", &junk_packet_count, itemType::integer));
            _add(new configItem("junk_packet_min_size", &junk_packet_min_size, itemType::integer));
            _add(new configItem("junk_packet_max_size", &junk_packet_max_size, itemType::integer));
            _add(new configItem("init_packet_junk_size", &init_packet_junk_size, itemType::integer));
            _add(new configItem("response_packet_junk_size", &response_packet_junk_size, itemType::integer));
            _add(new configItem("init_packet_magic_header", &init_packet_magic_header, itemType::integer));
            _add(new configItem("response_packet_magic_header", &response_packet_magic_header, itemType::integer));
            _add(new configItem("underload_packet_magic_header", &underload_packet_magic_header, itemType::integer));
            _add(new configItem("transport_packet_magic_header", &transport_packet_magic_header, itemType::integer));
        };

        QString FormatReserved() {
            QString res = "";
            for (int i=0;i<reserved.size();i++) {
                res += Int2String(reserved[i]);
                if (i != reserved.size() - 1) {
                    res += "-";
                }
            }
            return res;
        }

        QString DisplayType() override { return "Wireguard"; };

        CoreObjOutboundBuildResult BuildCoreObjSingBox() override;

        bool TryParseLink(const QString &link);

        bool TryParseJson(const QJsonObject &obj);

        QString ToShareLink() override;

        bool IsEndpoint() override {return true;}

    private:
        bool parseWgConfig(const QString &config)
        {
            if (!config.contains("[Interface]") || !config.contains("[Peer]")) return false;
            auto lines = config.split("\n");
            for (const auto& line : lines)
            {
                if (line.trimmed().isEmpty()) continue;
                if (line.contains("[Interface]") || line.contains("[Peer]")) continue;
                if (!line.contains("=")) return false;
                auto eqIdx = line.indexOf("=");
                if (line.startsWith("PrivateKey"))
                {
                    privateKey = line.mid(eqIdx + 1).trimmed();
                }
                if (line.startsWith("Address"))
                {
                    auto addresses = line.mid(eqIdx + 1).trimmed().split(",");
                    for (const auto& address : addresses) localAddress.append(address.trimmed());
                }
                if (line.startsWith("MTU"))
                {
                    MTU = line.mid(eqIdx + 1).toInt();
                }
                if (line.startsWith("PublicKey"))
                {
                    publicKey = line.mid(eqIdx + 1).trimmed();
                }
                if (line.startsWith("PresharedKey"))
                {
                    preSharedKey = line.mid(eqIdx + 1).trimmed();
                }
                if (line.startsWith("PersistentKeepalive"))
                {
                    persistentKeepalive = line.mid(eqIdx + 1).toInt();
                }
                if (line.startsWith("Endpoint"))
                {
                    auto addrPort = line.mid(eqIdx + 1).trimmed();
                    if (!addrPort.contains(":")) return false;
                    serverAddress = addrPort.split(":")[0].trimmed();
                    serverPort = addrPort.split(":")[1].trimmed().toInt();
                }
                if (line.startsWith("S1"))
                {
                    enable_amnezia = true;
                    init_packet_junk_size = line.mid(eqIdx + 1).toInt();
                }
                if (line.startsWith("S2"))
                {
                    enable_amnezia = true;
                    response_packet_junk_size = line.mid(eqIdx + 1).toInt();
                }
                if (line.startsWith("Jc"))
                {
                    enable_amnezia = true;
                    junk_packet_count = line.mid(eqIdx + 1).toInt();
                }
                if (line.startsWith("Jmin"))
                {
                    enable_amnezia = true;
                    junk_packet_min_size = line.mid(eqIdx + 1).toInt();
                }
                if (line.startsWith("Jmax"))
                {
                    enable_amnezia = true;
                    junk_packet_max_size = line.mid(eqIdx + 1).toInt();
                }
                if (line.startsWith("H1"))
                {
                    enable_amnezia = true;
                    init_packet_magic_header = line.mid(eqIdx + 1).toInt();
                }
                if (line.startsWith("H2"))
                {
                    enable_amnezia = true;
                    response_packet_magic_header = line.mid(eqIdx + 1).toInt();
                }
                if (line.startsWith("H3"))
                {
                    enable_amnezia = true;
                    underload_packet_magic_header = line.mid(eqIdx + 1).toInt();
                }
                if (line.startsWith("H4"))
                {
                    enable_amnezia = true;
                    transport_packet_magic_header = line.mid(eqIdx + 1).toInt();
                }
            }
            name = "Wg file config";
            return true;
        };
    };
} // namespace Configs
