/*
* Copyright (C) 2019
* Institute of Visual Computing and Human-Centered Technology, TU Wien
* Written by Tobias Klein <tklein@cg.tuwien.ac.at>
* All rights reserved.
*/

#pragma once

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLBuffer>
#include <QGLShader>
#include <QOpenGLShaderProgram>
#include <QFileSystemWatcher>
#include <QOpenGLDebugLogger>
#include <QOpenGLTexture>

#include "Camera.h"
#include "Volume.h"

class MainWindow;

class GLWidget : public QOpenGLWidget, protected QOpenGLFunctions
{
	Q_OBJECT

public:
	GLWidget(QWidget *parent, MainWindow *mainWindow);
	~GLWidget();

	Camera m_camera;
	int m_renderingMode;

public slots:
	void cleanup();
	void volumeTextureLoaded(Volume *volume);

signals:

protected:

	void mousePressEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
	void mouseMoveEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
	void keyPressEvent(QKeyEvent *event) Q_DECL_OVERRIDE;
	void keyReleaseEvent(QKeyEvent *event) Q_DECL_OVERRIDE;
	void wheelEvent(QWheelEvent *event) Q_DECL_OVERRIDE;

protected slots:

	void paintGL() Q_DECL_OVERRIDE;
	void initializeGL() Q_DECL_OVERRIDE;
	void resizeGL(int w, int h) Q_DECL_OVERRIDE;
	void fileChanged(const QString &path);
	void handleLoggedMessage();

private:

	void initglsw();
	void loadShaders();
	void loadTransferFunction();

	QOpenGLShaderProgram *m_programCube;
	QOpenGLShaderProgram *m_programVolume;
	
	QOpenGLShader *m_vertexShaderVolume;
	QOpenGLShader *m_fragmentShaderVolume;

	QOpenGLVertexArrayObject m_vaoCube;
	
	QOpenGLVertexArrayObject m_vaoQuad;
	
	QOpenGLBuffer m_vertexBuffer;
	QOpenGLBuffer m_indexBuffer;
	QOpenGLBuffer m_texCoordBuffer;

	QOpenGLBuffer m_vertexBufferQuad;
	QOpenGLBuffer m_indexBufferQuad;

	QOpenGLTexture *m_VolumeTexture;
	QOpenGLTexture *m_TransferFunction;

	QOpenGLDebugLogger *m_logger;
	
	QOpenGLFramebufferObject *m_FBO_frontFaces;
	QOpenGLFramebufferObject *m_FBO_backFaces;

	QFileSystemWatcher *m_fileWatcher;

	QPoint m_lastPos;

	MainWindow *m_MainWindow;
};


