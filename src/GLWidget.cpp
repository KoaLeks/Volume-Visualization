/*
* Copyright (C) 2019
* Institute of Visual Computing and Human-Centered Technology, TU Wien
* Written by Tobias Klein <tklein@cg.tuwien.ac.at>
* All rights reserved.
*/

#include "GLWidget.h"

#include "glsw.h"
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <QMouseEvent>
#include <QDir>
#include <qopenglwidget.h>
#include <qmessagebox.h>
#include <qopenglframebufferobject.h>
#include <QOpenGLTexture>

#include "MainWindow.h"

#define GL_GPU_MEM_INFO_TOTAL_AVAILABLE_MEM_NVX 0x9048
#define GL_GPU_MEM_INFO_CURRENT_AVAILABLE_MEM_NVX 0x9049

GLfloat cube_vertices[] = {
	// front
	-1.0, -1.0,  1.0,
	1.0, -1.0,  1.0,
	1.0,  1.0,  1.0,
	-1.0,  1.0,  1.0,
	// back
	-1.0, -1.0, -1.0,
	1.0, -1.0, -1.0,
	1.0,  1.0, -1.0,
	-1.0,  1.0, -1.0,
};

GLfloat tex_coordinates[] = {
	// front
	0.0, 0.0,  1.0,
	1.0, 0.0,  1.0,
	1.0,  1.0,  1.0,
	0.0,  1.0,  1.0,
	// back
	0.0, 0.0, 0.0,
	1.0, 0.0, 0.0,
	1.0,  1.0, 0.0,
	0.0,  1.0, 0.0,
};

GLushort cube_elements[] = {
	// front
	0, 1, 2,
	2, 3, 0,
	// top
	1, 5, 6,
	6, 2, 1,
	// back
	7, 6, 5,
	5, 4, 7,
	// bottom
	4, 0, 3,
	3, 7, 4,
	// left
	4, 5, 1,
	1, 0, 4,
	// right
	3, 2, 6,
	6, 7, 3,
};

GLfloat quad_vertices[] =
{
	-1.0f, -1.0f, 0.0f,
	1.0f, -1.0f, 0.0f,
	1.0f, 1.0f, 0.0f,
	-1.0f, 1.0f, 0.0f
};

GLushort quad_elements[] =
{
	0, 1, 2,
	0, 2, 3
};


const QString shaderPath1 = "/src/shader/cube.glsl";
const QString shaderPath2 = "/src/shader/raycasting.glsl";



GLWidget::GLWidget(QWidget *parent, MainWindow *mainWindow)
	: QOpenGLWidget(parent),
	m_vertexBuffer(QOpenGLBuffer::VertexBuffer),
	m_indexBuffer(QOpenGLBuffer::IndexBuffer),
	m_vertexBufferQuad(QOpenGLBuffer::VertexBuffer),
	m_indexBufferQuad(QOpenGLBuffer::IndexBuffer)
{
	m_MainWindow = mainWindow;
	m_renderingMode = 0;
	m_VolumeTexture = NULL;
	QSurfaceFormat format;
	// asks for a OpenGL 3.2 debug context using the Core profile
	format.setMajorVersion(3);
	format.setMinorVersion(2);
	format.setProfile(QSurfaceFormat::CoreProfile);
	format.setOption(QSurfaceFormat::DebugContext);

	setFormat(format);

	// we use a file watcher to recompile shaders on the fly
	// you can change shaders and directly see the result while the program is running
	m_fileWatcher = new QFileSystemWatcher(this);
	connect(m_fileWatcher, SIGNAL(fileChanged(const QString &)), this, SLOT(fileChanged(const QString &)));

	m_fileWatcher->addPath(QDir::currentPath() + shaderPath1);
	m_fileWatcher->addPath(QDir::currentPath() + shaderPath2);

	initglsw(); // initializes the OpenGL Shader Wrangler
}


GLWidget::~GLWidget()
{
	glswShutdown(); // shuts down the OpenGL Shader Wrangler
}

// initializes the OpenGL Shader Wrangler
void GLWidget::initglsw()
{
	glswInit();
	QString shader_folder = QDir::currentPath() + "/src/shader/";
	QByteArray ba = shader_folder.toLatin1();
	const char *c_str = ba.data();
	glswSetPath(c_str, ".glsl");
	qDebug() << "glsw path: " << c_str;
	glswAddDirectiveToken("", "#version 330");
}

// the rendering routing, this function is called for every frame
void GLWidget::paintGL()
{
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);

	// the matrix must be transposed when it is passed from glm to QT!
	QMatrix4x4 modelView = QMatrix4x4(glm::value_ptr(m_camera.getViewMatrix())).transposed();
	QMatrix4x4 projection = QMatrix4x4(glm::value_ptr(m_camera.getProjectionMatrix())).transposed();

	QOpenGLVertexArrayObject::Binder vaoBinder(&m_vaoCube);
	m_programCube->bind();

	m_programCube->setUniformValue(m_programCube->uniformLocation("mvMatrix"), modelView);
	m_programCube->setUniformValue(m_programCube->uniformLocation("projMatrix"), projection);


	// 1. render front faces to FBO
	glCullFace(GL_BACK);
	m_FBO_frontFaces->bind();
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_SHORT, 0);
	m_FBO_frontFaces->release();
	m_programCube->release();
	vaoBinder.release();

	QOpenGLVertexArrayObject::Binder vaoBinder2(&m_vaoQuad);
	m_programVolume->bind();
	glEnable(GL_TEXTURE_2D);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_FBO_frontFaces->texture());
	m_programVolume->setUniformValue(m_programVolume->uniformLocation("frontFaces"), 0);
	m_programVolume->setUniformValue(m_programVolume->uniformLocation("renderingMode"), m_renderingMode);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);
	m_programVolume->release();
	vaoBinder2.release();

	// 2. render back faces to FBO

	vaoBinder.rebind();
	m_programCube->bind();
	glCullFace(GL_FRONT);
	m_FBO_backFaces->bind();
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_SHORT, 0);
	m_FBO_backFaces->release();
	m_programCube->release();
	vaoBinder.release();

	vaoBinder2.rebind();
	m_programVolume->bind();
	glEnable(GL_TEXTURE_2D);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, m_FBO_backFaces->texture());
	m_programVolume->setUniformValue(m_programVolume->uniformLocation("backFaces"), 1);
	//glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);

	// 3. render the volume
	glEnable(GL_TEXTURE_3D);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_3D, m_VolumeTexture->textureId());
	m_programVolume->setUniformValue(m_programVolume->uniformLocation("volume"), 2);
	//glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);

	// 4. alpha-compositing
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, m_TransferFunction->textureId());
	m_programVolume->setUniformValue(m_programVolume->uniformLocation("transferFunction"), 3);
	//glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);
	m_programVolume->release();
	vaoBinder2.release();

}
void GLWidget::loadTransferFunction()
{
	QImage transfer;
	if (!transfer.load("transfer.bmp"))
	{
		qDebug() << "could not load transfer texture.";
	}
	QImage func = QGLWidget::convertToGLFormat(transfer);
	
	m_TransferFunction = new QOpenGLTexture(func);
	m_TransferFunction->setMinMagFilters(QOpenGLTexture::Nearest, QOpenGLTexture::Nearest);
}
void GLWidget::initializeGL()
{
	connect(context(), &QOpenGLContext::aboutToBeDestroyed, this, &GLWidget::cleanup);

	m_logger = new QOpenGLDebugLogger(this);
	m_logger->initialize();
	connect(m_logger, &QOpenGLDebugLogger::messageLogged, this, &GLWidget::handleLoggedMessage, Qt::DirectConnection);

	QWidget::setFocusPolicy(Qt::FocusPolicy::ClickFocus);

	m_VolumeTexture = new QOpenGLTexture(QOpenGLTexture::Target3D);
	m_VolumeTexture->setMinMagFilters(QOpenGLTexture::Nearest, QOpenGLTexture::Nearest);
	m_VolumeTexture->setFormat(QOpenGLTexture::RGB32F);

	initializeOpenGLFunctions();
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_FRONT);


	// create programs
	m_programCube = new QOpenGLShaderProgram();
	m_programVolume = new QOpenGLShaderProgram();

	loadShaders();
	loadTransferFunction();

	// create Frame Buffer Object (FBO)
	m_FBO_frontFaces = new QOpenGLFramebufferObject(width(), height());
	m_FBO_backFaces = new QOpenGLFramebufferObject(width(), height());

	if (!m_vaoCube.create()) {
		qDebug() << "error creating vao";
	}

	// bind vertex array object (VAO)
	// the VAO encapsulates all the data that is associated with the vertex processor
	// see: http://ogldev.atspace.co.uk/www/tutorial32/tutorial32.html
	QOpenGLVertexArrayObject::Binder vaoBinder(&m_vaoCube);

	// Setup our vertex buffer object
	m_vertexBuffer.create();
	m_vertexBuffer.bind();
	// Setup our index buffer object
	m_indexBuffer.create();
	m_indexBuffer.bind();

	// allocate vertices and indices of a cube
	m_vertexBuffer.allocate(cube_vertices, 8 * 3 * sizeof(GLfloat));
	m_indexBuffer.allocate(cube_elements, 12 * 3 * sizeof(GLushort));

	m_programCube->enableAttributeArray(0);
	m_programCube->enableAttributeArray(1);

	m_programCube->setAttributeBuffer(
		0,			// Specifies the index of the generic vertex attribute to be modified
		GL_FLOAT,	// Specifies the data type of each component in the array
		0,			// Specifies the byte offset between consecutive generic vertex attributes
		3			// Specifies the number of components per generic vertex attribute
	);


	// Setup our tex coord buffer object
	m_texCoordBuffer.create();
	m_texCoordBuffer.bind();

	m_texCoordBuffer.allocate(tex_coordinates, 8 * 3 * sizeof(GLfloat));

	m_programCube->setAttributeBuffer(
		1,			// Specifies the index of the generic vertex attribute to be modified
		GL_FLOAT,	// Specifies the data type of each component in the array
		0,			// Specifies the byte offset between consecutive generic vertex attributes
		3			// Specifies the number of components per generic vertex attribute
	);

	if (!m_vaoQuad.create()) {
		qDebug() << "error creating vao";
	}

	QOpenGLVertexArrayObject::Binder vaoBinder2(&m_vaoQuad);

	m_vertexBufferQuad.create();
	m_vertexBufferQuad.bind();

	// Setup our index buffer object
	m_indexBufferQuad.create();
	m_indexBufferQuad.bind();

	m_vertexBufferQuad.allocate(quad_vertices, 4 * 3 * sizeof(GLfloat));
	m_indexBufferQuad.allocate(quad_elements, 2 * 3 * sizeof(GLushort));

	m_programVolume->setAttributeBuffer(
		0,			// Specifies the index of the generic vertex attribute to be modified
		GL_FLOAT,	// Specifies the data type of each component in the array
		0,			// Specifies the byte offset between consecutive generic vertex attributes
		3			// Specifies the number of components per generic vertex attribute
	);

	m_programVolume->enableAttributeArray(0);

	GLint total_mem_kb = 0;
	glGetIntegerv(GL_GPU_MEM_INFO_TOTAL_AVAILABLE_MEM_NVX,
		&total_mem_kb);

	GLint cur_avail_mem_kb = 0;
	glGetIntegerv(GL_GPU_MEM_INFO_CURRENT_AVAILABLE_MEM_NVX,
		&cur_avail_mem_kb);

	float cur_avail_mem_mb = float(cur_avail_mem_kb) / 1024.0f;
	float total_mem_mb = float(total_mem_kb) / 1024.0f;

	m_MainWindow->displayTotalGPUMemory(total_mem_mb);
	m_MainWindow->displayUsedGPUMemory(total_mem_mb - cur_avail_mem_mb);
}

// this function is called when the volume texture is loaded 
void GLWidget::volumeTextureLoaded(Volume *volume)
{
	makeCurrent();
	if (m_VolumeTexture != NULL) {
		delete m_VolumeTexture;
	}
	m_VolumeTexture = new QOpenGLTexture(QOpenGLTexture::Target3D);
	m_VolumeTexture->setMinMagFilters(QOpenGLTexture::Nearest, QOpenGLTexture::Nearest);
	m_VolumeTexture->setFormat(QOpenGLTexture::RGB32F);

	m_VolumeTexture->setSize(volume->width(), volume->height(), volume->depth());

	m_VolumeTexture->allocateStorage(QOpenGLTexture::Red, QOpenGLTexture::Float32); // works
	m_VolumeTexture->setData(QOpenGLTexture::Red, QOpenGLTexture::Float32, volume->voxels()); // works

	doneCurrent();
}

void GLWidget::cleanup()
{
	makeCurrent();
	delete m_programCube;
	delete m_programVolume;
	doneCurrent();
}

void GLWidget::fileChanged(const QString &path)
{
	// reboot glsw, otherwise it will use the old cached shader
	glswShutdown();
	initglsw();

	loadShaders();
	update();

	// https://stackoverflow.com/questions/18300376/qt-qfilesystemwatcher-signal-filechanged-gets-emited-only-once
	if (QFile::exists(QDir::currentPath() + shaderPath1)) {
		m_fileWatcher->addPath(QDir::currentPath() + shaderPath1);
	}
	if (QFile::exists(QDir::currentPath() + shaderPath2)) {
		m_fileWatcher->addPath(QDir::currentPath() + shaderPath2);

	}
}

void GLWidget::loadShaders()
{
	bool success = false;

	const char *vs = glswGetShader("cube.Vertex");
	if (!vs)
	{
		qDebug() << "Error: not able to read vertex shader";
	}
	QOpenGLShader *vertexShader = new QOpenGLShader(QOpenGLShader::Vertex);
	success = vertexShader->compileSourceCode(vs);

	const char *fs = glswGetShader("cube.Fragment");
	if (!fs)
	{
		qDebug() << "Error: not able to read fragment shader";
	}
	QOpenGLShader *fragmentShader = new QOpenGLShader(QOpenGLShader::Fragment);
	success = fragmentShader->compileSourceCode(fs);

	m_programCube->removeAllShaders();
	m_programCube->addShader(vertexShader);
	m_programCube->addShader(fragmentShader);

	const char *vsVolume = glswGetShader("raycasting.Vertex");
	if (!vsVolume)
	{
		qDebug() << "Error: not able to read vertex shader";
	}
	QOpenGLShader *vertexShaderVolume = new QOpenGLShader(QOpenGLShader::Vertex);
	success = vertexShaderVolume->compileSourceCode(vsVolume);

	const char *fsVolume = glswGetShader("raycasting.Fragment");
	if (!fsVolume)
	{
		qDebug() << "Error: not able to read fragment shader";
	}
	QOpenGLShader *fragmentShaderVolume = new QOpenGLShader(QOpenGLShader::Fragment);
	success = fragmentShaderVolume->compileSourceCode(fsVolume);

	m_programVolume->removeAllShaders();
	m_programVolume->addShader(vertexShaderVolume);
	m_programVolume->addShader(fragmentShaderVolume);
	m_programVolume->link();

}

// this function is called whenever the widget has been resized
void GLWidget::resizeGL(int w, int h)
{
	// when the widget is resized, the aspect ratio of the camera needs to be updated
	m_camera.setAspect(float(w) / h);
}

void GLWidget::mousePressEvent(QMouseEvent *event)
{
	m_lastPos = event->pos();
}

void GLWidget::wheelEvent(QWheelEvent *event)
{
	m_camera.zoom(event->delta() / 100);
	update();
}

void GLWidget::mouseMoveEvent(QMouseEvent *event)
{
	int dx = event->x() - m_lastPos.x();
	int dy = event->y() - m_lastPos.y();

	if (event->buttons() & Qt::LeftButton) {
		m_camera.rotateAzimuth(dx / 100.0f);
		m_camera.rotatePolar(dy / 100.0f);
	}

	if (event->buttons() & Qt::RightButton) {
		m_camera.rotateAzimuth(dx / 100.0f);
		m_camera.rotatePolar(dy / 100.0f);
	}
	m_lastPos = event->pos();
	update();
}

void GLWidget::keyPressEvent(QKeyEvent *event)
{
	switch (event->key())
	{
	case Qt::Key_Space:
	{
		break;
	}
	default:
	{
		event->ignore();
		break;
	}
	}
}

void GLWidget::keyReleaseEvent(QKeyEvent *event)
{

}

void GLWidget::handleLoggedMessage()
{
	const QList<QOpenGLDebugMessage> messages = m_logger->loggedMessages();
	for (const QOpenGLDebugMessage &message : messages)
	{
		qDebug() << message;
	}
}