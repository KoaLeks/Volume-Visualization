/*
* Copyright (C) 2019
* Institute of Visual Computing and Human-Centered Technology, TU Wien
* Written by Tobias Klein <tklein@cg.tuwien.ac.at>
* All rights reserved.
*/

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "ui_MainWindow.h"
#include "Volume.h"
#include "GLWidget.h"

#include <QMainWindow>
#include <QPushButton>
#include <QLabel>
#include <QProgressBar>
#include <QStatusBar>
#include <QVariant>


class MainWindow : public QMainWindow
{
	Q_OBJECT

public:

	MainWindow(QWidget *parent = 0);
	~MainWindow();

	void displayTotalGPUMemory(float size);
	void displayUsedGPUMemory(float size);

	
protected slots :

	void openFileAction();
	void closeAction();
	void fieldOfViewChanged(int value);
	void perspectiveChanged(int value);
	void renderingModeChanged(int value);
	void nearPlaneChanged(double value);
	void farPlaneChanged(double value);
		

private:

	// USER INTERFACE ELEMENTS

	Ui_MainWindow *m_Ui;

	// DATA 
	enum DataType
	{
		VOLUME = 0,
		VECTORFIELD	 = 1,
		MULTIVARIATE = 2
	};

	struct FileType
	{
		QString			filename;
		DataType		type;
	} m_FileType;

	Volume *m_Volume;

	GLWidget *m_glWidget;
};

#endif
