#include "qsgfloattexture.h"


QSGFloatTexture::QSGFloatTexture() :
    QSGDynamicTexture(),
    m_id_texture(0),
    m_num_dims(0),
    m_num_components(0),
    m_buffer(nullptr),
    m_buffer_size(0),
    m_needs_upload(false)
{
    m_dims[0] = 0;
    m_dims[1] = 0;
    m_dims[2] = 0;
    glGenTextures(1, &m_id_texture);
}

QSGFloatTexture::~QSGFloatTexture() {
    if (m_buffer) {
        delete[] m_buffer;
    }

    if (m_id_texture) {
        if (!QOpenGLContext::currentContext()) {
            qWarning("QSGFloatTexture::~QSGFloatTexture needs OpenGL context");
        }
        glDeleteTextures(1, &m_id_texture);
    }
}

int QSGFloatTexture::textureId() const {
    return m_id_texture;
}

QSize QSGFloatTexture::textureSize() const {
    if (m_num_dims == 2) {
        return QSize(m_dims[0], m_dims[1]);
    } else {
        return QSize();
    }
}

bool QSGFloatTexture::hasAlphaChannel() const {
    return (m_num_components == 4);
}

bool QSGFloatTexture::hasMipmaps() const {
    return false;
}

void QSGFloatTexture::bind() {
    switch (m_num_dims) {
    case 1:
        glBindTexture(GL_TEXTURE_1D, m_id_texture);
        glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        break;
    case 2:
        glBindTexture(GL_TEXTURE_2D, m_id_texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        break;
    case 3:
        glBindTexture(GL_TEXTURE_3D, m_id_texture);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        break;
    default:
        break;
    }

    if (m_needs_upload) {
        m_needs_upload = false;
        // determine color format
        GLint internal_format;
        GLenum format;
        switch (m_num_components) {
        case 1:
            internal_format = GL_R32F;
            format = GL_RED;
            break;
        case 2:
            internal_format = GL_RG32F;
            format = GL_RG;
            break;
        case 3:
            internal_format = GL_RGB32F;
            format = GL_RGB;
            break;
        case 4:
            internal_format = GL_RGBA32F;
            format = GL_RGBA;
            break;
        default:
            return;
        }

        // upload data as 1D, 2D or 3D texture
        switch (m_num_dims) {
        case 1:
            glTexImage1D(GL_TEXTURE_1D, 0, internal_format, m_dims[0], 0, format, GL_FLOAT, m_buffer);
            break;
        case 2:
            glTexImage2D(GL_TEXTURE_2D, 0, internal_format, m_dims[0], m_dims[1], 0, format, GL_FLOAT, m_buffer);
            break;
        case 3:
            glTexImage3D(GL_TEXTURE_3D, 0, internal_format, m_dims[0], m_dims[1], m_dims[2], 0, format, GL_FLOAT, m_buffer);
            break;
        default:
            return;
        }
    }
}

void QSGFloatTexture::setData1D(double *data, int size, int num_components)
{
    m_dims[0] = size;
    m_num_dims = 1;
    m_num_components = num_components;
    int n = size * num_components;
    if (m_buffer_size < n) {
        if (m_buffer) delete[] m_buffer;
        m_buffer = new GLfloat[n];
        m_buffer_size = n;
    }
    for (int i = 0; i < n; ++i) {
        m_buffer[i] = data[i];
    }
    m_needs_upload = true;
}

void QSGFloatTexture::setData2D(double *data, int width, int height, int num_components)
{
    m_dims[0] = width;
    m_dims[1] = height;
    m_num_dims = 2;
    m_num_components = num_components;
    int n = width * height * num_components;
    if (m_buffer_size < n) {
        if (m_buffer) delete[] m_buffer;
        m_buffer = new GLfloat[n];
        m_buffer_size = n;
    }
    for (int i = 0; i < n; ++i) {
        m_buffer[i] = data[i];
    }
    m_needs_upload = true;
}

void QSGFloatTexture::setData3D(double *data, int width, int height, int depth, int num_components)
{
    m_dims[0] = width;
    m_dims[1] = height;
    m_dims[2] = depth;
    m_num_dims = 3;
    m_num_components = num_components;
    int n = width * height * depth * num_components;
    if (m_buffer_size < n) {
        if (m_buffer) delete[] m_buffer;
        m_buffer = new GLfloat[n];
        m_buffer_size = n;
    }
    for (int i = 0; i < n; ++i) {
        m_buffer[i] = data[i];
    }
    m_needs_upload = true;
}

bool QSGFloatTexture::updateTexture() {
    return false;
}
