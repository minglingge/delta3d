#include <dtEditQt/stageglwidget.h>
#include <dtEditQt/editorviewport.h>
#include <QtGui/QMouseEvent>

////////////////////////////////////////////////////////////////////////////////
dtEditQt::STAGEGLWidget::STAGEGLWidget(bool drawOnSeparateThread,  QWidget* parent,
                                       const QGLWidget* shareWidget, Qt::WindowFlags f)
   : dtQt::OSGAdapterWidget(drawOnSeparateThread, parent, shareWidget, f)
   , mViewport(NULL)
   , mIsMouseOver(false)
{
}

////////////////////////////////////////////////////////////////////////////////
dtEditQt::STAGEGLWidget::~STAGEGLWidget()
{

}

////////////////////////////////////////////////////////////////////////////////
void dtEditQt::STAGEGLWidget::mouseMoveEvent(QMouseEvent* e)
{
   dtQt::OSGAdapterWidget::mouseMoveEvent(e);

   if (mViewport!= NULL)
   {
      mViewport->mouseMoveEvent(e);
   }
}

////////////////////////////////////////////////////////////////////////////////
void dtEditQt::STAGEGLWidget::SetViewport(EditorViewport* viewport)
{
   mViewport = viewport;
}

////////////////////////////////////////////////////////////////////////////////
void dtEditQt::STAGEGLWidget::mousePressEvent(QMouseEvent* e)
{
   dtQt::OSGAdapterWidget::mousePressEvent(e);

   if (mViewport != NULL)
   {
      mViewport->mousePressEvent(e);
   }
}

////////////////////////////////////////////////////////////////////////////////
void dtEditQt::STAGEGLWidget::mouseReleaseEvent(QMouseEvent* e)
{
   dtQt::OSGAdapterWidget::mouseReleaseEvent(e);

   if (mViewport != NULL)
   {
      mViewport->mouseReleaseEvent(e);
   }
}

////////////////////////////////////////////////////////////////////////////////
void dtEditQt::STAGEGLWidget::dragEnterEvent(QDragEnterEvent* event)
{
   mIsMouseOver = true;

   if (mViewport != NULL)
   {
      if (!mViewport->GetIsRemoved())
      {
         ViewportManager::GetInstance().EnableViewport(mViewport, true);
         mViewport->dragEnterEvent(event);
      }
   }
}

////////////////////////////////////////////////////////////////////////////////
void dtEditQt::STAGEGLWidget::dragLeaveEvent(QDragLeaveEvent* event)
{
   mIsMouseOver = false;

   if (mViewport != NULL)
   {
      if (!mViewport->GetIsRemoved())
      {
         ViewportManager::GetInstance().EnableViewport(mViewport, false);
         mViewport->dragLeaveEvent(event);
      }
   }
}

////////////////////////////////////////////////////////////////////////////////
void dtEditQt::STAGEGLWidget::dragMoveEvent(QDragMoveEvent* event)
{
   if (mViewport != NULL)
   {
      mViewport->dragMoveEvent(event);
   }
}

////////////////////////////////////////////////////////////////////////////////
void dtEditQt::STAGEGLWidget::dropEvent(QDropEvent* event)
{
   if (mViewport != NULL)
   {
      mViewport->dropEvent(event);
   }
}

////////////////////////////////////////////////////////////////////////////////
void dtEditQt::STAGEGLWidget::paintGL()
{
   if (mViewport == NULL || mViewport->GetIsRemoved()) 
   {
      return;
   }

   //Qt wants to redraw this widget.  If this View isn't currently in the Application
   //temporarily add it, render, then remove it.
   bool viewAdded = false;
   if (ViewportManager::GetInstance().EnableViewport(mViewport, true))
   {
      viewAdded = true;
   }

   dtQt::OSGAdapterWidget::paintGL();
   mViewport->paintGL();

   //put things back the way they were
   if (viewAdded && !mIsMouseOver)
   {
      ViewportManager::GetInstance().EnableViewport(mViewport, false);
   }   
}

////////////////////////////////////////////////////////////////////////////////
void dtEditQt::STAGEGLWidget::initializeGL()
{
   dtQt::OSGAdapterWidget::initializeGL();

   if (mViewport != NULL)
   {
      mViewport->initializeGL();
   }
}

////////////////////////////////////////////////////////////////////////////////
void dtEditQt::STAGEGLWidget::resizeGL(int width, int height)
{
   dtQt::OSGAdapterWidget::resizeGL(width, height);

   if (mViewport != NULL)
   {
      mViewport->resizeGL(width, height);
   }
}

////////////////////////////////////////////////////////////////////////////////
void dtEditQt::STAGEGLWidget::wheelEvent(QWheelEvent* e)
{
   dtQt::OSGAdapterWidget::wheelEvent(e);

   if (mViewport != NULL)
   {
      mViewport->wheelEvent(e);
   }
}

////////////////////////////////////////////////////////////////////////////////
void dtEditQt::STAGEGLWidget::keyPressEvent(QKeyEvent* e)
{
   dtQt::OSGAdapterWidget::keyPressEvent(e);
   if (mViewport != NULL)
   {
      mViewport->keyPressEvent(e);
   }
}

////////////////////////////////////////////////////////////////////////////////
void dtEditQt::STAGEGLWidget::keyReleaseEvent(QKeyEvent* e)
{
   dtQt::OSGAdapterWidget::keyReleaseEvent(e);
   if (mViewport != NULL)
   {
      mViewport->keyReleaseEvent(e);
   }
}

////////////////////////////////////////////////////////////////////////////////
void dtEditQt::STAGEGLWidget::mouseDoubleClickEvent(QMouseEvent* e)
{
   dtQt::OSGAdapterWidget::mouseDoubleClickEvent(e);
   if (mViewport != NULL)
   {
      mViewport->mouseDoubleClickEvent(e);
   }
}

////////////////////////////////////////////////////////////////////////////////
void dtEditQt::STAGEGLWidget::enterEvent(QEvent *e)
{
   mIsMouseOver = true;

   dtQt::OSGAdapterWidget::enterEvent(e);
   if (mViewport != NULL)
   {
      if (!mViewport->GetIsRemoved())
      {
         //mouse entered this Widget.  Make sure the View is in the Application
         ViewportManager::GetInstance().EnableViewport(mViewport, true);
         mViewport->SetEnabled(true); //enable the Viewport
      }
   }
}

////////////////////////////////////////////////////////////////////////////////
void dtEditQt::STAGEGLWidget::leaveEvent(QEvent *e)
{
   mIsMouseOver = false;

   dtQt::OSGAdapterWidget::leaveEvent(e);
   if (mViewport != NULL)
   {
      if (!mViewport->GetIsRemoved())
      {
         //mouse left this Widget.  Make sure the View is not in the Application
         ViewportManager::GetInstance().EnableViewport(mViewport, false);
         mViewport->SetEnabled(false); //disable the Viewport
      }
   }
}

