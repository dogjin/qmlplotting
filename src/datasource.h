#ifndef DATASOURCE_H
#define DATASOURCE_H

#include <QQuickItem>
#include <QSGTextureProvider>
#include <QSGDynamicTexture>
#include <QByteArray>

class DataTexture;
class DataTextureProvider;

class DataSource : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(int dataWidth READ dataWidth NOTIFY dataChanged)
    Q_PROPERTY(int dataHeight READ dataHeight NOTIFY dataChanged)

public:
    explicit DataSource(QQuickItem *parent = 0);
    virtual ~DataSource();

    int dataWidth() const {return m_width;}
    int dataHeight() const {return m_height;}

    virtual bool isTextureProvider() const;
    virtual QSGTextureProvider* textureProvider();

    Q_INVOKABLE bool setDataFloat64(void* data, int width, int height);
    Q_INVOKABLE bool setTestData();

signals:
    void dataChanged();

public:
    double* m_data;
    int m_width;
    int m_height;

private:
    bool m_new_data;
    DataTextureProvider* m_provider;
    QByteArray m_test_data_buffer;
    friend class DataTexture;
};

class DataTextureProvider : public QSGTextureProvider
{
    Q_OBJECT

public:
    DataTextureProvider(DataSource* source);
    virtual ~DataTextureProvider();

    virtual QSGTexture* texture() const;
    DataTexture* m_datatexture;
};

#endif // DATASOURCE_H