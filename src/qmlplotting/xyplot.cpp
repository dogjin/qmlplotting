#include "xyplot.h"

#include <QSGGeometryNode>
#include <QSGFlatColorMaterial>
#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QPainter>
#include <vector>
#include <cstdint>
#include <cmath>
#include "qsgdatatexture.h"

#ifndef M_PI
#define M_PI		3.14159265358979323846
#endif

#define GLSL(ver, src) "#version " #ver "\n" #src


class XYMarkerMaterial : public QSGMaterial
{
public:
    XYMarkerMaterial() = default;
    QSGMaterialType *type() const override {
        static QSGMaterialType type;
        return &type;
    }
    QSGMaterialShader *createShader() const override;
    QSizeF m_size;
    QSizeF m_scale;
    QPointF m_offset;
    QColor m_markercolor;
    double m_markersize = 0;
    int m_markersegments = 0;
    bool m_markerborder = false;
    QSGDataTexture<uint8_t> m_markerimage;
};

class XYMarkerMaterialShader : public QSGMaterialShader
{
public:
    const char *vertexShader() const override {
        return GLSL(130,
            in highp vec4 vertex;
            uniform highp mat4 matrix;
            uniform highp vec2 size;
            uniform highp vec2 scale;
            uniform highp vec2 offset;
            uniform float msize;

            void main() {
                highp vec2 p = (vertex.xy - offset) * scale * size;
                gl_Position = matrix * vec4(p.x, size.y - p.y, 0., 1.);
                gl_PointSize = msize;
            }
        );
    }

    const char *fragmentShader() const override {
        return GLSL(130,
            uniform lowp float opacity;
            uniform lowp vec4 mcolor;
            uniform sampler2D mimage;
            out vec4 fragColor;

            void main() {
                lowp vec4 color = mcolor * texture(mimage, gl_PointCoord.xy);
                lowp float o = opacity * color.a;
                fragColor = vec4(color.rgb * o, o);
            }
        );
    }

    char const *const *attributeNames() const override {
        static char const *const names[] = { "vertex", nullptr };
        return names;
    }

    void initialize() override {
        QSGMaterialShader::initialize();
        QOpenGLShaderProgram* p = program();

        m_id_matrix = p->uniformLocation("matrix");
        m_id_opacity = p->uniformLocation("opacity");
        m_id_size = p->uniformLocation("size");
        m_id_scale = p->uniformLocation("scale");
        m_id_offset = p->uniformLocation("offset");
        m_id_msize = p->uniformLocation("msize");
        m_id_mcolor = p->uniformLocation("mcolor");
        m_id_mimage = p->uniformLocation("mimage");
    }

    void activate() override {
        QOpenGLFunctions *glFuncs = QOpenGLContext::currentContext()->functions();
        glFuncs->glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);
        glFuncs->glEnable(GL_POINT_SPRITE); // TODO: this is deprecated, but appears to be required on NVidia systems
    }

    void updateState(const RenderState& state, QSGMaterial* newMaterial, QSGMaterial*) override {
        Q_ASSERT(program()->isLinked());
        auto* material = static_cast<XYMarkerMaterial*>(newMaterial);
        QOpenGLShaderProgram* p = program();

        if (state.isMatrixDirty()) {
            program()->setUniformValue(m_id_matrix, state.combinedMatrix());
        }
        if (state.isOpacityDirty()) {
            program()->setUniformValue(m_id_opacity, state.opacity());
        }

        // bind material parameters
        p->setUniformValue(m_id_size, material->m_size);
        p->setUniformValue(m_id_scale, material->m_scale);
        p->setUniformValue(m_id_offset, material->m_offset);
        p->setUniformValue(m_id_msize, float(material->m_markersize));
        p->setUniformValue(m_id_mcolor, material->m_markercolor);

        // bind texture
        p->setUniformValue(m_id_mimage, 0);
        material->m_markerimage.bind();
    }

private:
    int m_id_matrix;
    int m_id_opacity;
    int m_id_size;
    int m_id_scale;
    int m_id_offset;
    int m_id_msize;
    int m_id_mcolor;
    int m_id_mimage;
};

inline QSGMaterialShader* XYMarkerMaterial::createShader() const { return new XYMarkerMaterialShader; }


class XYLineMaterial : public QSGMaterial
{
public:
    QSGMaterialType* type() const override {
        static QSGMaterialType type;
        return &type;
    }
    QSGMaterialShader* createShader() const override;
    QSizeF m_size;
    QSizeF m_scale;
    QPointF m_offset;
    QColor m_color;
};

class XYLineMaterialShader : public QSGMaterialShader
{
public:
    const char* vertexShader() const override {
        return GLSL(130,
            in highp vec4 vertex;
            uniform highp mat4 matrix;
            uniform highp vec2 size;
            uniform highp vec2 scale;
            uniform highp vec2 offset;

            void main() {
                highp vec2 p = (vertex.xy - offset) * scale * size;
                gl_Position = matrix * vec4(p.x, size.y - p.y, 0., 1.);
            }
        );
    }

    const char* fragmentShader() const override {
        return GLSL(130,
            uniform lowp vec4 color;
            uniform lowp float opacity;
            out vec4 fragColor;

            void main() {
                fragColor = vec4(color.rgb*color.a, color.a) * opacity;
            }
        );
    }

    char const *const *attributeNames() const override
    {
        static char const *const names[] = {"vertex", nullptr};
        return names;
    }

    void initialize() override {
        QSGMaterialShader::initialize();
        QOpenGLShaderProgram* p = program();

        m_id_matrix = p->uniformLocation("matrix");
        m_id_opacity = p->uniformLocation("opacity");
        m_id_size = p->uniformLocation("size");
        m_id_scale = p->uniformLocation("scale");
        m_id_offset = p->uniformLocation("offset");
        m_id_color = p->uniformLocation("color");
    }

    void updateState(const RenderState& state, QSGMaterial* newMaterial, QSGMaterial*) override {
        Q_ASSERT(program()->isLinked());
        auto* material = static_cast<XYLineMaterial*>(newMaterial);
        QOpenGLShaderProgram* p = program();

        if (state.isMatrixDirty()) {
            program()->setUniformValue(m_id_matrix, state.combinedMatrix());
        }
        if (state.isOpacityDirty()) {
            program()->setUniformValue(m_id_opacity, state.opacity());
        }

        // bind material parameters
        p->setUniformValue(m_id_size, material->m_size);
        p->setUniformValue(m_id_scale, material->m_scale);
        p->setUniformValue(m_id_offset, material->m_offset);
        p->setUniformValue(m_id_color, material->m_color);
    }

private:
    int m_id_matrix;
    int m_id_opacity;
    int m_id_size;
    int m_id_scale;
    int m_id_offset;
    int m_id_color;
};

inline QSGMaterialShader* XYLineMaterial::createShader() const { return new XYLineMaterialShader; }


class XYFillMaterial : public QSGMaterial
{
public:
    QSGMaterialType* type() const override {
        static QSGMaterialType type;
        return &type;
    }
    QSGMaterialShader* createShader() const override;
    QSizeF m_size;
    QSizeF m_scale;
    QPointF m_offset;
    QColor m_color;
};

class XYFillMaterialShader : public QSGMaterialShader
{
public:
    const char* vertexShader() const override {
        return GLSL(130,
            in highp vec4 vertex;
            uniform highp mat4 matrix;
            uniform highp vec2 size;
            uniform highp vec2 scale;
            uniform highp vec2 offset;

            void main() {
                highp vec2 p = (vertex.xy - offset) * scale * size;
                gl_Position = matrix * vec4(p.x, size.y - p.y, 0., 1.);
            }
        );
    }

    const char* fragmentShader() const override {
        return GLSL(130,
            uniform lowp vec4 color;
            uniform lowp float opacity;
            out vec4 fragColor;

            void main() {
                fragColor = vec4(color.rgb*color.a, color.a) * opacity;
            }
        );
    }

    char const *const *attributeNames() const override {
        static char const *const names[] = {"vertex", nullptr};
        return names;
    }

    void initialize() override {
        QSGMaterialShader::initialize();
        QOpenGLShaderProgram* p = program();

        m_id_matrix = p->uniformLocation("matrix");
        m_id_opacity = p->uniformLocation("opacity");
        m_id_size = p->uniformLocation("size");
        m_id_scale = p->uniformLocation("scale");
        m_id_offset = p->uniformLocation("offset");
        m_id_color = p->uniformLocation("color");
    }

    void updateState(const RenderState &state, QSGMaterial *newMaterial, QSGMaterial *) override {
        Q_ASSERT(program()->isLinked());
        auto* material = static_cast<XYFillMaterial*>(newMaterial);
        QOpenGLShaderProgram* p = program();

        if (state.isMatrixDirty()) {
            program()->setUniformValue(m_id_matrix, state.combinedMatrix());
        }
        if (state.isOpacityDirty()) {
            program()->setUniformValue(m_id_opacity, state.opacity());
        }

        // bind material parameters
        p->setUniformValue(m_id_size, material->m_size);
        p->setUniformValue(m_id_scale, material->m_scale);
        p->setUniformValue(m_id_offset, material->m_offset);
        p->setUniformValue(m_id_color, material->m_color);
    }

private:
    int m_id_matrix;
    int m_id_opacity;
    int m_id_size;
    int m_id_scale;
    int m_id_offset;
    int m_id_color;
};

inline QSGMaterialShader* XYFillMaterial::createShader() const { return new XYFillMaterialShader; }


XYPlot::XYPlot(QQuickItem *parent) : DataClient(parent)
{
    setFlag(QQuickItem::ItemHasContents);
    setClip(true);
}

void XYPlot::setViewRect(const QRectF &viewrect)
{
    if (viewrect != m_view_rect) {
        m_view_rect = viewrect;
        emit viewRectChanged(m_view_rect);
        update();
    }
}

void XYPlot::setFillEnabled(bool enabled)
{
    if (m_fill != enabled) {
        m_fill = enabled;
        emit fillEnabledChanged(m_fill);
        update();
    }
}

void XYPlot::setFillColor(const QColor &color)
{
    if (m_fillcolor != color) {
        m_fillcolor = color;
        emit fillColorChanged(m_fillcolor);
        update();
    }
}

void XYPlot::setLineEnabled(bool enabled)
{
    if (m_line != enabled) {
        m_line = enabled;
        emit lineEnabledChanged(m_line);
        update();
    }
}

void XYPlot::setLineWidth(double width)
{
    if (m_linewidth != width) {
        m_linewidth = width;
        emit lineWidthChanged(m_linewidth);
        update();
    }
}

void XYPlot::setLineColor(const QColor &color)
{
    if (m_linecolor != color) {
        m_linecolor = color;
        emit lineColorChanged(m_linecolor);
        update();
    }
}

void XYPlot::setMarkerEnabled(bool enabled)
{
    if (m_marker != enabled) {
        m_marker = enabled;
        emit lineEnabledChanged(m_marker);
        update();
    }
}

void XYPlot::setMarkerSegments(int n)
{
    if (m_markersegments != n) {
        m_markersegments = n;
        emit markerSegmentsChanged(static_cast<int>(m_markersegments));
        update();
    }
}

void XYPlot::setMarkerSize(double size)
{
    if (m_markersize != size) {
        m_markersize = size;
        emit markerSizeChanged(m_markersize);
        update();
    }
}

void XYPlot::setMarkerColor(const QColor &color)
{
    if (m_markercolor != color) {
        m_markercolor = color;
        emit markerColorChanged(m_markercolor);
        update();
    }
}

void XYPlot::setMarkerBorder(bool enabled)
{
    if (m_markerborder != enabled) {
        m_markerborder = enabled;
        emit markerBorderChanged(m_markerborder);
        update();
    }
}

void XYPlot::setLogY(bool enabled)
{
    if (m_logy != enabled) {
        m_logy = enabled;
        emit logYChanged(m_logy);
        m_new_data = true;
        update();
    }
}


class FillNode : public QSGGeometryNode
{
public:
    FillNode() {
        QSGGeometry* geometry;
        geometry = new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), 0);
        geometry->setDrawingMode(GL_TRIANGLE_STRIP);
        QSGMaterial* material;
        material = new XYFillMaterial;
        setGeometry(geometry);
        setFlag(QSGNode::OwnsGeometry);
        setMaterial(material);
        setFlag(QSGNode::OwnsMaterial);
    }
    ~FillNode() override = default;
    bool isSubtreeBlocked() const override {
        return m_blocked;
    }
    bool m_blocked = false;
    bool m_data_valid = false;
};


class LineNode : public QSGGeometryNode
{
public:
    LineNode() {
        QSGGeometry* geometry;
        geometry = new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), 0);
        geometry->setDrawingMode(GL_LINE_STRIP);
        QSGMaterial* material;
        material = new XYLineMaterial;
        setGeometry(geometry);
        setFlag(QSGNode::OwnsGeometry);
        setMaterial(material);
        setFlag(QSGNode::OwnsMaterial);
    }
    ~LineNode() override = default;
    bool isSubtreeBlocked() const override {
        return m_blocked;
    }
    bool m_blocked = false;
    bool m_data_valid = false;
};


class MarkerNode : public QSGGeometryNode
{
public:
    MarkerNode() {
        QSGGeometry* geometry;
        geometry = new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), 0);
        geometry->setDrawingMode(GL_POINTS);
        QSGMaterial* material;
        material = new XYMarkerMaterial;
        setGeometry(geometry);
        setFlag(QSGNode::OwnsGeometry);
        setMaterial(material);
        setFlag(QSGNode::OwnsMaterial);
    }
    ~MarkerNode() override = default;
    bool isSubtreeBlocked() const override {
        return m_blocked;
    }
    bool m_blocked = false;
    bool m_data_valid = false;
};


static void paintPolygon(QImage& img, unsigned int segments, bool border) {
    int size = img.width();
    QPainter p(&img);
    if (!border) {
        p.setPen(Qt::NoPen);
    }
    p.setBrush(QColor(Qt::white));

    if (segments != 0) {
        auto points = std::vector<QPointF>(segments);
        double r = size * .5;
        double dphi = 2.*M_PI * (1./segments);
        for (unsigned int i = 0; i < segments; ++i) {
            double x = size*.5 - r * sin(i*dphi);
            double y = size*.5 - r * cos(i*dphi);
            points[i].setX(x);
            points[i].setY(y);
        }
        p.drawConvexPolygon(points.data(), static_cast<int>(segments));
    } else {
        p.drawEllipse(1, 1, size-2, size-2);
    }
}

QSGNode *XYPlot::updatePaintNode(QSGNode *n, QQuickItem::UpdatePaintNodeData *)
{
    FillNode* n_fill;
    LineNode* n_line;
    MarkerNode* n_marker;
    QSGGeometry *fgeometry;
    XYFillMaterial *fmaterial;
    QSGGeometry *lgeometry;
    XYLineMaterial *lmaterial;
    QSGGeometry *mgeometry;
    XYMarkerMaterial *mmaterial;
    QSGNode::DirtyState dirty_state = QSGNode::DirtyMaterial;

    if (n == nullptr) {
        n = new QSGNode;
    }

    if (m_source == nullptr) {
        // remove child nodes if there is no data source
        if (n->childCount() != 0) {
            n_fill = static_cast<FillNode*>(n->childAtIndex(0));
            n_line = static_cast<LineNode*>(n->childAtIndex(1));
            n_marker = static_cast<MarkerNode*>(n->childAtIndex(2));
            n->removeAllChildNodes();
            delete n_fill;
            delete n_line;
            delete n_marker;
        }
        // return empty node
        return n;
    }

    if (n->childCount() == 0) {
        // append child nodes for fill, line and markers
        n_fill = new FillNode;
        n_line = new LineNode;
        n_marker = new MarkerNode;
        n_fill->setFlag(QSGNode::OwnedByParent);
        n_line->setFlag(QSGNode::OwnedByParent);
        n_marker->setFlag(QSGNode::OwnedByParent);
        n->appendChildNode(n_fill);
        n->appendChildNode(n_line);
        n->appendChildNode(n_marker);
    }

    // ** graph node and data source can be considered valid from here on **
    n_fill = static_cast<FillNode*>(n->childAtIndex(0));
    n_line = static_cast<LineNode*>(n->childAtIndex(1));
    n_marker = static_cast<MarkerNode*>(n->childAtIndex(2));
    fgeometry = n_fill->geometry();
    lgeometry = n_line->geometry();
    mgeometry = n_marker->geometry();
    fmaterial = static_cast<XYFillMaterial*>(n_fill->material());
    lmaterial = static_cast<XYLineMaterial*>(n_line->material());
    mmaterial = static_cast<XYMarkerMaterial*>(n_marker->material());

    // check if fill, line or markers were switched on or off
    if (n_fill->m_blocked == m_fill || n_line->m_blocked == m_line || n_marker->m_blocked == m_marker) {
        n_fill->m_blocked = !m_fill;
        n_line->m_blocked = !m_line;
        n_marker->m_blocked = !m_marker;
        dirty_state |= QSGNode::DirtySubtreeBlocked;
    }

    int num_data_points = m_source->dataWidth() / 2;
    double xmin = m_view_rect.left();
    double ymin = m_view_rect.top();
    double xrange = m_view_rect.width();
    double yrange = m_view_rect.height();

    if (m_fill) {
        // update fill material parameters
        fmaterial->m_size.setWidth(width());
        fmaterial->m_size.setHeight(height());
        fmaterial->m_scale.setWidth(1. / xrange);
        fmaterial->m_scale.setHeight(1. / yrange);
        fmaterial->m_offset.setX(xmin);
        fmaterial->m_offset.setY(ymin);
        fmaterial->m_color = m_fillcolor;
        fmaterial->setFlag(QSGMaterial::Blending, m_fillcolor.alphaF() != 1.);

        // reallocate geometry if number of points changed
        if (fgeometry->vertexCount() != (2*num_data_points)) {
            fgeometry->allocate(2*num_data_points);
            m_new_data = true;
        }
    }

    if (m_line) {
        // update line material parameters
        lmaterial->m_size.setWidth(width());
        lmaterial->m_size.setHeight(height());
        lmaterial->m_scale.setWidth(1. / xrange);
        lmaterial->m_scale.setHeight(1. / yrange);
        lmaterial->m_offset.setX(xmin);
        lmaterial->m_offset.setY(ymin);
        lmaterial->m_color = m_linecolor;
        lmaterial->setFlag(QSGMaterial::Blending, m_linecolor.alphaF() != 1.);
        lgeometry->setLineWidth(static_cast<float>(m_linewidth));

        // reallocate geometry if number of points changed
        if (lgeometry->vertexCount() != num_data_points) {
            lgeometry->allocate(num_data_points);
            m_new_data = true;
        }
    }

    if (m_marker) {
        // update marker image
        if (mmaterial->m_markersize != m_markersize ||
                mmaterial->m_markersegments != m_markersegments ||
                mmaterial->m_markerborder != m_markerborder) {
            auto image_size = static_cast<int>(std::ceil(m_markersize));
            uint8_t* data = mmaterial->m_markerimage.allocateData2D(image_size, image_size, 4);
            QImage qimage(data, image_size, image_size, QImage::Format_ARGB32);
            qimage.fill(0x00ffffff);
            paintPolygon(qimage, static_cast<unsigned int>(m_markersegments), m_markerborder);
            mmaterial->m_markerimage.commitData();
        }

        // update marker material parameters
        mmaterial->m_size.setWidth(width());
        mmaterial->m_size.setHeight(height());
        mmaterial->m_scale.setWidth(1. / xrange);
        mmaterial->m_scale.setHeight(1. / yrange);
        mmaterial->m_offset.setX(xmin);
        mmaterial->m_offset.setY(ymin);
        mmaterial->m_markersegments = m_markersegments;
        mmaterial->m_markerborder = m_markerborder;
        mmaterial->m_markercolor = m_markercolor;
        mmaterial->m_markersize = m_markersize;
        mmaterial->setFlag(QSGMaterial::Blending);

        // reallocate geometry if number of points changed
        if (mgeometry->vertexCount() != num_data_points) {
            mgeometry->allocate(num_data_points);
            m_new_data = true;
        }
    }

    // update geometry if new data is available
    if (m_new_source || m_new_data) {
        n_fill->m_data_valid = false;
        n_line->m_data_valid = false;
        n_marker->m_data_valid = false;
        m_new_source = false;
        m_new_data = false;
    }

    auto* src = reinterpret_cast<double*>(m_source->data());

    if (m_fill && !n_fill->m_data_valid) {
        auto* fdst = static_cast<float*>(fgeometry->vertexData());
        for (int i = 0; i < num_data_points; ++i) {
            fdst[4*i+0] = static_cast<float>(src[2*i+0]);
            fdst[4*i+1] = 0.;
            fdst[4*i+2] = static_cast<float>(src[2*i+0]);
            fdst[4*i+3] = static_cast<float>(src[2*i+1]);
        }
        dirty_state |= QSGNode::DirtyGeometry;
        n_fill->m_data_valid = true;
    }

    if (m_line && !n_line->m_data_valid) {
        auto* ldst = static_cast<float*>(lgeometry->vertexData());
        if (m_logy) {
            for (int i = 0; i < num_data_points; ++i) {
                ldst[i*2 + 0] = static_cast<float>(src[i*2 + 0]);
                ldst[i*2 + 1] = static_cast<float>(std::log10(src[i*2 + 1]));
            }
        } else {
            for (int i = 0; i < num_data_points*2; ++i) {
                ldst[i] = static_cast<float>(src[i]);
            }
        }
        dirty_state |= QSGNode::DirtyGeometry;
        n_line->m_data_valid = true;
    }

    if (m_marker && !n_marker->m_data_valid) {
        auto* mdst = static_cast<float*>(mgeometry->vertexData());
        if (m_logy) {
            for (int i = 0; i < num_data_points; ++i) {
                mdst[i*2 + 0] = static_cast<float>(src[i*2 + 0]);
                mdst[i*2 + 1] = static_cast<float>(std::log10(src[i*2 + 1]));
            }
        } else {
            for (int i = 0; i < num_data_points*2; ++i) {
                mdst[i] = static_cast<float>(src[i]);
            }
        }
        dirty_state |= QSGNode::DirtyGeometry;
        n_marker->m_data_valid = true;
    }

    n->markDirty(dirty_state);
    n_fill->markDirty(dirty_state);
    n_line->markDirty(dirty_state);
    n_marker->markDirty(dirty_state);
    return n;
}
