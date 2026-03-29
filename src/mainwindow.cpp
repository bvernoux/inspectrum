/*
 *  Copyright (C) 2015, Mike Walters <mike@flomp.net>
 *
 *  This file is part of inspectrum.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <QMessageBox>
#include <QMimeData>
#include <QtWidgets>
#include <QPixmapCache>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <sstream>

#include "mainwindow.h"
#include "util.h"

MainWindow::MainWindow()
{
    setWindowTitle(QString("%1 v%2").arg(APP_NAME, APP_VERSION));
    setAcceptDrops(true);

    QPixmapCache::setCacheLimit(40960);

    dock = new SpectrogramControls(tr("Controls"), this);
    dock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    addDockWidget(Qt::LeftDockWidgetArea, dock);

    input = new InputSource();
    input->subscribe(this);

    plots = new PlotView(input);
    setCentralWidget(plots);

    // Connect dock inputs
    connect(dock, &SpectrogramControls::openFile, this, &MainWindow::openFile);
    connect(dock->sampleRate, static_cast<void (QLineEdit::*)(const QString&)>(&QLineEdit::textChanged), this, static_cast<void (MainWindow::*)(QString)>(&MainWindow::setSampleRate));
    connect(dock, static_cast<void (SpectrogramControls::*)(int, int)>(&SpectrogramControls::fftOrZoomChanged), plots, &PlotView::setFFTAndZoom);
    connect(dock->powerMaxSlider, &QSlider::valueChanged, plots, &PlotView::setPowerMax);
    connect(dock->powerMinSlider, &QSlider::valueChanged, plots, &PlotView::setPowerMin);
    connect(dock->zeroPadSlider, &QSlider::valueChanged, plots, &PlotView::setZeroPad);
    connect(dock->zoomYSlider, &QSlider::valueChanged, plots, &PlotView::setZoomY);
    connect(dock->maskOutOfBandCheckBox, &QCheckBox::toggled, plots, &PlotView::setCropToTuner);
    connect(dock, &SpectrogramControls::tunerVisibleChanged, plots, &PlotView::setTunerVisible);
    connect(dock->cursorsCheckBox, &QCheckBox::stateChanged, plots, &PlotView::enableCursors);
    connect(dock->cursorGridSlider, &QSlider::valueChanged, plots, &PlotView::setCursorGridOpacity);
    connect(dock->scalesCheckBox, &QCheckBox::stateChanged, plots, &PlotView::enableScales);
    connect(dock->annosCheckBox, &QCheckBox::stateChanged, plots, &PlotView::enableAnnotations);
    connect(dock->annosCheckBox, &QCheckBox::stateChanged, dock, &SpectrogramControls::enableAnnotations);
    connect(dock->commentsCheckBox, &QCheckBox::stateChanged, plots, &PlotView::enableAnnotationCommentsTooltips);
    connect(dock->cursorSymbolsSpinBox, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), plots, &PlotView::setCursorSegments);
    connect(dock, &SpectrogramControls::symbolRateChanged, plots, &PlotView::setSymbolRate);
    connect(dock, &SpectrogramControls::periodChanged, plots, &PlotView::setPeriod);
    connect(dock, &SpectrogramControls::offsetChanged, plots, &PlotView::setOffset);
    connect(dock, &SpectrogramControls::saveSession, this, &MainWindow::saveSession);
    connect(dock, &SpectrogramControls::loadSessionFile, this, &MainWindow::loadSessionFile);
    connect(dock, &SpectrogramControls::autoDetectRate, this, &MainWindow::autoDetectRate);
    connect(dock, &SpectrogramControls::lsbFirstChanged, this, &MainWindow::setLsbFirst);

    connect(dock, &SpectrogramControls::avgChanged, plots, &PlotView::setAveraging);

    // Connect dock outputs
    connect(plots, &PlotView::timeSelectionChanged, dock, &SpectrogramControls::timeSelectionChanged);
    connect(plots, &PlotView::segmentsChanged, dock->cursorSymbolsSpinBox, &QSpinBox::setValue);
    connect(plots, &PlotView::zoomIn, dock, &SpectrogramControls::zoomIn);
    connect(plots, &PlotView::zoomOut, dock, &SpectrogramControls::zoomOut);
    connect(plots, &PlotView::tunerChanged, dock, &SpectrogramControls::tunerInfoChanged);
    connect(plots, &PlotView::renderTimeChanged, dock, &SpectrogramControls::renderTimeChanged);
    connect(plots, &PlotView::viewPositionChanged, dock, &SpectrogramControls::viewPositionChanged);
    connect(dock, &SpectrogramControls::bookmarkSelected, plots, &PlotView::jumpToBookmark);
    connect(dock, &SpectrogramControls::viewPosXEdited, plots, &PlotView::jumpToTime);
    connect(dock, &SpectrogramControls::viewPosYEdited, plots, &PlotView::jumpToFreq);
    connect(dock, &SpectrogramControls::tunerCentreEdited, plots, &PlotView::setTunerCentreHz);
    connect(dock, &SpectrogramControls::tunerBandwidthEdited, plots, &PlotView::setTunerBandwidthHz);

    // Set defaults after making connections so everything is in sync
    dock->setDefaults();

}

void MainWindow::openFile(QString fileName)
{
    this->setWindowTitle(QString("%1 v%2 - %3").arg(
        APP_NAME, APP_VERSION, QFileInfo(fileName).fileName()));

    // Try to parse osmocom_fft filenames and extract the sample rate and center frequency.
    // Example file name: "name-f2.411200e+09-s5.000000e+06-t20160807180210.cfile"
    QRegularExpression rx(QRegularExpression::anchoredPattern("(.*)-f(.*)-s(.*)-.*\\.cfile"));
    QString basename = QFileInfo(fileName).fileName();

    auto match = rx.match(basename);
    if (match.hasMatch()) {
        QString centerfreq = match.captured(2);
        QString samplerate = match.captured(3);

        std::stringstream ss(samplerate.toUtf8().constData());

        // Needs to be a double as the number is in scientific format
        double rate;
        ss >> rate;
        if (!ss.fail()) {
            setSampleRate(rate);
        }
    }

    try
    {
        input->openFile(fileName.toUtf8().constData());

        plots->resetCursorState();
        dock->cursorsCheckBox->setChecked(false);
        dock->cursorsCheckBox->setChecked(true);
    }
    catch (const std::exception &ex)
    {
        QMessageBox msgBox(QMessageBox::Critical, "Inspectrum openFile error", QString("%1: %2").arg(fileName).arg(ex.what()));
        msgBox.exec();
    }
}

void MainWindow::invalidateEvent()
{
    plots->setSampleRate(input->rate());

    double currentValue = 0;
    parseSIValue(dock->sampleRate->text().toStdString(), currentValue);
    if(QString::number(input->rate()) != QString::number(currentValue)) {
        setSampleRate(input->rate());
    }
}

void MainWindow::setSampleRate(QString rate)
{
    double sampleRate;
    if (!parseSIValue(rate.toStdString(), sampleRate))
        return;
    input->setSampleRate(sampleRate);
    plots->setSampleRate(sampleRate);

    QSettings settings;
    settings.setValue("SampleRate", sampleRate);
}

void MainWindow::setSampleRate(double rate)
{
    dock->sampleRate->setText(QString::fromStdString(
        formatSIValueSigned(rate, "Hz")));
}

void MainWindow::setFormat(QString fmt)
{
    input->setFormat(fmt.toUtf8().constData());
}

void MainWindow::saveSession()
{
    /* default to signal file's directory and base name,
     * fall back to exe directory */
    QString defaultPath = QCoreApplication::applicationDirPath();
    QString signalName = input->getFileName();
    if (!signalName.isEmpty()) {
        QFileInfo fi(signalName);
        defaultPath = fi.absolutePath() + "/" + fi.completeBaseName()
            + ".isession";
    }

    QString fileName = QFileDialog::getSaveFileName(this,
        "Save Session", defaultPath, "Session (*.isession)");
    if (fileName.isEmpty())
        return;

    QJsonObject session;
    session["version"] = APP_VERSION;

    /* signal file paths (absolute + relative for portability) */
    QString signalFile = input->getFileName();
    session["file"] = signalFile;
    QFileInfo sessionInfo(fileName);
    QFileInfo signalInfo(signalFile);
    session["fileRelative"] = sessionInfo.dir().relativeFilePath(
        signalInfo.absoluteFilePath());
    double sr = 0;
    parseSIValue(dock->sampleRate->text().toStdString(), sr);
    session["sampleRate"] = sr;

    /* spectrogram settings */
    QJsonObject spectrogram;
    spectrogram["fftSize"] = dock->fftSizeSlider->value();
    spectrogram["zoomLevel"] = dock->zoomLevelSlider->value();
    spectrogram["zeroPad"] = dock->zeroPadSlider->value();
    spectrogram["zoomY"] = dock->zoomYSlider->value();
    spectrogram["averaging"] = dock->avgSlider->value();
    spectrogram["powerMax"] = dock->powerMaxSlider->value();
    spectrogram["powerMin"] = dock->powerMinSlider->value();
    spectrogram["scales"] = dock->scalesCheckBox->isChecked();
    session["spectrogram"] = spectrogram;

    /* tuner */
    QJsonObject tuner;
    tuner["centre"] = plots->getTunerCentre();
    tuner["deviation"] = plots->getTunerDeviation();
    tuner["crop"] = dock->maskOutOfBandCheckBox->isChecked();
    tuner["visible"] = dock->tunerVisibleCheckBox->isChecked();
    session["tuner"] = tuner;

    /* cursors */
    QJsonObject cursors;
    cursors["enabled"] = dock->cursorsCheckBox->isChecked();
    cursors["gridOpacity"] = dock->cursorGridSlider->value();
    cursors["segments"] = dock->cursorSymbolsSpinBox->value();
    auto sel = plots->getSelectedSamples();
    cursors["sampleMin"] = (qint64)sel.minimum;
    cursors["sampleMax"] = (qint64)sel.maximum;
    session["cursors"] = cursors;

    /* view state */
    QJsonObject view;
    view["scrollX"] = plots->horizontalScrollBar()->value();
    view["scrollY"] = plots->verticalScrollBar()->value();
    view["lsbFirst"] = dock->lsbFirstCheckBox->isChecked();
    view["bookmarks"] = dock->getBookmarksJson();
    session["view"] = view;

    /* SigMF */
    QJsonObject sigmf;
    sigmf["annotations"] = dock->annosCheckBox->isChecked();
    sigmf["comments"] = dock->commentsCheckBox->isChecked();
    session["sigmf"] = sigmf;

    /* derived plots */
    session["plots"] = plots->getDerivedPlotsState();

    /* window geometry and dock layout */
    QJsonObject window;
    window["geometry"] = QString::fromLatin1(saveGeometry().toBase64());
    window["state"] = QString::fromLatin1(saveState().toBase64());
    session["window"] = window;

    QJsonDocument doc(session);
    QFile file(fileName);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(doc.toJson());
        file.close();
    }
}

void MainWindow::loadSessionFile(const QString &fileName)
{
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly))
        return;

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();

    QJsonObject session = doc.object();

    /* clear focus so hasFocus() guards don't block updates */
    dock->setFocus();

    dock->maskOutOfBandCheckBox->setChecked(false);
    dock->lsbFirstCheckBox->setChecked(false);
    dock->zeroPadSlider->setValue(0);
    dock->zoomYSlider->setValue(0);
    dock->cursorGridSlider->setValue(80);
    dock->cursorsCheckBox->setChecked(false);
    dock->cursorSymbolsSpinBox->setValue(1);
    dock->scalesCheckBox->setChecked(true);
    dock->annosCheckBox->setChecked(true);
    dock->commentsCheckBox->setChecked(true);
    dock->detectStatusLabel->setText("");
    dock->symbolRateEdit->setText("");
    dock->tunerCentreEdit->setText("");
    dock->tunerBandwidthEdit->setText("");
    dock->avgSlider->setValue(0);
    dock->setBookmarksJson(QJsonArray());
    plots->resetCursorState();

    plots->restoreSessionPlots(QJsonArray());

    QString signalFile;
    QFileInfo sessionFileInfo(fileName);
    QString relPath = session["fileRelative"].toString();

    if (!relPath.isEmpty()) {
        QString resolved = sessionFileInfo.dir().filePath(relPath);
        if (QFileInfo::exists(resolved))
            signalFile = resolved;
    }

    if (signalFile.isEmpty())
        signalFile = session["file"].toString();

    if (!signalFile.isEmpty())
        openFile(signalFile);

    double rate = session["sampleRate"].toDouble();
    if (rate > 0)
        setSampleRate(rate);

    /* spectrogram */
    QJsonObject spec = session["spectrogram"].toObject();
    dock->fftSizeSlider->setValue(spec["fftSize"].toInt(9));
    dock->zoomLevelSlider->setValue(spec["zoomLevel"].toInt(0));
    dock->zeroPadSlider->setValue(spec["zeroPad"].toInt(0));
    dock->zoomYSlider->setValue(spec["zoomY"].toInt(0));
    dock->avgSlider->setValue(spec["averaging"].toInt(0));
    dock->powerMaxSlider->setValue(spec["powerMax"].toInt(0));
    dock->powerMinSlider->setValue(spec["powerMin"].toInt(-100));
    dock->scalesCheckBox->setChecked(spec["scales"].toBool(true));

    /* sigmf */
    QJsonObject sigmf = session["sigmf"].toObject();
    dock->annosCheckBox->setChecked(sigmf["annotations"].toBool(true));
    dock->commentsCheckBox->setChecked(sigmf["comments"].toBool(true));

    /* view */
    QJsonObject view = session["view"].toObject();
    if (view.contains("bookmarks"))
        dock->setBookmarksJson(view["bookmarks"].toArray());
    dock->lsbFirstCheckBox->setChecked(view["lsbFirst"].toBool(false));

    /* cursors */
    QJsonObject cur = session["cursors"].toObject();
    bool curEnabled = cur["enabled"].toBool(false);
    dock->cursorsCheckBox->setChecked(curEnabled);
    dock->cursorGridSlider->setValue(cur["gridOpacity"].toInt(80));
    if (curEnabled) {
        range_t<size_t> sel = {
            (size_t)cur["sampleMin"].toDouble(),
            (size_t)cur["sampleMax"].toDouble()
        };
        plots->setSelectedSamples(sel);
    }
    int segments = cur["segments"].toInt(1);
    dock->cursorSymbolsSpinBox->blockSignals(true);
    dock->cursorSymbolsSpinBox->setValue(segments);
    dock->cursorSymbolsSpinBox->blockSignals(false);
    plots->setSegmentsOnly(segments);

    /* tuner */
    QJsonObject tuner = session["tuner"].toObject();
    plots->setTunerPosition(
        tuner["centre"].toInt(100),
        tuner["deviation"].toInt(10));

    /* derived plots */
    QJsonArray plotsArr = session["plots"].toArray();
    if (!plotsArr.isEmpty())
        plots->restoreSessionPlots(plotsArr);

    plots->setLsbFirst(dock->lsbFirstCheckBox->isChecked());
    plots->refreshThresholdPlots();

    /* window geometry */
    QJsonObject win = session["window"].toObject();
    if (win.contains("geometry")) {
        QByteArray geom = QByteArray::fromBase64(
            win["geometry"].toString().toLatin1());
        restoreGeometry(geom);
    }
    if (win.contains("state")) {
        QByteArray st = QByteArray::fromBase64(
            win["state"].toString().toLatin1());
        restoreState(st);
    }

    /* scroll positions */
    plots->setScrollPosition(view["scrollX"].toInt(0),
                             view["scrollY"].toInt(0));

    /* crop + visibility (last) */
    dock->maskOutOfBandCheckBox->setChecked(tuner["crop"].toBool(false));
    dock->tunerVisibleCheckBox->setChecked(tuner["visible"].toBool(true));
}

void MainWindow::autoDetectRate()
{
    DetectResult result = plots->autoDetectSymbolRate(DemodAmplitude);

    dock->detectStatusLabel->setText(result.status);

    if (result.rate > 0) {
        dock->symbolRateEdit->setText(
            QString::number(result.rate, 'f', 3) + "Bd");
        plots->setSymbolRate(result.rate);
    }
}

void MainWindow::setLsbFirst(bool lsb)
{
    plots->setLsbFirst(lsb);
}


void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls() && event->mimeData()->urls().size() == 1)
        event->acceptProposedAction();
}

void MainWindow::dropEvent(QDropEvent *event)
{
    auto urls = event->mimeData()->urls();

    if (urls.isEmpty())
        return;

    QString filePath = urls.first().toLocalFile();

    if (filePath.isEmpty())
        return;

    if (filePath.endsWith(".isession", Qt::CaseInsensitive))
        loadSessionFile(filePath);
    else
        openFile(filePath);
}
