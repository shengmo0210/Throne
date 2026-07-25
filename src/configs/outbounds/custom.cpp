#include "include/configs/outbounds/custom.h"

#include "include/configs/common/OutboundFactory.h"
#include "include/configs/common/utils.h"
#include "include/global/Utils.hpp"

#include <QJsonArray>
#include <memory>

namespace Configs
{
    namespace
    {
        // ---- sing-box -----------------------------------------------------

        bool isSingBoxInfra(const QString& t)
        {
            return t == "direct" || t == "block" || t == "dns";
        }

        // Reuses the real per-protocol parser + GetSecurity() so a custom
        // outbound reads exactly like the equivalent native one.
        SecurityInfo analyzeSingBoxOutbound(const QJsonObject& o)
        {
            const auto type = o["type"].toString();
            if (type.isEmpty() || type == "custom" || type == "selector" || type == "urltest"
                || isSingBoxInfra(type))
                return {};
            std::shared_ptr<outbound> ob(NewOutboundByType(type));
            if (!ob || ob->invalid) return {};
            ob->ParseFromJson(o);
            return ob->GetSecurity();
        }

        // Resolves route.final and selector/urltest indirection (depth-guarded)
        // down to the outbound that actually egresses.
        QJsonObject resolveSingBoxEgress(const QJsonArray& outbounds, const QString& tag, int depth)
        {
            if (depth <= 0 || outbounds.isEmpty()) return {};
            // The named tag, or the first outbound (sing-box's default egress).
            QJsonObject target;
            if (tag.isEmpty()) {
                target = outbounds.first().toObject();
            } else {
                for (const auto& v : outbounds) {
                    if (auto obj = v.toObject(); obj["tag"].toString() == tag) {
                        target = obj;
                        break;
                    }
                }
            }
            if (target.isEmpty()) return {};
            const auto type = target["type"].toString();
            if (type == "selector" || type == "urltest") {
                auto subs = target["outbounds"].toArray();
                QString next = target["default"].toString();
                if (next.isEmpty() && !subs.isEmpty()) next = subs.first().toString();
                return resolveSingBoxEgress(outbounds, next, depth - 1);
            }
            if (type.isEmpty() || isSingBoxInfra(type)) return {};
            return target;
        }

        // ---- Xray ---------------------------------------------------------

        bool isXrayInfra(const QString& p)
        {
            return p == "freedom" || p == "blackhole" || p == "dns" || p == "loopback";
        }

        // Xray isn't backed by Throne's parsers, so read the raw fields directly.
        SecurityInfo analyzeXrayOutbound(const QJsonObject& o)
        {
            const auto protocol = o["protocol"].toString();
            if (protocol.isEmpty() || isXrayInfra(protocol)) return {};

            const auto stream = o["streamSettings"].toObject();
            SecurityInfo info;
            info.transport = DisplayTransportName(stream["network"].toString());

            const auto security = stream["security"].toString();
            if (security == "reality") {
                info.label = QObject::tr("Reality");
                info.level = SecurityLevel::Secure;
                return info;
            }
            if (security == "tls") {
                const bool insecure = stream["tlsSettings"].toObject()["allowInsecure"].toBool();
                info.label = insecure ? QObject::tr("Insecure TLS") : QObject::tr("TLS");
                info.level = insecure ? SecurityLevel::Weak : SecurityLevel::Secure;
                return info;
            }
            // No transport security: shadowsocks still encrypts its payload,
            // vmess encrypts but is trivially detectable, the rest are plaintext.
            if (protocol == "shadowsocks") {
                info.label = QObject::tr("Encrypted");
                info.level = SecurityLevel::Secure;
                return info;
            }
            if (protocol == "vmess") {
                info.label = QObject::tr("Encrypted");
                info.level = SecurityLevel::Weak;
                return info;
            }
            info.label = QObject::tr("Raw");
            info.level = SecurityLevel::None;
            return info;
        }

        // Xray routes to the first outbound by default; pick the first real proxy.
        QJsonObject firstXrayEgress(const QJsonArray& outbounds)
        {
            for (const auto& v : outbounds) {
                if (auto obj = v.toObject(); !isXrayInfra(obj["protocol"].toString())) return obj;
            }
            return {};
        }
    } // namespace

    SecurityInfo Custom::GetSecurity()
    {
        if (type == CustomOutbound) {
            return analyzeSingBoxOutbound(QString2QJsonObject(config));
        }
        if (type == CustomFullConfig) {
            const auto obj = QString2QJsonObject(config);
            const auto egress = resolveSingBoxEgress(obj["outbounds"].toArray(),
                                                     obj["route"].toObject()["final"].toString(), 5);
            return egress.isEmpty() ? SecurityInfo{} : analyzeSingBoxOutbound(egress);
        }
        if (type == CustomXrayOutbound) {
            return analyzeXrayOutbound(QString2QJsonObject(config));
        }
        if (type == CustomXrayFullConfig) {
            const auto obj = QString2QJsonObject(config);
            const auto egress = firstXrayEgress(obj["outbounds"].toArray());
            return egress.isEmpty() ? SecurityInfo{} : analyzeXrayOutbound(egress);
        }
        return {};
    }
}
