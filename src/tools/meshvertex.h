#ifndef MESHVERTEX_H
#define MESHVERTEX_H

#include <QtMath>
#include <QVector3D>
#include <QVector4D>
#include <QMatrix4x4>
#include <QQuaternion>
#include <QOpenGLFunctions>

class MeshVertex
{
public:
    MeshVertex();

    GLushort id;
    QVector3D position;

    float baseAngle; // Used on Perlin's computation
    float baseHeight; // Used on Perlin's computation
    int layer;

    QVector3D color;
    MeshVertex* top;
    MeshVertex* bottom;
    MeshVertex* right;
    MeshVertex* left;

    void computeNormalVector();
    void rescale(QVector3D scale);
    void rescale(float scale);
    void translate(QVector3D axis);
    void rotate(QQuaternion rotationQuat);
    void rotate(QQuaternion rotationQuat, QVector3D point);
    void rotate(float angle, QVector3D axis);
    void rotate(float angle, QVector3D axis, QVector3D point);
};

#endif // MESHVERTEX_H
