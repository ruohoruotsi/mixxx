#ifndef EMPTYWAVEFORM_H
#define EMPTYWAVEFORM_H

#include "waveformwidgetabstract.h"
#include <QWidget>

//This class can be used as a template file to create new WaveformWidgets
//it contain minimal set of method to re-implement

class EmptyWaveformWidget : public WaveformWidgetAbstract, public QWidget
{
public:
    virtual ~EmptyWaveformWidget();

    virtual QString getWaveformWidgetName() { return "Empty";}
    virtual WaveformWidgetType::Type getType() const { return WaveformWidgetType::EmptyWaveform;}

    virtual bool useOpenGl() const { return false;}
    virtual bool useOpenGLShaders() const { return false;}

protected:
    virtual void castToQWidget();
    virtual void paintEvent(QPaintEvent* event);

private:
    EmptyWaveformWidget() {}
    EmptyWaveformWidget( const char* group, QWidget* parent);
    friend class WaveformWidgetFactory;
};

#endif // EMPTYWAVEFORM_H
