#include "mainwidget.h"

#include <QMouseEvent>

#include <math.h>

MainWidget::MainWidget(QWidget *parent) :
    QOpenGLWidget(parent),
    geometries(0),
    angularSpeed(0)
{}

MainWidget::~MainWidget() {
    // Make sure the context is current when deleting the texture
    // and the buffers.
    makeCurrent();
    delete geometries;
    doneCurrent();
}

void MainWidget::mousePressEvent(QMouseEvent *e) {
    // Save mouse press position
    mousePressPosition = QVector2D(e->localPos());
}

void MainWidget::mouseReleaseEvent(QMouseEvent *e) {
    // Mouse release position - mouse press position
    QVector2D diff = QVector2D(e->localPos()) - mousePressPosition;

    // Rotation axis is perpendicular to the mouse position difference
    // vector
    QVector3D n = QVector3D(diff.y(), diff.x(), 0.0).normalized();

    // Accelerate angular speed relative to the length of the mouse sweep
    qreal acc = diff.length() / 100.0;

    // Calculate new rotation axis as weighted sum
    rotationAxis = (rotationAxis * angularSpeed + n * acc).normalized();

    // Increase angular speed
    angularSpeed += acc;
}

void MainWidget::timerEvent(QTimerEvent *)
{
    // Decrease angular speed (friction)
    angularSpeed *= 0.99;

    // Stop rotation when speed goes below threshold
    if (angularSpeed < 0.01) {
        angularSpeed = 0.0;
    } else {
        // Update rotation
        rotation = QQuaternion::fromAxisAndAngle(rotationAxis, angularSpeed) * rotation;

        // Request an update
        update();
    }
}

void MainWidget::initializeGL()
{
    initializeOpenGLFunctions();

    glClearColor(1, 1, 1, 1);

    initShaders();

    // Enable depth buffer
    glEnable(GL_DEPTH_TEST);

    // Enable back face culling
    //glEnable(GL_CULL_FACE);

    // Enable point size manipulation
    glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);
    glEnable(GL_POINT_SMOOTH);

    geometries = new GeometryEngine;

    // Use QBasicTimer because its faster than QTimer
    timer.start(12, this);
}

void MainWidget::initShaders() {
    QString vshader;
    QString fshader;
    vshader = ":/vshader.glsl";
    fshader = ":/fshader.glsl";

    // Compile vertex shader
    if (!program.addShaderFromSourceFile(QOpenGLShader::Vertex, vshader))
        close();

    // Compile fragment shader
    if (!program.addShaderFromSourceFile(QOpenGLShader::Fragment, fshader))
        close();

    // Link shader pipeline
    if (!program.link())
        close();

    // Bind shader pipeline for use
    if (!program.bind())
        close();
}

void MainWidget::resizeGL(int w, int h) {
    // Calculate aspect ratio
    qreal aspect = qreal(w) / qreal(h ? h : 1);

    // Set near plane to 3.0, far plane to 7.0, field of view 45 degrees
    const qreal zNear = 3.0, zFar = 500, fov = 45.0;

    // Reset projection
    projection.setToIdentity();

    // Set perspective projection
    projection.perspective(fov, aspect, zNear, zFar);
}

void MainWidget::paintGL() {
    // Clear color and depth buffer
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    drawCube();
}

void MainWidget::drawMan() {
    QMatrix4x4 t;
    t.translate(0,0,-25);

    // BODY
    // Calculate model view transformation
    QMatrix4x4 matrix;
    matrix.translate(0.0, 0, -25);
    matrix.rotate(rotation);
    matrix.scale(3,5,2);
    // Set modelview-projection matrix
    program.setUniformValue("mvp", projection * matrix);
    // Draw cube geometry
    geometries->drawGeometry(&program);

    // HEAD
    // Calculate model view transformation
    QMatrix4x4 matrixHead;
    matrixHead.rotate(rotation);
    matrixHead.translate(0.0, 3.5, 0);
    matrixHead.scale(2,2,2);
    // Set modelview-projection matrix
    program.setUniformValue("mvp", projection * t * matrixHead);
    // Draw cube geometry
    geometries->drawGeometry(&program);

    // ARM1
    // Calculate model view transformation
    QMatrix4x4 matrixArm1;
    matrixArm1.rotate(rotation);
    matrixArm1.translate(2, 0.5, 0);
    matrixArm1.scale(1,4,1);
    // Set modelview-projection matrix
    program.setUniformValue("mvp", projection * t * matrixArm1);
    // Draw cube geometry
    geometries->drawGeometry(&program);

    // ARM2
    // Calculate model view transformation
    QMatrix4x4 matrixArm2;
    matrixArm2.rotate(rotation);
    matrixArm2.translate(-2, 3.5, 0);
    matrixArm2.scale(1,4,1);
    // Set modelview-projection matrix
    program.setUniformValue("mvp", projection * t * matrixArm2);
    // Draw cube geometry
    geometries->drawGeometry(&program);

    // LEG1
    // Calculate model view transformation
    QMatrix4x4 matrixLeg1;
    matrixLeg1.rotate(rotation);
    matrixLeg1.translate(1, -5, 0);
    matrixLeg1.scale(1,5,1);
    // Set modelview-projection matrix
    program.setUniformValue("mvp", projection * t * matrixLeg1);
    // Draw cube geometry
    geometries->drawGeometry(&program);

    // LEG2
    // Calculate model view transformation
    QMatrix4x4 matrixLeg2;
    matrixLeg2.rotate(rotation);
    matrixLeg2.translate(-1, -5, 0);
    matrixLeg2.scale(1,5,1);
    // Set modelview-projection matrix
    program.setUniformValue("mvp", projection * t * matrixLeg2);
    // Draw cube geometry
    geometries->drawGeometry(&program);
}

void MainWidget::drawCube() {
    // Calculate model view transformation
    QMatrix4x4 matrix;
    matrix.translate(0.0, 0, -10);
    matrix.rotate(rotation);
    matrix.scale(3,3,3);
    // Set modelview-projection matrix
    program.setUniformValue("mvp", projection * matrix);
    // Draw cube geometry
    geometries->drawGeometry(&program);
}
