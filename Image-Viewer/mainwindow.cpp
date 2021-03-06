#include "mainwindow.h"
#include <stack>
#include <QtWidgets>
#include <stack>


//---------------constructor-------------------------------------------------------
MainWindow::MainWindow() : imageLabel(new QLabel), scrollArea(new QScrollArea), scaleFactor(1), btn_rotateLeft(new QPushButton), btn_rotateRight(new QPushButton)
{

  factorSaved = 1.0;
  isZoomedIn = false;
  isZoomedOut = false;
  isCrop = false;

  undoStack = new QStack<QPair<QImage,double>>();
  redoStack = new QStack<QPair<QImage,double>>();

  //imageLabel->setMargin(10);
  imageLabel->setStyleSheet("QLabel { background-color : black;  }");
  //imageLabel->setContentsMargins(1000,100,1000,100);
  imageLabel->setScaledContents(true);
  //imageLabel->setFrameRect(QRect(0,100,5000,100));

  //imageLabel->setMaximumHeight(1000);
  //imageLabel->setMaximumWidth(1500);
  //imageLabel->setMinimumHeight(100);
  //imageLabel->setMinimumWidth(200);

  imageLabel->setBackgroundRole(QPalette::Base);
  imageLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
  imageLabel->setAlignment(Qt::AlignAbsolute);

  scaleFactor = 1.0;
  scrollArea->setBackgroundRole(QPalette::Dark);
  scrollArea->setWidget(imageLabel);
  scrollArea->setVisible(false);
  setCentralWidget(scrollArea);
  createActions();
  resize(QGuiApplication::primaryScreen()->availableSize());


}

//-mouse select press,move,release-------------------------------------------------
void MainWindow::mousePressEvent(QMouseEvent *event){
  event->accept();

  origin = event->pos();//imageLabel->mapFromGlobal(event->globalPos());
  if(rubberBand){
      rubberBand ->close();
      rubberBand = NULL;
  }
  else{
    rubberBand = new QRubberBand(QRubberBand::Rectangle, this);
    rubberBand->setGeometry(QRect(origin, QSize()));
    rubberBand->show();
  }
  scrollArea->verticalScrollBar()->setEnabled(true);
  scrollArea->horizontalScrollBar()->setEnabled(true);

}

void MainWindow::mouseMoveEvent(QMouseEvent *event){
  event->accept();

  if(rubberBand){
     rubberBand->setGeometry(QRect(origin, event->pos()).normalized());
     rubberBand->show();
   }

}

void MainWindow::mouseReleaseEvent(QMouseEvent *event){
  event->accept();
  endOrigin = event->pos();//imageLabel->mapFromGlobal(event->globalPos());
  if(rubberBand){
      rubberBand->show();
   }

  if(isZoomedIn){
      undoStack->push(qMakePair(image,factorSaved));
      zoomInHelper();
      isZoomedIn = false;
  }
  if (isCrop){
      undoStack->push(qMakePair(image,factorSaved));
      cropHelper();
      isCrop = false;
  }
  rubberBand->close();
  rubberBand = NULL;
}


//--------load picture (open) using file dialog-----------------------------------
bool MainWindow::loadFile(const QString &fileName)
{

    QImageReader reader(fileName);
    reader.setAutoTransform(true);
    const QImage newImage = reader.read();
    originalImage = newImage;
    if (newImage.isNull()) {
        QMessageBox::information(this, QGuiApplication::applicationDisplayName(),
                                 tr("Cannot load %1: %2")
                                 .arg(QDir::toNativeSeparators(fileName), reader.errorString()));
        return false;
    }

    setImage(newImage);

    setWindowFilePath(fileName);
    widthBefore = newImage.width();
    heightBefore = newImage.height();
    const QString message = tr("Opened \"%1\", %2x%3, Depth: %4")
        .arg(QDir::toNativeSeparators(fileName)).arg(image.width()).arg(image.height()).arg(image.depth());
    statusBar()->showMessage(message);
    return true;

}



static void initializeImageFileDialog(QFileDialog &dialog, QFileDialog::AcceptMode acceptMode)
{
    static bool firstDialog = true;

    if (firstDialog) {
        firstDialog = false;
        const QStringList picturesLocations = QStandardPaths::standardLocations(QStandardPaths::PicturesLocation);
        dialog.setDirectory(picturesLocations.isEmpty() ? QDir::currentPath() : picturesLocations.last());
    }

    QStringList mimeTypeFilters;
    const QByteArrayList supportedMimeTypes = acceptMode == QFileDialog::AcceptOpen
        ? QImageReader::supportedMimeTypes() : QImageWriter::supportedMimeTypes();
    foreach (const QByteArray &mimeTypeName, supportedMimeTypes)
        mimeTypeFilters.append(mimeTypeName);
    mimeTypeFilters.sort();
    dialog.setMimeTypeFilters(mimeTypeFilters);
    dialog.selectMimeTypeFilter("image/jpeg");
    if (acceptMode == QFileDialog::AcceptSave)
        dialog.setDefaultSuffix("jpg");
}
void MainWindow::open(){
    QFileDialog dialog(this, tr("Open File"));
    initializeImageFileDialog(dialog, QFileDialog::AcceptOpen);
    while (dialog.exec() == QDialog::Accepted && !loadFile(dialog.selectedFiles().first())) {}



}

//-------------setting image-----------------------------------------------------
void MainWindow::setImage(const QImage &newImage){

  image = newImage;
  imageLabel->setPixmap(QPixmap::fromImage(image));
 // imageLabel->setPixmap(QPixmap::fromImage(image).scaled(image.width()/2,image.height()/2,Qt::KeepAspectRatio));
  scrollArea->setVisible(true);
  updateActions();
  imageLabel->adjustSize();

}


//-------------saving------------------------------------------------------------



bool MainWindow::saveFile(const QString &fileName)
{
    QImageWriter writer(fileName);

    if (!writer.write(image)) {
        QMessageBox::information(this, QGuiApplication::applicationDisplayName(),
                                 tr("Cannot write %1: %2")
                                 .arg(QDir::toNativeSeparators(fileName)), writer.errorString());
        return false;
    }
    const QString message = tr("Wrote \"%1\"").arg(QDir::toNativeSeparators(fileName));
    statusBar()->showMessage(message);
    return true;
}


void MainWindow::saveAs(){
    QFileDialog dialog(this, tr("Save File As"));
    initializeImageFileDialog(dialog, QFileDialog::AcceptSave);

    while (dialog.exec() == QDialog::Accepted && !saveFile(dialog.selectedFiles().first())) {}

}


//-------------undo redo---------------------------------------------------------
void MainWindow::undo(){

    if(undoStack->size()<=0)
           return;
    redoStack->push(qMakePair(image,factorSaved));
    QPair<QImage,double>popped= undoStack->pop();
    setImage(popped.first);

    scaleImage(1/factorSaved);
    factorSaved = popped.second;

}

void MainWindow::redo(){
    if(redoStack->size()==0)
           return;
    undoStack->push(qMakePair(image,factorSaved));
    QPair<QImage,double>popped = redoStack->pop();
    setImage(popped.first);
    scaleImage(popped.second);
    factorSaved = popped.second;


}
//-------------reset-------------------------------------------------------------
void MainWindow::reset(){
    setImage(originalImage);

}

//-------------crop--------------------------------------------------------------
void MainWindow::crop(){
    isCrop = true;
}
void MainWindow::cropHelper(){
    delete redoStack;
    redoStack= new QStack<QPair<QImage,double>>();
       QImage copy ;
       QPoint tmp1 = origin;
       QPoint tmp2 = endOrigin;
       int x1 =tmp1.x();
       int x2 =tmp2.x();
       int y1 = tmp1.y();
       int y2 =tmp2.y();

        copy = image.copy(std::min(x1,x2)+scrollArea->horizontalScrollBar()->value(),std::min(y1,y2)+scrollArea->verticalScrollBar()->value(),std::abs(tmp1.x()-tmp2.x()),std::abs(tmp1.y()-tmp2.y()));
      // copy = image.copy(std::min(x1,x2),std::min(y1,y2),std::abs(tmp1.x()-tmp2.x()),std::abs(tmp1.y()-tmp2.y()));

       setImage(copy);
}
//-------------rotation----------------------------------------------------------
void MainWindow::rotate(double rotationAngle){
    QTransform trans;
    QImage tmp;
    trans.rotate(rotationAngle);
    totalRotationAngle+=rotationAngle;
    qDebug()<<totalRotationAngle;
    if((int)totalRotationAngle %360 == 0){

        image = originalImage;
         cropAction->setEnabled(true);

    }
    else
     {
        qDebug()<<"kolo awleed";
        image = image.transformed(trans);

        cropAction->setEnabled(false);


    }
    setImage(image);


}
void MainWindow::rotateAngle(){

    bool ok;
    double value=0,min=-359,max=359;
    int decimals = 1;
    double rotationAngle =   QInputDialog::getDouble(this,
                                                            tr("Rotation"),
                                                            tr("Angle:"),
                                                             value,
                                                             min,
                                                             max ,
                                                             decimals,
                                                               &ok,
                                                          Qt::WindowFlags());

     if(ok)
    {
      undoStack->push(qMakePair(image,factorSaved));
      delete redoStack;
      redoStack= new QStack<QPair<QImage,double>>();
      rotate(rotationAngle);
    }
}

void MainWindow::rotateLeft(){
   undoStack->push(qMakePair(image,factorSaved));
  QTransform trans;
  trans.rotate(270);
  image = image.transformed(trans);
  delete redoStack;
  redoStack= new QStack<QPair<QImage,double>>();

  setImage(image);
}
void MainWindow::rotateRight(){
 undoStack->push(qMakePair(image,factorSaved));
  QTransform trans;
  trans.rotate(90);
  image = image.transformed(trans);
  delete redoStack;
  redoStack= new QStack<QPair<QImage,double>>();

  setImage(image);
}

//---------------zoom-------------------------------------------------------------
void MainWindow::zoomIn(){
       isZoomedIn = true;
}
void MainWindow::zoomOut(){
    zoomOutHelper();
}

void MainWindow::zoomInHelper(){
    //removing redo stack

    delete redoStack;
    redoStack= new QStack<QPair<QImage,double>>();

    float pre_hor_val = scrollArea->horizontalScrollBar()->value();
    float pre_ver_val = scrollArea->verticalScrollBar()->value();

    scaleImage(1.25);
    scrollArea->setUpdatesEnabled(true);
    scrollArea->horizontalScrollBar()->setUpdatesEnabled(true);
    scrollArea->verticalScrollBar()->setUpdatesEnabled(true);


    scrollArea->horizontalScrollBar()->setRange(std::min(origin.rx(),endOrigin.rx())+rubberBand->width()/2,imageLabel->width());
    scrollArea->horizontalScrollBar()->setValue(pre_hor_val*1.25);
    scrollArea->verticalScrollBar()->setRange(std::min(origin.ry(),endOrigin.ry())+rubberBand->height()/2,imageLabel->height());
    scrollArea->verticalScrollBar()->setValue(pre_ver_val*1.25);


}
void MainWindow::zoomOutHelper(){
    undoStack->push(qMakePair(image,factorSaved));
    delete redoStack;
   redoStack= new QStack<QPair<QImage,double>>();


    scaleImage(1/1.25);
    scrollArea->setUpdatesEnabled(true);
    scrollArea->horizontalScrollBar()->setUpdatesEnabled(true);
    scrollArea->verticalScrollBar()->setUpdatesEnabled(true);



}

void MainWindow::fitScreen(){
  imageLabel->adjustSize();
  scaleFactor = 1.0;
}
//--------------about--------------------------------------------------------------
void MainWindow::about(){
  QMessageBox::about(this, tr("About Image-Viewer"),
    tr("<p> <b>Image-Viewer</b> is Developed by"
        "<br>"
        "Mohamed Sabra"
       "<br>"
       "Ziad Hendawy"
        "<br>"
        "Ahmed Sallam"
        "<br>"
        ));
}


void MainWindow::createActions(){
  fileMenu = menuBar()->addMenu(tr("&File"));

  openAction = fileMenu->addAction(tr("&Open..."), this, &MainWindow::open);
  openAction->setShortcut(QKeySequence::Open);

  saveAction = fileMenu->addAction(tr("&Save As..."), this, &MainWindow::saveAs);
  saveAction->setShortcut(tr("Ctrl+S"));
  saveAction->setEnabled(false);

  fileMenu->addSeparator();

  closeAction = fileMenu->addAction(tr("E&xit"), this, &QWidget::close);
  closeAction->setShortcut(tr("Ctrl+Q"));

  editeMenu = menuBar()->addMenu(tr("&Edit"));

  undoAction = editeMenu->addAction(tr("&Undo"), this, &MainWindow::undo);
  undoAction->setShortcut(tr("Ctrl+Z"));
  undoAction->setEnabled(false);

  redoAction = editeMenu->addAction(tr("&Redo"), this, &MainWindow::redo);
  redoAction->setShortcut(tr("Ctrl+Y"));
  redoAction->setEnabled(false);

  resetAction = editeMenu->addAction(tr("&Reset"), this, &MainWindow::reset);
  resetAction->setShortcut(tr("Ctrl+F"));
  resetAction->setEnabled(false);

  editeMenu->addSeparator();

  cropAction = editeMenu->addAction(tr("&Crop"), this, &MainWindow::crop);
  cropAction->setShortcut(tr("Ctrl+C"));
  cropAction->setEnabled(false);

  editeMenu->addSeparator();

  rotateAngleAction = editeMenu->addAction(tr("&Rotate By Angle"),this,&MainWindow::rotateAngle);
  rotateAngleAction->setEnabled(false);

  rotateLeftAction = editeMenu->addAction(tr("&Rotate Left by 90"), this, &MainWindow::rotateLeft);
  rotateLeftAction->setShortcut(tr("Ctrl+L"));
  rotateLeftAction->setEnabled(false);

  rotateRightAction = editeMenu->addAction(tr("&Rotate Right by 90"), this, &MainWindow::rotateRight);
  rotateRightAction->setShortcut(tr("Ctrl+R"));
  rotateRightAction->setEnabled(false);

  viewMenu = menuBar()->addMenu(tr("&View"));

  zoomInAction = viewMenu->addAction(tr("Zoom &In"), this, &MainWindow::zoomIn);
  zoomInAction->setShortcut(QKeySequence::ZoomIn);
  zoomInAction->setEnabled(false);

  zoomOutAction = viewMenu->addAction(tr("Zoom &Out"), this, &MainWindow::zoomOut);
  zoomOutAction->setShortcut(QKeySequence::ZoomOut);
  zoomOutAction->setEnabled(false);


  helpMenu = menuBar()->addMenu(tr("&Help"));
  helpMenu->addAction(tr("&About"), this, &MainWindow::about);
}

void MainWindow::updateActions(){

    saveAction->setEnabled(!image.isNull());

    zoomInAction->setEnabled(!image.isNull());
    zoomOutAction->setEnabled(!image.isNull());

    undoAction->setEnabled(!undoStack->isEmpty());
    redoAction->setEnabled(!redoStack->isEmpty());

    resetAction->setEnabled(!image.isNull());

    rotateLeftAction->setEnabled(!image.isNull());
    rotateRightAction->setEnabled(!image.isNull());
    rotateAngleAction->setEnabled(!image.isNull());

    cropAction->setEnabled(!image.isNull());
}

void MainWindow::scaleImage(double factor){
  Q_ASSERT(imageLabel->pixmap());
  factorSaved = factor;
  scaleFactor *= factor;
  cropAction->setEnabled(scaleFactor==1);
  imageLabel->resize(scaleFactor * imageLabel->pixmap()->size());
  adjustScrollBar(scrollArea->horizontalScrollBar(), factor);
  adjustScrollBar(scrollArea->verticalScrollBar(), factor);

  zoomInAction->setEnabled(scaleFactor < 3.0);
  zoomOutAction->setEnabled(scaleFactor > 0.333);
  undoAction->setEnabled(!undoStack->isEmpty());
  redoAction->setEnabled(!redoStack->isEmpty());

}

void MainWindow::adjustScrollBar(QScrollBar *scrollBar, double factor){
  scrollBar->setValue(int(factor * scrollBar->value() + ((factor - 1) * scrollBar->pageStep()/2)));
}
