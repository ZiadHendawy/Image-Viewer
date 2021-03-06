#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QImage>
#include <QUndoStack>
#include <QUndoView>
#include <QRect>
#include <QRubberBand>
#include <QPushButton>
#include <QBoxLayout>

class QAction;
class QMenu;
class QActionGroup;
class QLabel;
class QScrollArea;
class QScrollBar;
class QUndoStack;
class QUndoView;
//class QPushButton;

class MainWindow : public QMainWindow
{
    Q_OBJECT

  public:
      MainWindow();
      bool loadFile(const QString &);

  private slots:
      void open();
      void saveAs();
      void undo();
      void redo();
      void reset();

      //crop
      void crop();
      void cropHelper();
      //rotation
      void rotateLeft();
      void rotateRight();
      void rotateAngle();

      //zoom
      void zoomIn();
      void zoomOut();
      void fitScreen();
      void zoomInHelper();
      void zoomOutHelper();

      //about
      void about();


private:
    void createMenus();
    void createActions();
    void updateActions();
    bool openFile(const QString &fileName);
    bool saveFile(const QString &fileName);
    void setImage(const QImage &newImage);
    void scaleImage(double factor);
    void adjustScrollBar(QScrollBar *scrollBar, double factor);
    void setImageLabel(QSize);
    void rotate(double roationAngle);

    QPoint origin;
    QPoint endOrigin;
    QImage image;
    QImage originalImage;
    QLabel *imageLabel;
    QScrollArea *scrollArea;
    QSize originalImageLabelSize;


    double scaleFactor;
    double factorSaved;
    bool click ;
    bool isZoomedIn;
    bool isZoomedOut;
    bool isCrop;
    double totalRotationAngle;
    double widthBefore;
    double heightBefore;

    QRubberBand *rubberBand = NULL;
    QPoint myPoint;
    QPushButton btn_rotateLeft;
    QPushButton btn_rotateRight;

    QMenu *fileMenu;
    QMenu *editeMenu;
    QMenu *viewMenu;
    QMenu *helpMenu;

    QAction *openAction;
    QAction *saveAction;
    QAction *closeAction;
    QAction *undoAction;
    QAction *redoAction;
    QAction *resetAction;
    QAction *cropAction;
    QAction *rotateAngleAction;
    QAction *rotateLeftAction;
    QAction *rotateRightAction;
    QAction *zoomInAction;
    QAction *zoomOutAction;
    QAction *fitScreenAction;

    QStack<QPair<QImage,double>> *undoStack ;//undostack ---> pair {image,labelsize}
    QStack<QPair<QImage,double>> *redoStack ;


protected:
    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

//#ifndef QT_NO_CONTEXTMENU
    //void contextMenuEvent(QContextMenuEvent *event) Q_DECL_OVERRIDE;
//#endif // QT_NO_CONTEXTMENU

};

#endif // MAINWINDOW_H
