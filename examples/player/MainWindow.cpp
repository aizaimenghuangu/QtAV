/******************************************************************************
    Simple Player:  this file is part of QtAV examples
    Copyright (C) 2012-2014 Wang Bin <wbsecg1@gmail.com>

*   This file is part of QtAV

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/
#include "MainWindow.h"
#include "EventFilter.h"
#include <QtCore/QTimer>
#include <QTimeEdit>
#include <QLabel>
#include <QApplication>
#include <QDesktopWidget>
#include <QtCore/QFileInfo>
#include <QtCore/QTextStream>
#include <QGraphicsOpacityEffect>
#include <QResizeEvent>
#include <QWindowStateChangeEvent>
#include <QWidgetAction>
#include <QLayout>
#include <QPushButton>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QInputDialog>
#include <QMenu>
#include <QMessageBox>
#include <QToolTip>
#include <QKeyEvent>
#include <QWheelEvent>
#include <QtAV/QtAV.h>
#include "Button.h"
#include "ClickableMenu.h"
#include "Slider.h"
#include "StatisticsView.h"
#include "TVView.h"
#include "config/DecoderConfigPage.h"
#include "config/Config.h"
#include "config/VideoEQConfigPage.h"
#include "playlist/PlayList.h"
#include "common/ScreenSaver.h"

/*
 *TODO:
 * disable a/v actions if player is 0;
 * use action's value to set player's parameters when start to play a new file
 */

#define SLIDER_ON_VO 0

#define AVDEBUG() \
    qDebug("%s %s @%d", __FILE__, __FUNCTION__, __LINE__);

using namespace QtAV;
const qreal kVolumeInterval = 0.05;

static void QLabelSetElideText(QLabel *label, QString text, int W = 0)
{
    QFontMetrics metrix(label->font());
    int width = label->width() - label->indent() - label->margin();
    if (W || label->parent()) {
        int w = W;
        if (!w)
            w = ((QWidget*)label->parent())->width();
        width = qMax(w - label->indent() - label->margin() - 13*(30), 0); //TODO: why 30?
    }
    QString clippedText = metrix.elidedText(text, Qt::ElideRight, width);
    label->setText(clippedText);
}

MainWindow::MainWindow(QWidget *parent) :
    QWidget(parent)
  , mIsReady(false)
  , mHasPendingPlay(false)
  , mNullAO(false)
  , mControlOn(false)
  , mShowControl(2)
  , mRepeateMax(0)
  , mpPlayer(0)
  , mpRenderer(0)
  , mpTempRenderer(0)
{
    mpAudioTrackAction = 0;
    setMouseTracking(true); //mouseMoveEvent without press.
    connect(this, SIGNAL(ready()), SLOT(processPendingActions()));
    //QTimer::singleShot(10, this, SLOT(setupUi()));
    setupUi();
    //setToolTip(tr("Click black area to use shortcut(see right click menu)"));
}

MainWindow::~MainWindow()
{
    mpHistory->save();
    mpPlayList->save();
    if (mpVolumeSlider && !mpVolumeSlider->parentWidget()) {
        mpVolumeSlider->close();
        delete mpVolumeSlider;
        mpVolumeSlider = 0;
    }
}

void MainWindow::initPlayer()
{
    mpPlayer = new AVPlayer(this);
    mIsReady = true;
    //mpPlayer->setAudioOutput(AudioOutputFactory::create(AudioOutputId_PortAudio));
    qApp->installEventFilter(new EventFilter(mpPlayer));
    //QHash<QByteArray, QByteArray> dict;
    //dict.insert("rtsp_transport", "tcp");
    //mpPlayer->setOptionsForFormat(dict);
    qDebug("player created");
    mpCaptureBtn->setToolTip(tr("Capture video frame") + "\n" + tr("Save to") + ": " + mpPlayer->videoCapture()->captureDir());
    connect(mpStopBtn, SIGNAL(clicked()), mpPlayer, SLOT(stop()));
    connect(mpForwardBtn, SIGNAL(clicked()), mpPlayer, SLOT(seekForward()));
    connect(mpBackwardBtn, SIGNAL(clicked()), mpPlayer, SLOT(seekBackward()));
    connect(mpVolumeBtn, SIGNAL(clicked()), SLOT(showHideVolumeBar()));
    connect(mpVolumeSlider, SIGNAL(sliderPressed()), SLOT(setVolume()));
    connect(mpVolumeSlider, SIGNAL(valueChanged(int)), SLOT(setVolume()));

    connect(mpPlayer, SIGNAL(error(QtAV::AVError)), this, SLOT(handleError(QtAV::AVError)));
    connect(mpPlayer, SIGNAL(started()), this, SLOT(onStartPlay()));
    connect(mpPlayer, SIGNAL(stopped()), this, SLOT(onStopPlay()));
    connect(mpPlayer, SIGNAL(paused(bool)), this, SLOT(onPaused(bool)));
    connect(mpPlayer, SIGNAL(speedChanged(qreal)), this, SLOT(onSpeedChange(qreal)));
    connect(mpPlayer, SIGNAL(positionChanged(qint64)), this, SLOT(onPositionChange(qint64)));
    connect(mpVideoEQ, SIGNAL(brightnessChanged(int)), mpPlayer, SLOT(setBrightness(int)));
    connect(mpVideoEQ, SIGNAL(contrastChanged(int)), mpPlayer, SLOT(setContrast(int)));
    connect(mpVideoEQ, SIGNAL(saturationChanged(int)), mpPlayer, SLOT(setSaturation(int)));

    emit ready(); //emit this signal after connection. otherwise the slots may not be called for the first time
}

void MainWindow::setupUi()
{
    QVBoxLayout *mainLayout = new QVBoxLayout();
    mainLayout->setSpacing(0);
    mainLayout->setMargin(0);
    setLayout(mainLayout);

    mpPlayerLayout = new QVBoxLayout();
    mpControl = new QWidget(this);
    mpControl->setMaximumHeight(25);

    //mpPreview = new QLable(this);

    mpTimeSlider = new Slider(mpControl);
    mpTimeSlider->setDisabled(true);
    //mpTimeSlider->setFixedHeight(8);
    mpTimeSlider->setMaximumHeight(8);
    mpTimeSlider->setTracking(true);
    mpTimeSlider->setOrientation(Qt::Horizontal);
    mpTimeSlider->setMinimum(0);
#if SLIDER_ON_VO
    QGraphicsOpacityEffect *oe = new QGraphicsOpacityEffect(this);
    oe->setOpacity(0.5);
    mpTimeSlider->setGraphicsEffect(oe);
#endif //SLIDER_ON_VO

    mpCurrent = new QLabel(mpControl);
    mpCurrent->setToolTip(tr("Current time"));
    mpCurrent->setMargin(2);
    mpCurrent->setText("00:00:00");
    mpDuration = new QLabel(mpControl);
    mpDuration->setToolTip(tr("Duration"));
    mpDuration->setMargin(2);
    mpDuration->setText("00:00:00");
    mpTitle = new QLabel(mpControl);
    mpTitle->setToolTip(tr("Render engine"));
    mpTitle->setText("QPainter");
    mpTitle->setIndent(8);
    mpSpeed = new QLabel("1.00");
    mpSpeed->setMargin(1);
    mpSpeed->setToolTip(tr("Speed. Ctrl+Up/Down"));

    mPlayPixmap = QPixmap(":/theme/button-play-pause.png");
    int w = mPlayPixmap.width(), h = mPlayPixmap.height();
    mPausePixmap = mPlayPixmap.copy(QRect(w/2, 0, w/2, h));
    mPlayPixmap = mPlayPixmap.copy(QRect(0, 0, w/2, h));
    qDebug("%d x %d", mPlayPixmap.width(), mPlayPixmap.height());
    mpPlayPauseBtn = new Button(mpControl);
    int a = qMin(w/2, h);
    const int kMaxButtonIconWidth = 20;
    const int kMaxButtonIconMargin = kMaxButtonIconWidth/3;
    a = qMin(a, kMaxButtonIconWidth);
    mpPlayPauseBtn->setIconWithSates(mPlayPixmap);
    mpPlayPauseBtn->setIconSize(QSize(a, a));
    mpPlayPauseBtn->setMaximumSize(a+kMaxButtonIconMargin+2, a+kMaxButtonIconMargin);
    mpStopBtn = new Button(mpControl);
    mpStopBtn->setIconWithSates(QPixmap(":/theme/button-stop.png"));
    mpStopBtn->setIconSize(QSize(a, a));
    mpStopBtn->setMaximumSize(a+kMaxButtonIconMargin+2, a+kMaxButtonIconMargin);
    mpBackwardBtn = new Button(mpControl);
    mpBackwardBtn->setIconWithSates(QPixmap(":/theme/button-rewind.png"));
    mpBackwardBtn->setIconSize(QSize(a, a));
    mpBackwardBtn->setMaximumSize(a+kMaxButtonIconMargin+2, a+kMaxButtonIconMargin);
    mpForwardBtn = new Button(mpControl);
    mpForwardBtn->setIconWithSates(QPixmap(":/theme/button-fastforward.png"));
    mpForwardBtn->setIconSize(QSize(a, a));
    mpForwardBtn->setMaximumSize(a+kMaxButtonIconMargin+2, a+kMaxButtonIconMargin);
    mpOpenBtn = new Button(mpControl);
    mpOpenBtn->setToolTip(tr("Open"));
    mpOpenBtn->setIconWithSates(QPixmap(":/theme/open_folder.png"));
    mpOpenBtn->setIconSize(QSize(a, a));
    mpOpenBtn->setMaximumSize(a+kMaxButtonIconMargin+2, a+kMaxButtonIconMargin);

    mpInfoBtn = new Button();
    mpInfoBtn->setToolTip(QString("Media information. Not implemented."));
    mpInfoBtn->setIconWithSates(QPixmap(":/theme/info.png"));
    mpInfoBtn->setIconSize(QSize(a, a));
    mpInfoBtn->setMaximumSize(a+kMaxButtonIconMargin+2, a+kMaxButtonIconMargin);
    mpCaptureBtn = new Button();
    mpCaptureBtn->setIconWithSates(QPixmap(":/theme/screenshot.png"));
    mpCaptureBtn->setIconSize(QSize(a, a));
    mpCaptureBtn->setMaximumSize(a+kMaxButtonIconMargin+2, a+kMaxButtonIconMargin);
    mpVolumeBtn = new Button();
    mpVolumeBtn->setIconWithSates(QPixmap(":/theme/button-max-volume.png"));
    mpVolumeBtn->setIconSize(QSize(a, a));
    mpVolumeBtn->setMaximumSize(a+kMaxButtonIconMargin+2, a+kMaxButtonIconMargin);

    mpVolumeSlider = new Slider();
    mpVolumeSlider->hide();
    mpVolumeSlider->setOrientation(Qt::Horizontal);
    mpVolumeSlider->setMinimum(0);
    const int kVolumeSliderMax = 100;
    mpVolumeSlider->setMaximum(kVolumeSliderMax);
    mpVolumeSlider->setMaximumHeight(8);
    mpVolumeSlider->setMaximumWidth(88);
    mpVolumeSlider->setValue(int(1.0/kVolumeInterval*qreal(kVolumeSliderMax)/100.0));
    setVolume();

    mpMenuBtn = new Button();
    mpMenuBtn->setAutoRaise(true);
    mpMenuBtn->setPopupMode(QToolButton::InstantPopup);

/*
    mpMenuBtn->setIconWithSates(QPixmap(":/theme/search-arrow.png"));
    mpMenuBtn->setIconSize(QSize(a, a));
    mpMenuBtn->setMaximumSize(a+kMaxButtonIconMargin+2, a+kMaxButtonIconMargin);
*/
    QMenu *subMenu = 0;
    QWidgetAction *pWA = 0;
    mpMenu = new QMenu(mpMenuBtn);
    mpMenu->addAction(tr("Open Url"), this, SLOT(openUrl()));
    mpMenu->addAction(tr("Online channels"), this, SLOT(onTVMenuClick()));
    mpMenu->addSeparator();

    subMenu = new QMenu(tr("Play list"));
    mpMenu->addMenu(subMenu);
    mpPlayList = new PlayList(this);
    mpPlayList->setSaveFile(Config::instance().defaultDir() + "/playlist.qds");
    mpPlayList->load();
    connect(mpPlayList, SIGNAL(aboutToPlay(QString)), SLOT(play(QString)));
    pWA = new QWidgetAction(0);
    pWA->setDefaultWidget(mpPlayList);
    subMenu->addAction(pWA); //must add action after the widget action is ready. is it a Qt bug?

    subMenu = new QMenu(tr("History"));
    mpMenu->addMenu(subMenu);
    mpHistory = new PlayList(this);
    mpHistory->setMaxRows(20);
    mpHistory->setSaveFile(Config::instance().defaultDir() + "/history.qds");
    mpHistory->load();
    connect(mpHistory, SIGNAL(aboutToPlay(QString)), SLOT(play(QString)));
    pWA = new QWidgetAction(0);
    pWA->setDefaultWidget(mpHistory);
    subMenu->addAction(pWA); //must add action after the widget action is ready. is it a Qt bug?

    mpMenu->addSeparator();

    //mpMenu->addAction(tr("Setup"), this, SLOT(setup()))->setEnabled(false);
    //mpMenu->addAction(tr("Report"))->setEnabled(false); //report bug, suggestions etc. using maillist?
    mpMenu->addAction(tr("About"), this, SLOT(about()));
    mpMenu->addAction(tr("Help"), this, SLOT(help()));
    mpMenu->addAction(tr("About Qt"), qApp, SLOT(aboutQt()));
    mpMenu->addSeparator();
    mpMenuBtn->setMenu(mpMenu);
    mpMenu->addSeparator();

    subMenu = new QMenu(tr("Speed"));
    mpMenu->addMenu(subMenu);
    QDoubleSpinBox *pSpeedBox = new QDoubleSpinBox(0);
    pSpeedBox->setRange(0.01, 20);
    pSpeedBox->setValue(1.0);
    pSpeedBox->setSingleStep(0.01);
    pSpeedBox->setCorrectionMode(QAbstractSpinBox::CorrectToPreviousValue);
    pWA = new QWidgetAction(0);
    pWA->setDefaultWidget(pSpeedBox);
    subMenu->addAction(pWA); //must add action after the widget action is ready. is it a Qt bug?

    subMenu = new ClickableMenu(tr("Repeat"));
    mpMenu->addMenu(subMenu);
    //subMenu->setEnabled(false);
    mpRepeatEnableAction = subMenu->addAction(tr("Enable"));
    mpRepeatEnableAction->setCheckable(true);
    connect(mpRepeatEnableAction, SIGNAL(toggled(bool)), SLOT(toggleRepeat(bool)));
    // TODO: move to a func or class
    mpRepeatBox = new QSpinBox(0);
    mpRepeatBox->setMinimum(-1);
    mpRepeatBox->setValue(-1);
    mpRepeatBox->setToolTip("-1: " + tr("infinity"));
    connect(mpRepeatBox, SIGNAL(valueChanged(int)), SLOT(setRepeateMax(int)));
    QLabel *pRepeatLabel = new QLabel(tr("Times"));
    QHBoxLayout *hb = new QHBoxLayout;
    hb->addWidget(pRepeatLabel);
    hb->addWidget(mpRepeatBox);
    QVBoxLayout *vb = new QVBoxLayout;
    vb->addLayout(hb);
    pRepeatLabel = new QLabel(tr("From"));
    mpRepeatA = new QTimeEdit();
    mpRepeatA->setDisplayFormat("HH:mm:ss");
    mpRepeatA->setToolTip(tr("negative value means from the end"));
    connect(mpRepeatA, SIGNAL(timeChanged(QTime)), SLOT(repeatAChanged(QTime)));
    hb = new QHBoxLayout;
    hb->addWidget(pRepeatLabel);
    hb->addWidget(mpRepeatA);
    vb->addLayout(hb);
    pRepeatLabel = new QLabel(tr("To"));
    mpRepeatB = new QTimeEdit();
    mpRepeatB->setDisplayFormat("HH:mm:ss");
    mpRepeatB->setToolTip(tr("negative value means from the end"));
    connect(mpRepeatB, SIGNAL(timeChanged(QTime)), SLOT(repeatBChanged(QTime)));
    hb = new QHBoxLayout;
    hb->addWidget(pRepeatLabel);
    hb->addWidget(mpRepeatB);
    vb->addLayout(hb);
    QWidget *pRepeatWidget = new QWidget;
    pRepeatWidget->setLayout(vb);

    pWA = new QWidgetAction(0);
    pWA->setDefaultWidget(pRepeatWidget);
    pWA->defaultWidget()->setEnabled(false);
    subMenu->addAction(pWA); //must add action after the widget action is ready. is it a Qt bug?
    mpRepeatAction = pWA;

    mpMenu->addSeparator();

    subMenu = new ClickableMenu(tr("Audio track"));
    mpMenu->addMenu(subMenu);
    mpAudioTrackMenu = subMenu;
    connect(mpAudioTrackMenu, SIGNAL(triggered(QAction*)), SLOT(changeAudioTrack(QAction*)));
    subMenu = new ClickableMenu(tr("Channel"));
    mpMenu->addMenu(subMenu);
    connect(subMenu, SIGNAL(triggered(QAction*)), SLOT(changeChannel(QAction*)));
    mpChannelAction = subMenu->addAction(tr("As input"));
    mpChannelAction->setData(AudioFormat::ChannelLayout_Unsupported); //will set to input in resampler if not supported.
    subMenu->addAction(tr("Stero"))->setData(AudioFormat::ChannelLayout_Stero);
    subMenu->addAction(tr("Mono (center)"))->setData(AudioFormat::ChannelLayout_Center);
    subMenu->addAction(tr("Left"))->setData(AudioFormat::ChannelLayout_Left);
    subMenu->addAction(tr("Right"))->setData(AudioFormat::ChannelLayout_Right);
    foreach(QAction* action, subMenu->actions()) {
        action->setCheckable(true);
    }
    mpChannelAction->setChecked(true);

    subMenu = new QMenu(tr("Aspect ratio"), mpMenu);
    mpMenu->addMenu(subMenu);
    connect(subMenu, SIGNAL(triggered(QAction*)), SLOT(switchAspectRatio(QAction*)));
    mpARAction = subMenu->addAction(tr("Video"));
    mpARAction->setData(0);
    subMenu->addAction(tr("Window"))->setData(-1);
    subMenu->addAction("4:3")->setData(4.0/3.0);
    subMenu->addAction("16:9")->setData(16.0/9.0);
    subMenu->addAction(tr("Custom"))->setData(-2);
    foreach(QAction* action, subMenu->actions()) {
        action->setCheckable(true);
    }
    mpARAction->setChecked(true);

    subMenu = new ClickableMenu(tr("Color space"));
    mpMenu->addMenu(subMenu);
    mpVideoEQ = new VideoEQConfigPage();
    pWA = new QWidgetAction(0);
    pWA->setDefaultWidget(mpVideoEQ);
    subMenu->addAction(pWA);

    subMenu = new ClickableMenu(tr("Decoder"));
    mpMenu->addMenu(subMenu);
    mpDecoderConfigPage = new DecoderConfigPage(&Config::instance());
    pWA = new QWidgetAction(0);
    pWA->setDefaultWidget(mpDecoderConfigPage);
    subMenu->addAction(pWA);

    subMenu = new ClickableMenu(tr("Renderer"));
    mpMenu->addMenu(subMenu);
    connect(subMenu, SIGNAL(triggered(QAction*)), SLOT(changeVO(QAction*)));
    //TODO: AVOutput.name,detail(description). check whether it is available
    mpVOAction = subMenu->addAction("QPainter");
    mpVOAction->setData(VideoRendererId_Widget);
    subMenu->addAction("OpenGL")->setData(VideoRendererId_GLWidget);
    subMenu->addAction("GDI+")->setData(VideoRendererId_GDI);
    subMenu->addAction("Direct2D")->setData(VideoRendererId_Direct2D);
    subMenu->addAction("XV")->setData(VideoRendererId_XV);
    mVOActions = subMenu->actions();
    foreach(QAction* action, subMenu->actions()) {
        action->setCheckable(true);
    }
    mpVOAction->setChecked(true);


    mainLayout->addLayout(mpPlayerLayout);
    mainLayout->addWidget(mpTimeSlider);
    mainLayout->addWidget(mpControl);

    QHBoxLayout *controlLayout = new QHBoxLayout();
    controlLayout->setSpacing(0);
    controlLayout->setMargin(1);
    mpControl->setLayout(controlLayout);
    controlLayout->addWidget(mpCurrent);
    controlLayout->addWidget(mpTitle);
    QSpacerItem *space = new QSpacerItem(mpPlayPauseBtn->width(), mpPlayPauseBtn->height(), QSizePolicy::MinimumExpanding);
    controlLayout->addSpacerItem(space);
    controlLayout->addWidget(mpVolumeSlider);
    controlLayout->addWidget(mpVolumeBtn);
    controlLayout->addWidget(mpCaptureBtn);
    controlLayout->addWidget(mpPlayPauseBtn);
    controlLayout->addWidget(mpStopBtn);
    controlLayout->addWidget(mpBackwardBtn);
    controlLayout->addWidget(mpForwardBtn);
    controlLayout->addWidget(mpOpenBtn);
    controlLayout->addWidget(mpInfoBtn);
    controlLayout->addWidget(mpSpeed);
    //controlLayout->addWidget(mpSetupBtn);
    controlLayout->addWidget(mpMenuBtn);
    controlLayout->addWidget(mpDuration);

    connect(pSpeedBox, SIGNAL(valueChanged(double)), SLOT(onSpinBoxChanged(double)));
    connect(mpOpenBtn, SIGNAL(clicked()), SLOT(openFile()));
    connect(mpPlayPauseBtn, SIGNAL(clicked()), SLOT(togglePlayPause()));
    connect(mpCaptureBtn, SIGNAL(clicked()), this, SLOT(capture()));
    connect(mpInfoBtn, SIGNAL(clicked()), SLOT(showInfo()));
    //valueChanged can be triggered by non-mouse event
    //TODO: connect sliderMoved(int) to preview(int)
    //connect(mpTimeSlider, SIGNAL(sliderMoved(int)), this, SLOT(seekToMSec(int)));
    connect(mpTimeSlider, SIGNAL(sliderPressed()), SLOT(seek()));
    connect(mpTimeSlider, SIGNAL(sliderReleased()), SLOT(seek()));
    connect(mpTimeSlider, SIGNAL(onHover(int,int)), SLOT(onTimeSliderHover(int,int)));
    QTimer::singleShot(0, this, SLOT(initPlayer()));
}

void MainWindow::changeChannel(QAction *action)
{
    if (action == mpChannelAction) {
        action->toggle();
        return;
    }
    AudioFormat::ChannelLayout cl = (AudioFormat::ChannelLayout)action->data().toInt();
    AudioOutput *ao = mpPlayer ? mpPlayer->audio() : 0; //getAO()?
    if (!ao) {
        qWarning("No audio output!");
        return;
    }
    mpChannelAction->setChecked(false);
    mpChannelAction = action;
    mpChannelAction->setChecked(true);
    if (!ao->close()) {
        qWarning("close audio failed");
        return;
    }
    ao->audioFormat().setChannelLayout(cl);
    if (!ao->open()) {
        qWarning("open audio failed");
        return;
    }
}

void MainWindow::changeAudioTrack(QAction *action)
{
    if (mpAudioTrackAction == action) {
        action->toggle();
        return;
    }
    int track = action->data().toInt();

    if (!mpPlayer->setAudioStream(track, true)) {
        action->toggle();
        return;
    }
    mpAudioTrackAction->setChecked(false);
    mpAudioTrackAction = action;
    mpAudioTrackAction->setChecked(true);
}

void MainWindow::changeVO(QAction *action)
{
    if (action == mpVOAction) {
        action->toggle(); //check state changes if clicked
        return;
    }
    VideoRendererId vid = (VideoRendererId)action->data().toInt();
    VideoRenderer *vo = VideoRendererFactory::create(vid);
    if (vo && vo->isAvailable()) {
        if (vo->osdFilter()) {
            vo->osdFilter()->setShowType(OSD::ShowNone);
        }
        if (vo->widget()) {
            vo->widget()->resize(rect().size()); //TODO: why not mpPlayer->renderer()->rendererSize()?
            vo->resizeRenderer(mpPlayer->renderer()->rendererSize());
        }
        setRenderer(vo);
    } else {
        action->toggle(); //check state changes if clicked
        QMessageBox::critical(0, "QtAV", tr("not availabe on your platform!"));
        return;
    }
}

void MainWindow::processPendingActions()
{
    if (!mpTempRenderer)
        return;
    setRenderer(mpTempRenderer);
    mpTempRenderer = 0;
    if (mHasPendingPlay) {
        mHasPendingPlay = false;
        play(mFile);
    }
}

void MainWindow::enableAudio(bool yes)
{
    mNullAO = !yes;
    if (!mpPlayer)
        return;
    mpPlayer->enableAudio(yes);
}

void MainWindow::setAudioOutput(AudioOutput *ao)
{

}

void MainWindow::setRenderer(QtAV::VideoRenderer *renderer)
{
    if (!mIsReady) {
        mpTempRenderer = renderer;
        return;
    }
    if (!renderer)
        return;
#if SLIDER_ON_VO
    int old_pos = 0;
    int old_total = 0;

    if (mpTimeSlider) {
        old_pos = mpTimeSlider->value();
        old_total = mpTimeSlider->maximum();
        mpTimeSlider->hide();
    } else {
    }
#endif //SLIDER_ON_VO
    QWidget *r = 0;
    if (mpRenderer)
        r = mpRenderer->widget();
    //release old renderer and add new
    if (r) {
        mpPlayerLayout->removeWidget(r);
        if (r->testAttribute(Qt::WA_DeleteOnClose)) {
            r->close();
        } else {
            r->close();
            delete r;
        }
        r = 0;
    }
    mpRenderer = renderer;
    //setInSize?
    mpPlayerLayout->addWidget(renderer->widget());
    resize(renderer->widget()->size());

    renderer->widget()->setMouseTracking(true); //mouseMoveEvent without press.
    mpPlayer->setRenderer(renderer);
#if SLIDER_ON_VO
    if (mpTimeSlider) {
        mpTimeSlider->setParent(mpRenderer->widget());
        mpTimeSlider->show();
    }
#endif //SLIDER_ON_VO
    if (mpVOAction) {
        mpVOAction->setChecked(false);
    }
    foreach (QAction *action, mVOActions) {
        if (action->data() == renderer->id()) {
            mpVOAction = action;
            break;
        }
    }
    mpVOAction->setChecked(true);
    mpTitle->setText(mpVOAction->text());
}

void MainWindow::play(const QString &name)
{
    mFile = name;
    if (!mIsReady) {
        mHasPendingPlay = true;
        return;
    }
    if (!mFile.contains("://") || mFile.startsWith("file://")) {
        mTitle = QFileInfo(mFile).fileName();
    }
    setWindowTitle(mTitle);
    mpPlayer->enableAudio(!mNullAO);
    if (!mpRepeatEnableAction->isChecked())
        mRepeateMax = 0;
    mpPlayer->setRepeat(mRepeateMax);
    mpPlayer->setPriority(Config::instance().decoderPriority());
    PlayListItem item;
    item.setUrl(mFile);
    item.setTitle(mTitle);
    item.setLastTime(0);
    mpHistory->remove(mFile);
    mpHistory->insertItemAt(item, 0);
    mpPlayer->play(name);
}

void MainWindow::setVideoDecoderNames(const QStringList &vd)
{
    QStringList vdnames;
    foreach (QString v, vd) {
        vdnames << v.toLower();
    }
    QVector<VideoDecoderId> vidp;
    QVector<VideoDecoderId> vids = GetRegistedVideoDecoderIds();
    foreach (VideoDecoderId vid, vids) {
        QString v(VideoDecoderFactory::name(vid).c_str());
        if (vdnames.contains(v.toLower())) {
            vidp.append(vid);
        }
    }
    Config::instance().decoderPriority(vidp);
}

void MainWindow::openFile()
{
    QString file = QFileDialog::getOpenFileName(0, tr("Open a media file"));
    if (file.isEmpty())
        return;
    play(file);
}

void MainWindow::togglePlayPause()
{
    if (mpPlayer->isPlaying()) {
        qDebug("isPaused = %d", mpPlayer->isPaused());
        mpPlayer->pause(!mpPlayer->isPaused());
    } else {
        if (mFile.isEmpty())
            return;
        mpPlayer->play();
        mpPlayPauseBtn->setIconWithSates(mPausePixmap);
    }
}

void MainWindow::onSpinBoxChanged(double v)
{
    if (!mpPlayer)
        return;
    mpPlayer->setSpeed(v);
}

void MainWindow::onPaused(bool p)
{
    if (p) {
        qDebug("start pausing...");
        mpPlayPauseBtn->setIconWithSates(mPlayPixmap);
    } else {
        qDebug("stop pausing...");
        mpPlayPauseBtn->setIconWithSates(mPausePixmap);
    }
}

void MainWindow::onStartPlay()
{
    mFile = mpPlayer->file(); //open from EventFilter's menu
    setWindowTitle(mTitle);

    mpPlayPauseBtn->setIconWithSates(mPausePixmap);
    mpTimeSlider->setMaximum(mpPlayer->duration());
    mpTimeSlider->setValue(0);
    qDebug(">>>>>>>>>>>>>>enable slider");
    mpTimeSlider->setEnabled(true);
    mpDuration->setText(QTime(0, 0, 0).addMSecs(mpPlayer->duration()).toString("HH:mm:ss"));
    setVolume();
    mShowControl = 0;
    QTimer::singleShot(3000, this, SLOT(tryHideControlBar()));
    ScreenSaver::instance().disable();
    initAudioTrackMenu();
    mpRepeatA->setMaximumTime(QTime(0, 0, 0).addMSecs(mpPlayer->mediaStopPosition()));
    mpRepeatB->setMaximumTime(QTime(0, 0, 0).addMSecs(mpPlayer->mediaStopPosition()));
    mpRepeatA->setTime(QTime(0, 0, 0).addMSecs(mpPlayer->startPosition()));
    mpRepeatB->setTime(QTime(0, 0, 0).addMSecs(mpPlayer->stopPosition()));
    mCursorTimer = startTimer(3000);
    PlayListItem item = mpHistory->itemAt(0);
    item.setUrl(mFile);
    item.setTitle(mTitle);
    item.setDuration(mpPlayer->duration());
    mpHistory->setItemAt(item, 0);
}

void MainWindow::onStopPlay()
{
    mpPlayer->setPriority(Config::instance().decoderPriority());
    if (mpPlayer->currentRepeat() < mpPlayer->repeat())
        return;
    mpPlayPauseBtn->setIconWithSates(mPlayPixmap);
    mpTimeSlider->setValue(0);
    qDebug(">>>>>>>>>>>>>>disable slider");
    mpTimeSlider->setDisabled(true);
    mpCurrent->setText("00:00:00");
    mpDuration->setText("00:00:00");
    tryShowControlBar();
    ScreenSaver::instance().enable();
    toggleRepeat(false);
    //mRepeateMax = 0;
    killTimer(mCursorTimer);
    unsetCursor();
}

void MainWindow::onSpeedChange(qreal speed)
{
    mpSpeed->setText(QString("%1").arg(speed, 4, 'f', 2, '0'));
}

void MainWindow::seekToMSec(int msec)
{
    mpPlayer->seek(qint64(msec));
}

void MainWindow::seek()
{
    mpPlayer->seek((qint64)mpTimeSlider->value());
}

void MainWindow::capture()
{
    mpPlayer->captureVideo();
}

void MainWindow::showHideVolumeBar()
{
    if (mpVolumeSlider->isHidden()) {
        mpVolumeSlider->show();
    } else {
        mpVolumeSlider->hide();
    }
}

void MainWindow::setVolume()
{
    AudioOutput *ao = mpPlayer ? mpPlayer->audio() : 0;
    qreal v = qreal(mpVolumeSlider->value())*kVolumeInterval;
    if (ao) {
        ao->setVolume(v);
    }
    mpVolumeSlider->setToolTip(QString::number(v));
    mpVolumeBtn->setToolTip(QString::number(v));
}

void MainWindow::closeEvent(QCloseEvent *e)
{
    if (mpPlayer)
        mpPlayer->stop();
    qApp->quit();
}

void MainWindow::resizeEvent(QResizeEvent *e)
{
    Q_UNUSED(e);
#if 0
    if (e->size() == qApp->desktop()->size()) {
        mpControl->hide();
        mpTimeSlider->hide();
    } else {
        if (mpControl->isHidden())
            mpControl->show();
        if (mpTimeSlider->isHidden())
            mpTimeSlider->show();
    }
#endif
    /*
    if (mpTitle)
        QLabelSetElideText(mpTitle, QFileInfo(mFile).fileName(), e->size().width());
    */
#if SLIDER_ON_VO
    int m = 4;
    QWidget *w = static_cast<QWidget*>(mpTimeSlider->parent());
    if (w) {
        mpTimeSlider->resize(w->width() - m*2, 44);
        qDebug("%d %d %d", m, w->height() - mpTimeSlider->height() - m, w->width() - m*2);
        mpTimeSlider->move(m, w->height() - mpTimeSlider->height() - m);
    }
#endif //SLIDER_ON_VO
}

void MainWindow::timerEvent(QTimerEvent *e)
{
    if (e->timerId() == mCursorTimer) {
        if (mpControl->isVisible())
            return;
        setCursor(Qt::BlankCursor);
    }
}

void MainWindow::onPositionChange(qint64 pos)
{
    mpTimeSlider->setValue(pos);
    mpCurrent->setText(QTime(0, 0, 0).addMSecs(pos).toString("HH:mm:ss"));
}

void MainWindow::repeatAChanged(const QTime& t)
{
    if (!mpPlayer)
        return;
    mpPlayer->setStartPosition(QTime(0, 0, 0).msecsTo(t));
}

void MainWindow::repeatBChanged(const QTime& t)
{
    if (!mpPlayer)
        return;
    mpPlayer->setStopPosition(QTime(0, 0, 0).msecsTo(t));
}

void MainWindow::keyPressEvent(QKeyEvent *e)
{
    mControlOn = e->key() == Qt::Key_Control;
}

void MainWindow::keyReleaseEvent(QKeyEvent *e)
{
    mControlOn = false;
}

void MainWindow::mousePressEvent(QMouseEvent *e)
{
    if (!mControlOn)
        return;
    mGlobalMouse = e->globalPos();
}

void MainWindow::mouseMoveEvent(QMouseEvent *e)
{
    unsetCursor();
    if (e->pos().y() > height() - mpTimeSlider->height() - mpControl->height()) {
        if (mShowControl == 0) {
            mShowControl = 1;
            tryShowControlBar();
        }
    } else {
        if (mShowControl == 1) {
            mShowControl = 0;
            QTimer::singleShot(3000, this, SLOT(tryHideControlBar()));
        }
    }
    if (mControlOn && e->button() == Qt::LeftButton) {
        if (!mpRenderer || !mpRenderer->widget())
            return;
        QRectF roi = mpRenderer->realROI();
        QPointF delta = e->pos() - mGlobalMouse;
        roi.moveLeft(-delta.x());
        roi.moveTop(-delta.y());
        mpRenderer->setRegionOfInterest(roi);
    }
}

void MainWindow::wheelEvent(QWheelEvent *e)
{
    if (!mControlOn)
        return;
    if (!mpRenderer || !mpRenderer->widget()) {
        return;
    }
    QPointF p = mpRenderer->widget()->mapFrom(this, e->pos());
    QPointF fp = mpRenderer->mapToFrame(p);
    if (fp.x() < 0)
        fp.setX(0);
    if (fp.y() < 0)
        fp.setY(0);
    if (fp.x() > mpRenderer->frameSize().width())
        fp.setX(mpRenderer->frameSize().width());
    if (fp.y() > mpRenderer->frameSize().height())
        fp.setY(mpRenderer->frameSize().height());

    QRectF viewport = QRectF(mpRenderer->mapToFrame(QPointF(0, 0)), mpRenderer->mapToFrame(QPointF(mpRenderer->rendererWidth(), mpRenderer->rendererHeight())));
    //qDebug("vo: (%.1f, %.1f)=> frame: (%.1f, %.1f)", p.x(), p.y(), fp.x(), fp.y());

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    qreal deg = e->angleDelta().y()/8;
#else
    qreal deg = e->delta()/8;
#endif //QT_VERSION
    qreal zoom = 1.0 + deg*3.14/180.0;
    //qDebug("deg: %d, %d zoom: %.2f", e->angleDelta().x(), e->angleDelta().y(), zoom);
    QTransform m;
    m.translate(fp.x(), fp.y());
    m.scale(1.0/zoom, 1.0/zoom);
    m.translate(-fp.x(), -fp.y());
    QRectF r = m.mapRect(mpRenderer->realROI());
    mpRenderer->setRegionOfInterest((r | m.mapRect(viewport))&QRectF(QPointF(0,0), mpRenderer->frameSize()));
}

void MainWindow::about()
{
    QtAV::about();
}

void MainWindow::help()
{
    QFile f(qApp->applicationDirPath() + "/help.html");
    if (!f.exists()) {
        f.setFileName(":/help.html");
    }
    if (!f.open(QIODevice::ReadOnly)) {
        qWarning("Failed to open help.html: %s", qPrintable(f.errorString()));
        return;
    }
    QTextStream ts(&f);
    QString text = ts.readAll();
    QMessageBox::information(0, "Help", text);
}

void MainWindow::openUrl()
{
    QString url = QInputDialog::getText(0, tr("Open an url"), tr("Url"));
    if (url.isEmpty())
        return;
    play(url);
}

void MainWindow::initAudioTrackMenu()
{
    int track = mpPlayer->currentAudioStream();
    QList<QAction*> as = mpAudioTrackMenu->actions();
    int tracks = mpPlayer->audioStreamCount();
    if (mpAudioTrackAction && tracks == as.size() && mpAudioTrackAction->data().toInt() == track)
        return;
    while (tracks < as.size()) {
        QAction *a = as.takeLast();
        mpAudioTrackMenu->removeAction(a);
        delete a;
    }
    while (tracks > as.size()) {
        QAction *a = mpAudioTrackMenu->addAction(QString::number(as.size()));
        a->setData(as.size());
        a->setCheckable(true);
        a->setChecked(false);
        as.push_back(a);
    }
    if (as.isEmpty()) {
        mpAudioTrackAction = 0;
        return;
    }
    foreach(QAction *a, as) {
        if (a->data().toInt() == track) {
            qDebug("track found!!!!!");
            mpAudioTrackAction = a;
            a->setChecked(true);
        } else {
            a->setChecked(false);
        }
    }
    mpAudioTrackAction->setChecked(true);
}

void MainWindow::switchAspectRatio(QAction *action)
{
    qreal r = action->data().toDouble();
    if (action == mpARAction && r != -2) {
        action->toggle(); //check state changes if clicked
        return;
    }
    if (r == 0) {
        mpPlayer->renderer()->setOutAspectRatioMode(VideoRenderer::VideoAspectRatio);
    } else if (r == -1) {
        mpPlayer->renderer()->setOutAspectRatioMode(VideoRenderer::RendererAspectRatio);
    } else {
        if (r == -2)
            r = QInputDialog::getDouble(0, tr("Aspect ratio"), "", 1.0);
        mpPlayer->renderer()->setOutAspectRatioMode(VideoRenderer::CustomAspectRation);
        mpPlayer->renderer()->setOutAspectRatio(r);
    }
    mpARAction->setChecked(false);
    mpARAction = action;
    mpARAction->setChecked(true);
}

void MainWindow::toggleRepeat(bool r)
{
    mpRepeatEnableAction->setChecked(r);
    mpRepeatAction->defaultWidget()->setEnabled(r); //why need defaultWidget?
    if (r) {
        mRepeateMax = mpRepeatBox->value();
    } else {
        mRepeateMax = 0;
    }
    if (mpPlayer) {
        mpPlayer->setRepeat(mRepeateMax);
    }
}

void MainWindow::setRepeateMax(int m)
{
    mRepeateMax = m;
    if (mpPlayer) {
        mpPlayer->setRepeat(m);
    }
}

void MainWindow::playOnlineVideo(QAction *action)
{
    mTitle = action->text();
    play(action->data().toString());
}

void MainWindow::onTVMenuClick()
{
    static TVView *tvv = new TVView;
    tvv->show();
    connect(tvv, SIGNAL(clicked(QString,QString)), SLOT(onPlayListClick(QString,QString)));
    tvv->setMaximumHeight(qApp->desktop()->height());
    tvv->setMinimumHeight(tvv->width()*2);
}

void MainWindow::onPlayListClick(const QString &key, const QString &value)
{
    mTitle = key;
    play(value);
}

void MainWindow::tryHideControlBar()
{
    if (mShowControl > 0) {
        return;
    }
    if (mpControl->isHidden() && mpTimeSlider->isHidden())
        return;
    mpControl->hide();
    mpTimeSlider->hide();
}

void MainWindow::tryShowControlBar()
{
    unsetCursor();
    if (mpTimeSlider->isHidden())
        mpTimeSlider->show();
    if (mpControl->isHidden())
        mpControl->show();
}

void MainWindow::showInfo()
{
    // why crash if on stack
    static StatisticsView *sv = new StatisticsView;
    if (mpPlayer)
        sv->setStatistics(mpPlayer->statistics());
    sv->show();
}

void MainWindow::onTimeSliderHover(int pos, int value)
{
    QToolTip::showText(mapToGlobal(mpTimeSlider->pos() + QPoint(pos, 0)), QTime(0, 0, 0).addMSecs(value).toString("HH:mm:ss"));
}

void MainWindow::onTimeSliderLeave()
{

}

void MainWindow::handleError(const AVError &e)
{
    QMessageBox::warning(0, "Player error", e.string());
}
