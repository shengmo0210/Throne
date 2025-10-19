#include "QrDecoder.h"

#include "quirc/quirc.h"
#include <qdebug.h>

#include "quirc/quirc_internal.h"

QrDecoder::QrDecoder()
    : m_qr(quirc_new())
{
}

QrDecoder::~QrDecoder()
{
    quirc_destroy(m_qr);
}

QVector<QString> QrDecoder::decode(const QImage &image)
{
    QVector<QString> result;
    if (m_qr == nullptr)
    {
        return result;
    }

    if (quirc_resize(m_qr, image.width(), image.height()) < 0)
    {
        return result;
    }

    uint8_t *rawImage = quirc_begin(m_qr, nullptr, nullptr);
    if (rawImage == nullptr)
    {
        return result;
    }
    std::copy(image.constBits(), image.constBits() + image.width()*image.height(), rawImage);
    quirc_end(m_qr);

    const int count = quirc_count(m_qr);
    if (count < 0)
    {
        return result;
    }

    for (int index = 0; index < count; ++index)
    {
        quirc_code code;
        quirc_extract(m_qr, index, &code);

        quirc_data data;
        const quirc_decode_error_t err = quirc_decode(&code, &data);
        if (err == QUIRC_SUCCESS)
        {
            result.append(QLatin1String((const char *)data.payload));
        }
    }

    return result;
}
