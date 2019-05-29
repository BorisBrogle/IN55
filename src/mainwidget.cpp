#include "mainwidget.h"
#include "globals.h"


MainWidget::MainWidget(QWidget *parent) :
    QOpenGLWidget(parent),
    geometries(0),
    angularSpeed(0),
    zoomTranslation(0.0f)
{
    QVBoxLayout *layout = new QVBoxLayout(this);

    layout->addWidget(this->addLabel("Global size :"));
    layout->addWidget(this->addSlider(parameters.globalSizeFactor, 0.1, 2));

    layout->addWidget(this->addLabel("Curvature variance :"));
    layout->addWidget(this->addSlider(parameters.curvatureVariance, 0.008, 0.5));

    layout->addWidget(this->addLabel("Radius at base :"));
    layout->addWidget(this->addSlider(parameters.radiusAtBaseFactor, 1, 4));

    layout->addWidget(this->addLabel("Stem height part :"));
    layout->addWidget(this->addSlider(parameters.stemHeightPart, 0.1, 0.9));

    layout->addWidget(this->addLabel("Perlin power :"));
    layout->addWidget(this->addSlider(parameters.capGlobalPerlinPower, 0.10, 2));

    layout->addWidget(this->addLabel("Holes density :"));
    layout->addWidget(this->addSlider(parameters.holesDensityFactor, 0.10, 2));

    QPushButton *submitButton = new QPushButton("Regenerate \n a morel", this);
    submitButton->setFixedSize(100,50);
    QObject::connect(submitButton, SIGNAL(clicked()), this, SLOT(redrawMorel()));
    layout->addWidget(submitButton,0,Qt::AlignTop);

    this->setLayout(layout);
}

QLabel* MainWidget::addLabel(QString text) {
    QLabel *label = new QLabel(this);
    label->setText(text);
    label->setFixedSize(100,20);
    return label;
}

QSlider* MainWidget::addSlider(double value, double min, double max) {
    QSlider *slider = new QSlider(Qt::Horizontal);
    slider->setFocusPolicy(Qt::StrongFocus);
    slider->setSingleStep(1);
    slider->setFixedSize(100,20);

    slider->setValue(100*abs(value-min)/abs(max-min));

    SliderParameters p = {min, max, value, slider};
    this->sliders.append(p);
    return slider;
}

void MainWidget::redrawMorel(){
    for(auto&& s: this->sliders) {
        double sval = s.slider->value()/100.0;
        s.value = s.min+sval*abs(s.max-s.min);
    }

    parameters.globalSizeFactor = this->sliders[0].value;
    parameters.curvatureVariance = this->sliders[1].value;
    parameters.radiusAtBaseFactor = this->sliders[2].value;
    parameters.stemHeightPart = this->sliders[3].value;
    parameters.capGlobalPerlinPower = this->sliders[4].value;
    parameters.holesDensityFactor = this->sliders[5].value;

    this->geometries->initGeometry();
    this->drawCube();
}

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

void MainWidget::wheelEvent(QWheelEvent *e){
    if(e->delta() < 0){
        zoomTranslation -= 0.5f;
    }
    else{
        zoomTranslation += 0.5f;
    }
    update();
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

    glClearColor(120.0/255.0, 120.0/255.0, 120.0/255.0, 1);

    initShaders();

    // Enable depth buffer
    glEnable(GL_DEPTH_TEST);

    // Enable back face culling
    //glEnable(GL_CULL_FACE);
    // glCullFace(GL_FRONT);

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

void MainWidget::drawCube() {
    // Calculate model view transformation
    QMatrix4x4 matrix;
    matrix.translate(0, 0, -10+zoomTranslation);
    matrix.rotate(rotation);
    matrix.scale(1.5,1.5,1.5);
    program.setUniformValue("mv", matrix);
    QMatrix4x4 normal = (matrix.inverted()).transposed();
    program.setUniformValue("nm", normal);
    // Set modelview-projection matrix
    program.setUniformValue("mvp", projection * matrix);

    //lights positions
    int nbLightsEnabled = 1;
    int IndexLightsEnabled[nbLightsEnabled];
    IndexLightsEnabled[0] = 4;

    int nbLights = 5;
    QVector3D LightPositions[nbLights];
    LightPositions[0] = QVector3D(matrix*QVector4D(-3.0f, 2.0f, 3.0f,1.0f));
    LightPositions[1] = QVector3D(matrix*QVector4D(3.0f, 2.0f, 3.0f,1.0f));
    LightPositions[2] = QVector3D(matrix*QVector4D(3.0f, -2.0f, 3.0f,1.0f));
    LightPositions[3] = QVector3D(matrix*QVector4D(-3.0f, -2.0f, 3.0f,1.0f));
    LightPositions[4] = QVector3D(0.0f,0.0f,0.0f); //Camera light

    QVector3D LEP[nbLightsEnabled];
    for(int i=0; i<nbLightsEnabled; i++){
        LEP[i] = LightPositions[IndexLightsEnabled[i]];
    }

    program.setUniformValue("nbLights", nbLightsEnabled);
    program.setUniformValueArray("LightPositions", LEP, nbLightsEnabled);

    // Draw cube geometry
    geometries->drawGeometry(&program);
}
