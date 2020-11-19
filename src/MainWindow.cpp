#include "MainWindow.h"

#include <QFileDialog>

#include <QPainter>


MainWindow::MainWindow(QWidget *parent)
	: QMainWindow(parent), m_Volume(0)
{
	m_Ui = new Ui_MainWindow();
	m_Ui->setupUi(this);

	QLayout *layout = m_Ui->controls->layout();
	layout->setAlignment(Qt::AlignTop);
	m_Ui->controls->setLayout(layout);

	m_glWidget = new GLWidget(this, this);
	m_Ui->glLayout->addWidget(m_glWidget);

	// signals and slots are used for communication between objects
	// the signals and slots mechanism is a central feature of Qt
	// see here: http://doc.qt.io/qt-4.8/signalsandslots.html
	connect(m_Ui->actionOpen, SIGNAL(triggered()), this, SLOT(openFileAction()));
	connect(m_Ui->actionClose, SIGNAL(triggered()), this, SLOT(closeAction()));
	connect(m_Ui->slider_FieldOfView, SIGNAL(sliderMoved(int)), this, SLOT(fieldOfViewChanged(int)));
	connect(m_Ui->box_nearPlane, SIGNAL(valueChanged(double)), this, SLOT(nearPlaneChanged(double)));
	connect(m_Ui->box_FarPlane, SIGNAL(valueChanged(double)), this, SLOT(farPlaneChanged(double)));
	connect(m_Ui->switch_projection, SIGNAL(currentIndexChanged(int)), this, SLOT(perspectiveChanged(int)));	
	connect(m_Ui->switch_renderingMode, SIGNAL(currentIndexChanged(int)), this, SLOT(renderingModeChanged(int)));
	
	m_Ui->usedMemLCD->setPalette(Qt::darkGreen);
	m_Ui->memSizeLCD->setPalette(Qt::darkGreen);
}

MainWindow::~MainWindow()
{
}


//-------------------------------------------------------------------------------------------------
// Slots
//-------------------------------------------------------------------------------------------------

void MainWindow::openFileAction()
{
	QString filename = QFileDialog::getOpenFileName(this, "Data File", 0, tr("Data Files (*.dat *.gri *.csv)"));

	if (!filename.isEmpty())
	{
		// store filename
		m_FileType.filename = filename;
		std::string fn = filename.toStdString();
		bool success = false;

		// progress bar and top label
		m_Ui->progressBar->setEnabled(true);
		m_Ui->labelTop->setText("Loading data ...");

		// load data according to file extension
		if (fn.substr(fn.find_last_of(".") + 1) == "dat")		// LOAD VOLUME
		{
			// create VOLUME
			m_FileType.type = VOLUME;
			m_Volume = new Volume();

			// load file
			success = m_Volume->loadFromFile(filename, m_Ui->progressBar);
		}

		m_Ui->progressBar->setEnabled(false);

		// status message
		if (success)
		{
			QString type;
			if (m_FileType.type == VOLUME) type = "VOLUME";
			else if (m_FileType.type == VECTORFIELD) type = "VECTORFIELD";
			else if (m_FileType.type == MULTIVARIATE) type = "MULTIVARIATE";
			m_Ui->labelTop->setText("File LOADED [" + filename + "] - Type [" + type + "]");
			m_glWidget->volumeTextureLoaded(m_Volume);
		}
		else
		{
			m_Ui->labelTop->setText("ERROR loading file " + filename + "!");
			m_Ui->progressBar->setValue(0);
		}
	}
}

void MainWindow::closeAction()
{
	close();
}

void MainWindow::displayTotalGPUMemory(float size)
{
	m_Ui->memSizeLCD->display(size);
}
void MainWindow::displayUsedGPUMemory(float size)
{
	m_Ui->usedMemLCD->display(size);
}

void MainWindow::fieldOfViewChanged(int value)
{
	m_glWidget->m_camera.mFieldOfView = value;
	m_glWidget->m_camera.buildProjectionMatrix();
	m_glWidget->update();
}

void MainWindow::perspectiveChanged(int value)
{
	m_glWidget->m_camera.setOrthogonal(value == 1);
	m_glWidget->update();
	m_Ui->slider_FieldOfView->setDisabled(value == 1);
}

void MainWindow::renderingModeChanged(int value)
{
	m_glWidget->m_renderingMode = value;
	m_glWidget->update();
}

void MainWindow::nearPlaneChanged(double value)
{
	m_glWidget->m_camera.setNearPlane(value);
	m_glWidget->update();
}

void MainWindow::farPlaneChanged(double value)
{
	m_glWidget->m_camera.setFarPlane(value);
	m_glWidget->update();
}

